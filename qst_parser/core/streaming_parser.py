"""
Streaming QST v4 Parser — Buffer-by-Buffer Pipeline

Two-stage pipeline:
  Stage 1: Metadata + PINF  (BufferReader)
  Stage 2: Backward timestamp + forward dispatch (events written inline)

Thread/CPU events use duration slices; all others use instant events.
KerCall/Comm events are resolved inline via _cpu_pending (no deferred storage).

Supports two output formats:
  - Perfetto protobuf (.perfetto)
  - Chrome JSON Trace Event Format (.json)
"""

import os
import struct
from typing import Dict, Tuple, Optional, Set

from ..constants import (
    InternalClass, StructType,
    _TRACE_MAX_KER_CALL_NUM, IntEventType,
)
from ..classes.thread.constants import THREAD_STATES, MAX_TH_STATE_NUM
from .file_access import open_qst
from .buffer_reader import BufferReader
from .timestamp import signed_cycle_diff, cycle_gt, cycles_to_ns


from ..event_names import (
    get_kercall_names, COMM_NAMES as _COMM_NAMES,
    COMM_FIELDS as _COMM_FIELDS, COMM_WIDE_FIELDS as _COMM_WIDE_FIELDS,
    SYSTEM_NAMES as _SYSTEM_NAMES, SYSTEM_FIELDS as _SYSTEM_FIELDS,
    get_kercall_fields, decode_io_msg_fields,
    format_kercall_args, format_general_args,
)

_POLICY_NAMES = {
    0: "UNKNOWN", 1: "SCHED_FIFO", 2: "SCHED_RR",
    3: "SCHED_OTHER", 4: "SCHED_SPORADIC",
}


def _format_policy(policy: int) -> str:
    return _POLICY_NAMES.get(policy, f"POLICY_{policy}")


class StreamingParser:
    """
    Buffer-by-buffer streaming parser for QST v4 (new format only).

    All events are resolved and written during the forward dispatch pass.
    Supports protobuf (.perfetto) and JSON (.json) output formats.
    """

    def __init__(self, filepath: str, verbose: bool = False,
                 lightweight: bool = False,
                 filter_pids: Optional[frozenset] = None,
                 filter_tids: Optional[frozenset] = None,
                 filter_time_start_ns: Optional[int] = None,
                 filter_time_end_ns: Optional[int] = None):
        self._filepath = filepath
        self._verbose = verbose
        self._lightweight = lightweight

        self._filter_pids = filter_pids
        self._filter_tids = filter_tids
        self._filter_time_start = filter_time_start_ns
        self._filter_time_end = filter_time_end_ns

        self._fa: Optional['FileAccess'] = None
        self._reader: Optional[BufferReader] = None

        # Protobuf mode
        self._builder = None
        self._tracks = None

        # JSON mode
        self._jw = None  # JsonTraceWriter

        # Combine buffers for wide-mode events (tiny, <1KB)
        self._thread_combine: Dict[str, dict] = {}
        self._proc_combine: Dict[str, dict] = {}

        self._stats = {
            'thread_slices': 0,
            'cpu_running_slices': 0,
            'interrupt_slices': 0,
            'interrupt_instants': 0,
            'kercall_instants': 0,
            'comm_instants': 0,
            'system_instants': 0,
            'sync_flows': 0,
            'ipc_msg_flows': 0,
            'ipc_send_ack_flows': 0,
            'ipc_reply_flows': 0,
            'signal_flows': 0,
            'total_events_processed': 0,
            'total_buffers': 0,
        }

        # Per-thread last-seen state for duration computation
        self._thread_pending: Dict[Tuple[int, int], tuple] = {}

        # Per-CPU currently RUNNING thread: cpu_id → (ts_ns, pid, tid)
        self._cpu_pending: Dict[int, tuple] = {}

        # Interrupt pairing: (cpu_id, irq_num) → (ts_ns_enter, args)
        self._int_pending: Dict[Tuple[int, int], tuple] = {}

        self._all_cpu_ids: Set[int] = set()
        self._all_threads: Set[Tuple[int, int]] = set()

        self._kercall_names: dict = {}

        # Generic combine buffer for wide-mode events across all classes
        # Key: (class_tag, cpu_id, d0_timestamp)
        # Value: {'int_event': ..., 'ts_ns': ..., 'data': [d1, d2, ...]}
        self._combine_buf: dict = {}

        # --- Sync flow event tracking ---
        # Thread waiting on sync: (pid,tid) → (sync_ptr, enter_ts_ns)
        self._thread_sync_wait: Dict[Tuple[int, int], Tuple[int, int]] = {}
        # Active unlock/signal/post window: cpu_id → (sync_ptr, ts_ns, pid, tid)
        self._active_sync_release: Dict[int, tuple] = {}
        self._flow_id = 0

        # --- IPC flow event tracking ---
        # Thread blocked in MsgSend: (pid,tid) → ts_ns
        self._ipc_send_pending: Dict[Tuple[int, int], int] = {}
        # MsgReply target pending: (target_pid,target_tid) → (server_pid, server_tid, ts_ns)
        self._ipc_reply_pending: Dict[Tuple[int, int], tuple] = {}

        # --- Signal flow event tracking ---
        # Pending signal: target_pid → (sender_pid, sender_tid, ts_ns, target_tid_or_0)
        self._signal_pending: Dict[int, tuple] = {}

    # =========================================================================
    # Public API
    # =========================================================================

    def parse_and_export(self, output_path: str,
                         fmt: str = 'protobuf') -> dict:
        """Run the full pipeline: parse + export.

        Args:
            output_path: destination file path
            fmt: 'protobuf' for .perfetto binary, 'json' for Chrome JSON
        """
        if fmt == 'json':
            return self._export_json(output_path)
        return self._export_protobuf(output_path)

    def _export_protobuf(self, output_path: str) -> dict:
        from ..exporters.perfetto.utils import BatchingWriter
        from ..exporters.perfetto.tracks import TrackManager

        with open(output_path, 'wb') as out_f:
            self._builder = BatchingWriter(out_f)
            self._tracks = TrackManager(self._builder)

            with open_qst(self._filepath) as fa:
                self._fa = fa
                self._reader = BufferReader(fa, verbose=self._verbose)

                self._reader.read_structure()
                self._reader.process_names.setdefault(1, 'procnto')
                self._kercall_names = get_kercall_names(
                    self._reader.header.os_version)
                self._tracks.set_names(
                    self._reader.process_names,
                    self._reader.thread_names,
                )
                self._tracks.create_cpu_tracks(
                    set(range(self._reader.header.num_cpus)))

                self._process_backward()
                self._flush_pending_slices()

            self._builder.finalize()

        self._stats['file_size_kb'] = os.path.getsize(output_path) / 1024
        return self._stats

    def _export_json(self, output_path: str) -> dict:
        from ..exporters.json_writer import JsonTraceWriter

        with open(output_path, 'w') as out_f:
            self._jw = JsonTraceWriter(out_f)

            with open_qst(self._filepath) as fa:
                self._fa = fa
                self._reader = BufferReader(fa, verbose=self._verbose)
                self._reader.read_structure()
                self._reader.process_names.setdefault(1, 'procnto')
                self._kercall_names = get_kercall_names(
                    self._reader.header.os_version)

                # Write CPU metadata — single parent process for all CPUs
                cpid = JsonTraceWriter.cpu_pid()
                self._jw.write_process_name(cpid, 'CPU Tracks')
                self._jw.write_process_sort_index(cpid, -1000)
                for cpu_id in range(self._reader.header.num_cpus):
                    self._jw.write_thread_name(
                        cpid, JsonTraceWriter.cpu_main_tid(cpu_id),
                        f'CPU {cpu_id}')
                    self._jw.write_thread_name(
                        cpid, JsonTraceWriter.cpu_irq_tid(cpu_id),
                        f'CPU {cpu_id} IRQ')

                # Write process/thread name metadata
                for pid, name in self._reader.process_names.items():
                    self._jw.write_process_name(pid, name)
                for (pid, tid), name in self._reader.thread_names.items():
                    self._jw.write_thread_name(pid, tid, name)

                self._process_backward()
                self._flush_pending_slices()

            self._jw.finalize()

        self._stats['file_size_kb'] = os.path.getsize(output_path) / 1024
        return self._stats

    def parse_summary(self) -> dict:
        """Parse without Perfetto export, return summary only."""
        with open_qst(self._filepath) as fa:
            self._fa = fa
            self._reader = BufferReader(fa, verbose=self._verbose)
            self._reader.read_structure()

        h = self._reader.header
        total_events = sum(b.num_events for b in self._reader.sorted_buffers)
        return {
            'os_version': h.os_version,
            'num_cpus': h.num_cpus,
            'clock_freq': h.clock_freq,
            'capture_start_ns': h.capture_start_ns,
            'capture_end_ns': h.capture_end_ns,
            'tracebuf_size': h.tracebuf_size,
            'data_offset': h.data_offset,
            'total_buffers': len(self._reader.sorted_buffers),
            'total_events': total_events,
            'process_names': len(self._reader.process_names),
            'thread_names': len(self._reader.thread_names),
        }

    # =========================================================================
    # Stage 2: Two-pass (backward timestamps → forward dispatch)
    # =========================================================================

    def _process_backward(self):
        h = self._reader.header
        clock_freq = h.clock_freq
        anchor_ns = h.capture_end_ns

        if h.os_version == 800 and h.bufs_per_cpu > 0:
            self._process_percpu(clock_freq, anchor_ns)
        else:
            self._process_global(clock_freq, anchor_ns)

    def _process_global(self, clock_freq: int, anchor_ns: int):
        buffers = self._reader.sorted_buffers
        if not buffers:
            return
        carries = self._compute_carries(buffers, clock_freq, anchor_ns)
        self._forward_dispatch(buffers, carries, clock_freq)

    def _process_percpu(self, clock_freq: int, anchor_ns: int):
        groups = self._reader.get_cpu_groups()

        global_max_cycle = 0
        for group in groups:
            if not group:
                continue
            last_buf = group[-1]
            last_ev_off = (last_buf.num_events - 1) * 16 + 4
            ev_data = self._fa.read_at(last_buf.file_offset + last_ev_off, 4)
            cycle = struct.unpack('<I', ev_data)[0]
            if cycle_gt(cycle, global_max_cycle):
                global_max_cycle = cycle

        for group in groups:
            if not group:
                continue
            last_buf = group[-1]
            last_ev_off = (last_buf.num_events - 1) * 16 + 4
            ev_data = self._fa.read_at(last_buf.file_offset + last_ev_off, 4)
            cpu_last_cycle = struct.unpack('<I', ev_data)[0]
            delta = signed_cycle_diff(global_max_cycle, cpu_last_cycle)
            cpu_anchor_ns = anchor_ns - cycles_to_ns(delta, clock_freq)

            carries = self._compute_carries(group, clock_freq, cpu_anchor_ns)
            self._forward_dispatch(group, carries, clock_freq)

    def _compute_carries(self, buffers: list, clock_freq: int,
                         anchor_ns: int) -> list:
        """Backward pass: compute (first_cycle, first_ts_ns) per buffer.

        Only reads the first and last event of each buffer (2 reads per buffer
        instead of N), then chains buffer-to-buffer using boundary cycles.
        Within-buffer timestamp accuracy relies on the invariant that events
        within a single tracebuf_t are chronologically ordered.
        """
        carries = []
        carry_cycle = None
        carry_ts = None

        _MAX_U32 = 0xFFFFFFFF
        _HALF_U32 = 0x7FFFFFFF
        _BILLION = 1_000_000_000
        read_at = self._fa.read_at
        unpack = struct.unpack

        for buf in reversed(buffers):
            ne = buf.num_events
            off = buf.file_offset

            first_cycle = unpack('<I', read_at(off + 4, 4))[0]
            if ne > 1:
                last_cycle = unpack('<I', read_at(off + (ne - 1) * 16 + 4, 4))[0]
            else:
                last_cycle = first_cycle

            if carry_ts is None:
                last_ts = anchor_ns
            else:
                raw = (carry_cycle - last_cycle) & _MAX_U32
                delta = raw - (_MAX_U32 + 1) if raw > _HALF_U32 else raw
                last_ts = carry_ts - delta * _BILLION // clock_freq

            raw = (last_cycle - first_cycle) & _MAX_U32
            inner_delta = raw - (_MAX_U32 + 1) if raw > _HALF_U32 else raw
            first_ts = last_ts - inner_delta * _BILLION // clock_freq

            carry_cycle = first_cycle
            carry_ts = first_ts
            carries.append((first_cycle, first_ts))

        carries.reverse()
        return carries

    def _forward_dispatch(self, buffers: list, carries: list,
                          clock_freq: int):
        """Forward pass: dispatch events in chronological order.

        Inlines timestamp arithmetic and event dispatch for speed —
        avoids per-event function-call overhead on 24M+ events.
        """
        event_fmt = struct.Struct('<IIII')
        read_at = self._fa.read_at
        dispatch = self._dispatch_event

        _MAX_U32 = 0xFFFFFFFF
        _HALF_U32 = 0x7FFFFFFF
        _BILLION = 1_000_000_000

        for buf_idx, buf in enumerate(buffers):
            events_data = read_at(buf.file_offset, buf.num_events * 16)
            ne = buf.num_events
            base_cycle, base_ts = carries[buf_idx]
            prev_cycle = base_cycle
            prev_ts = base_ts

            for i, (header, d0, d1, d2) in enumerate(
                    event_fmt.iter_unpack(events_data[:ne * 16])):
                if i == 0:
                    ts_ns = base_ts
                else:
                    raw = (d0 - prev_cycle) & _MAX_U32
                    delta = raw - (_MAX_U32 + 1) if raw > _HALF_U32 else raw
                    ts_ns = prev_ts + delta * _BILLION // clock_freq
                prev_cycle = d0
                prev_ts = ts_ns

                dispatch(header, d0, d1, d2, ts_ns)

            self._stats['total_buffers'] += 1

    # =========================================================================
    # Event Dispatch
    # =========================================================================

    def _dispatch_event(self, header: int, d0: int, d1: int, d2: int,
                        ts_ns: int):
        self._stats['total_events_processed'] += 1
        int_class = (header >> 10) & 0x1F
        int_event = header & 0x3FF
        cpu_id = (header >> 24) & 0x3F
        struct_type = (header >> 30) & 0x3

        if int_class == InternalClass.PR_TH:
            self._handle_pr_th(int_event, cpu_id, struct_type,
                               d0, d1, d2, ts_ns)
        elif self._lightweight:
            return
        elif int_class == InternalClass.INT:
            self._handle_interrupt(int_event, cpu_id, struct_type,
                                   d0, d1, d2, ts_ns)
        elif int_class == InternalClass.KER_CALL:
            self._handle_kercall(int_event, cpu_id, struct_type,
                                 d0, d1, d2, ts_ns)
        elif int_class == InternalClass.COMM:
            self._handle_comm(int_event, cpu_id, struct_type,
                              d0, d1, d2, ts_ns)
        elif int_class == InternalClass.SYSTEM:
            self._handle_system(int_event, cpu_id, struct_type,
                                d0, d1, d2, ts_ns)

    # =========================================================================
    # PR_TH handler (thread/vthread/process)
    # =========================================================================

    def _handle_pr_th(self, int_event: int, cpu_id: int, struct_type: int,
                      d0: int, d1: int, d2: int, ts_ns: int):
        if int_event >= 2 * MAX_TH_STATE_NUM:
            self._handle_process_event(int_event, cpu_id, struct_type,
                                       d0, d1, d2, ts_ns)
            return

        is_vthread = int_event >= MAX_TH_STATE_NUM
        state_code = int_event - MAX_TH_STATE_NUM if is_vthread else int_event
        state = THREAD_STATES.get(state_code, f"STATE_{state_code}")

        if struct_type == StructType.SIMPLE:
            pid, tid = d1, d2
            self._emit_thread_state(ts_ns, cpu_id, pid, tid, state,
                                    is_vthread, False, 0, 0, 0, 0)
        elif struct_type == StructType.COMBINE_BEGIN:
            key = f"th_{cpu_id}_{d0}"
            self._thread_combine[key] = {
                'cpu_id': cpu_id, 'pid': d1, 'tid': d2,
                'state': state, 'is_vthread': is_vthread,
                'priority': 0, 'policy': 0,
                'partition_id': 0, 'sched_flags': 0,
                'has_cont': False, 'ts_ns': ts_ns,
            }
        elif struct_type == StructType.COMBINE_CONT:
            key = f"th_{cpu_id}_{d0}"
            buf = self._thread_combine.get(key)
            if buf:
                buf['priority'] = d1
                buf['policy'] = d2
                buf['has_cont'] = True
        elif struct_type == StructType.COMBINE_END:
            key = f"th_{cpu_id}_{d0}"
            buf = self._thread_combine.pop(key, None)
            if buf:
                if buf['has_cont']:
                    buf['partition_id'] = d1
                    buf['sched_flags'] = d2
                else:
                    buf['priority'] = d1
                    buf['policy'] = d2
                self._emit_thread_state(
                    buf['ts_ns'], buf['cpu_id'], buf['pid'], buf['tid'],
                    buf['state'], buf['is_vthread'], True,
                    buf['priority'], buf['policy'],
                    buf['partition_id'], buf['sched_flags'],
                )

    @property
    def _is_json(self) -> bool:
        return self._jw is not None

    def _in_time_range(self, ts_ns: int) -> bool:
        if self._filter_time_start and ts_ns < self._filter_time_start:
            return False
        if self._filter_time_end and ts_ns > self._filter_time_end:
            return False
        return True

    def _pid_tid_match(self, pid: int, tid: int = 0) -> bool:
        """Check if (pid, tid) passes the PID/TID filter.
        Returns True if no filter is set or the pid/tid matches.
        """
        if not self._filter_pids:
            return True
        if pid not in self._filter_pids:
            return False
        if self._filter_tids and tid and tid not in self._filter_tids:
            return False
        return True

    def _emit_thread_state(self, ts_ns: int, cpu_id: int, pid: int, tid: int,
                           state: str, is_vthread: bool, is_wide: bool,
                           priority: int, policy: int,
                           partition_id: int, sched_flags: int):
        if state in ('CREATE', 'DESTROY'):
            return

        self._all_cpu_ids.add(cpu_id)
        self._all_threads.add((pid, tid))

        if self._builder is None and self._jw is None:
            return

        key = (pid, tid)
        emit = self._pid_tid_match(pid, tid) and self._in_time_range(ts_ns)

        if key in self._thread_pending:
            prev = self._thread_pending[key]
            prev_ts, prev_state, prev_cpu, prev_pri, prev_pol = prev[:5]
            prev_vth, prev_wide = prev[5], prev[6]

            if emit:
                if self._is_json:
                    args = self._build_thread_args(
                        pid, tid, prev_cpu, prev_pri, prev_pol,
                        prev_vth, prev_wide)
                    self._jw.write_complete(
                        pid, tid, prev_ts / 1000, (ts_ns - prev_ts) / 1000,
                        prev_state, 'thread', args)
                else:
                    from ..exporters.perfetto.utils import write_slice, format_policy
                    track_uuid = self._tracks._ensure_thread_track(pid, tid)
                    if track_uuid:
                        args = self._build_thread_args(
                            pid, tid, prev_cpu, prev_pri, prev_pol,
                            prev_vth, prev_wide)
                        write_slice(self._builder, prev_ts, ts_ns,
                                    track_uuid, prev_state, args)
                self._stats['thread_slices'] += 1

            self._check_sync_flow(prev_state, state, pid, tid, ts_ns)
            self._check_ipc_reply_flow(prev_state, state, pid, tid, ts_ns)
            self._check_signal_flow(prev_state, state, pid, tid, ts_ns)

            if prev_state == 'RUNNING' and prev_cpu in self._cpu_pending:
                cp = self._cpu_pending.pop(prev_cpu)
                if emit:
                    tname = self._get_thread_name(cp[1], cp[2])
                    cpu_args = {
                        'process': self._get_process_label(cp[1]),
                        'thread': self._get_thread_label(cp[1], cp[2]),
                        'priority': prev_pri,
                        'policy': _format_policy(prev_pol),
                    }
                    if self._is_json:
                        from ..exporters.json_writer import JsonTraceWriter
                        cpid = JsonTraceWriter.cpu_pid()
                        self._jw.write_complete(
                            cpid, JsonTraceWriter.cpu_main_tid(prev_cpu),
                            cp[0] / 1000, (ts_ns - cp[0]) / 1000,
                            tname, 'cpu', cpu_args)
                    else:
                        from ..exporters.perfetto.utils import write_slice
                        cpu_track = self._tracks.get_cpu_track(prev_cpu)
                        if cpu_track:
                            write_slice(self._builder, cp[0], ts_ns,
                                        cpu_track, tname, cpu_args)
                    self._stats['cpu_running_slices'] += 1

        self._thread_pending[key] = (
            ts_ns, state, cpu_id, priority, policy, is_vthread, is_wide
        )
        if state == 'RUNNING':
            self._cpu_pending[cpu_id] = (ts_ns, pid, tid)

    def _get_process_label(self, pid: int) -> str:
        name = self._reader.process_names.get(pid, f"Process {pid}")
        return f"{name} ({pid})"

    def _get_thread_label(self, pid: int, tid: int) -> str:
        tname = self._reader.thread_names.get((pid, tid))
        if tname:
            return f"{tname} ({pid}:{tid})"
        pname = self._reader.process_names.get(pid)
        if pname:
            return f"{pname}:{tid} ({pid}:{tid})"
        return f"Thread ({pid}:{tid})"

    def _get_thread_name(self, pid: int, tid: int) -> str:
        tname = self._reader.thread_names.get((pid, tid))
        pname = self._reader.process_names.get(pid)
        if tname:
            return tname
        if pname:
            return f"{pname}:{tid}"
        return f"pid={pid}:{tid}"

    def _resolve_pid_tid_args(self, args: dict):
        """Resolve pid/tid data fields to human-readable 'name(id)' format.

        Skips the event owner's 'pid'/'tid' keys only when companion
        'process'/'thread' fields are already present.
        """
        has_owner = 'process' in args
        pnames = self._reader.process_names
        tnames = self._reader.thread_names
        for key in list(args.keys()):
            val = args[key]
            if not isinstance(val, int) or val <= 0:
                continue
            kl = key.lower()
            if 'pid' in kl:
                if key == 'pid' and has_owner:
                    continue
                name = pnames.get(val)
                if name:
                    args[key] = f"{name}({val})"
            elif 'tid' in kl:
                if key == 'tid' and has_owner:
                    continue
                if key == 'tid':
                    pid_key = 'pid'
                else:
                    pid_key = key.replace('tid', 'pid')
                ppid = args.get(pid_key)
                if isinstance(ppid, str) and '(' in ppid:
                    try:
                        ppid = int(ppid.rsplit('(', 1)[-1].rstrip(')'))
                    except (ValueError, IndexError):
                        ppid = None
                if isinstance(ppid, int) and ppid > 0:
                    tname = tnames.get((ppid, val))
                    if tname:
                        args[key] = f"{tname}:{val}"

    def _build_thread_args(self, pid, tid, cpu_id, priority, policy,
                           is_vthread, is_wide):
        args = {
            'process': self._get_process_label(pid),
            'thread': self._get_thread_label(pid, tid),
            'pid': pid, 'tid': tid, 'cpu': cpu_id,
        }
        if is_wide:
            args['priority'] = priority
            args['policy'] = _format_policy(policy)
        if is_vthread:
            args['is_vthread'] = True
        return args

    def _handle_process_event(self, int_event: int, cpu_id: int,
                              struct_type: int,
                              d0: int, d1: int, d2: int, ts_ns: int):
        """Handle process lifecycle events and name extraction."""
        shift = (int_event >> 6)
        if shift < 1:
            return
        ext_event = 1 << (shift - 1)
        _PROC_CREATE = 1
        _PROC_DESTROY = 2
        _PROCCREATE_NAME = 4
        _PROCTHREAD_NAME = 16

        if ext_event == _PROC_CREATE and struct_type == StructType.SIMPLE:
            self._emit_proc_lifecycle(
                ts_ns, cpu_id, d1, d2, 'PROC_CREATE')
            return
        if ext_event == _PROC_DESTROY and struct_type == StructType.SIMPLE:
            self._emit_proc_lifecycle(
                ts_ns, cpu_id, d1, d2, 'PROC_DESTROY')
            return

        if ext_event not in (_PROCCREATE_NAME, _PROCTHREAD_NAME):
            return

        key = f"proc_{cpu_id}"

        if struct_type == StructType.COMBINE_BEGIN:
            if ext_event == _PROCCREATE_NAME:
                self._proc_combine[key] = {
                    'type': 'proc', 'pid': d2, 'name_parts': [],
                }
            else:
                self._proc_combine[key] = {
                    'type': 'thread', 'pid': d1, 'tid': d2,
                    'name_parts': [],
                }
        elif struct_type in (StructType.COMBINE_CONT, StructType.COMBINE_END):
            buf = self._proc_combine.get(key)
            if buf is None:
                return
            buf['name_parts'].append(struct.pack('<II', d1, d2))
            if struct_type == StructType.COMBINE_END:
                raw = b''.join(buf['name_parts'])
                name = raw.split(b'\x00')[0].decode(
                    'utf-8', errors='replace').strip()
                name = ''.join(c for c in name if c >= ' ' or c == '\t')
                if buf['type'] == 'proc':
                    pid = buf['pid']
                    self._reader.process_names[pid] = name
                    if self._tracks:
                        from ..exporters.perfetto.tracks import TrackManager
                        self._tracks._process_names[pid] = \
                            TrackManager._sanitize_name(name)
                    if self._jw:
                        self._jw.write_process_name(pid, name)
                else:
                    pid, tid = buf['pid'], buf['tid']
                    self._reader.thread_names[(pid, tid)] = name
                    if self._tracks:
                        from ..exporters.perfetto.tracks import TrackManager
                        self._tracks._thread_names[(pid, tid)] = \
                            TrackManager._sanitize_name(name)
                    if self._jw:
                        self._jw.write_thread_name(pid, tid, name)
                del self._proc_combine[key]

    def _emit_proc_lifecycle(self, ts_ns: int, cpu_id: int,
                             ppid: int, pid: int, name: str):
        """Emit a process CREATE/DESTROY instant event."""
        if self._lightweight:
            return
        if not self._in_time_range(ts_ns):
            return
        if self._filter_pids and not (
                self._pid_tid_match(ppid, 0) or
                self._pid_tid_match(pid, 0)):
            return
        args = {
            'ppid': ppid,
            'pid': pid,
            'parent': self._get_process_label(ppid),
            'process': self._get_process_label(pid),
            'cpu': cpu_id,
        }
        if self._jw:
            self._jw.write_instant(
                ppid, 1, ts_ns / 1000, name, 'process', args)
        elif self._builder:
            track_uuid = self._tracks.get_thread_track(ppid, 1)
            from ..exporters.perfetto.utils import write_instant
            write_instant(self._builder, ts_ns, track_uuid, name, args)

    # =========================================================================
    # Interrupt handler
    # =========================================================================

    def _handle_interrupt(self, int_event: int, cpu_id: int, struct_type: int,
                          d0: int, d1: int, d2: int, ts_ns: int):
        if self._builder is None and self._jw is None:
            return

        self._all_cpu_ids.add(cpu_id)

        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buf[('int', cpu_id, d0)] = {
                'ev': int_event, 'cpu': cpu_id, 'ts': ts_ns,
                'data': [d1, d2],
            }
            return
        elif struct_type == StructType.COMBINE_CONT:
            key = ('int', cpu_id, d0)
            buf = self._combine_buf.get(key)
            if buf:
                buf['data'].extend([d1, d2])
            return
        elif struct_type == StructType.COMBINE_END:
            key = ('int', cpu_id, d0)
            buf = self._combine_buf.pop(key, None)
            if buf:
                buf['data'].extend([d1, d2])
                self._emit_interrupt(buf['ev'], cpu_id, buf['data'],
                                     buf['ts'])
            return

        self._emit_interrupt(int_event, cpu_id, [d1, d2], ts_ns)

    def _emit_interrupt(self, int_event: int, cpu_id: int,
                        data: list, ts_ns: int):
        """Emit an interrupt event after optional combine reassembly."""
        if int_event == IntEventType.ENTRY:
            irq_num = data[0]
            args = {'irq': irq_num, 'cpu': cpu_id, 'ip': data[1]}
        elif int_event == IntEventType.ENTRY_64:
            irq_num = data[0]
            ip = data[2] | (data[3] << 32) if len(data) > 3 else data[1]
            args = {'irq': irq_num, 'cpu': cpu_id, 'ip': f"0x{ip:x}"}
        elif int_event == IntEventType.HANDLER_ENTRY:
            irq_num = data[1] if len(data) > 1 else data[0]
            pid = data[0]
            args = {'irq': irq_num, 'cpu': cpu_id, 'pid': pid,
                    'ip': data[2] if len(data) > 2 else 0,
                    'area': data[3] if len(data) > 3 else 0}
        elif int_event == IntEventType.HANDLER_ENTRY_64:
            irq_num = data[1] if len(data) > 1 else data[0]
            pid = data[0]
            ip = (data[2] | (data[3] << 32)) if len(data) > 3 else 0
            area = (data[4] | (data[5] << 32)) if len(data) > 5 else 0
            args = {'irq': irq_num, 'cpu': cpu_id, 'pid': pid,
                    'ip': f"0x{ip:x}", 'area': f"0x{area:x}"}
        elif int_event == IntEventType.EXIT:
            irq_num = data[0]
            key = (cpu_id, irq_num)
            enter = self._int_pending.pop(key, None)
            if enter and self._in_time_range(ts_ns):
                if self._is_json:
                    from ..exporters.json_writer import JsonTraceWriter
                    cpid = JsonTraceWriter.cpu_pid()
                    self._jw.write_complete(
                        cpid, JsonTraceWriter.cpu_irq_tid(cpu_id),
                        enter[0] / 1000, (ts_ns - enter[0]) / 1000,
                        f"IRQ{irq_num}", 'irq', enter[1])
                else:
                    from ..exporters.perfetto.utils import write_slice
                    track_uuid = self._tracks.get_or_create_cpu_irq_track(
                        cpu_id)
                    if track_uuid:
                        write_slice(self._builder, enter[0], ts_ns,
                                    track_uuid, f"IRQ{irq_num}", enter[1])
                self._stats['interrupt_slices'] += 1
                return
            if not enter:
                args = {'irq': irq_num, 'cpu': cpu_id,
                        'kernel_flag': data[1] if len(data) > 1 else 0}
                if self._in_time_range(ts_ns):
                    self._emit_irq_instant(cpu_id, ts_ns,
                                           f"IRQ{irq_num}_EXIT", args)
                    self._stats['interrupt_instants'] += 1
            return
        elif int_event == IntEventType.HANDLER_EXIT:
            irq_num = data[0]
            key = (cpu_id, irq_num)
            enter = self._int_pending.pop(key, None)
            if enter and self._in_time_range(ts_ns):
                enter[1]['sigevent'] = data[1] if len(data) > 1 else 0
                if self._is_json:
                    from ..exporters.json_writer import JsonTraceWriter
                    cpid = JsonTraceWriter.cpu_pid()
                    self._jw.write_complete(
                        cpid, JsonTraceWriter.cpu_irq_tid(cpu_id),
                        enter[0] / 1000, (ts_ns - enter[0]) / 1000,
                        f"IRQ{irq_num}", 'irq', enter[1])
                else:
                    from ..exporters.perfetto.utils import write_slice
                    track_uuid = self._tracks.get_or_create_cpu_irq_track(
                        cpu_id)
                    if track_uuid:
                        write_slice(self._builder, enter[0], ts_ns,
                                    track_uuid, f"IRQ{irq_num}", enter[1])
                self._stats['interrupt_slices'] += 1
                return
            if not enter:
                args = {'irq': irq_num, 'cpu': cpu_id,
                        'sigevent': data[1] if len(data) > 1 else 0}
                if self._in_time_range(ts_ns):
                    self._emit_irq_instant(cpu_id, ts_ns,
                                           f"IRQ{irq_num}_HANDLER_EXIT", args)
                    self._stats['interrupt_instants'] += 1
            return
        else:
            return

        key = (cpu_id, irq_num)
        self._int_pending[key] = (ts_ns, args)

    def _emit_irq_instant(self, cpu_id: int, ts_ns: int,
                          name: str, args: dict):
        if self._is_json:
            from ..exporters.json_writer import JsonTraceWriter
            cpid = JsonTraceWriter.cpu_pid()
            self._jw.write_instant(
                cpid, JsonTraceWriter.cpu_irq_tid(cpu_id),
                ts_ns / 1000, name, 'irq', args)
        else:
            from ..exporters.perfetto.utils import write_instant
            track_uuid = self._tracks.get_or_create_cpu_irq_track(cpu_id)
            if track_uuid:
                write_instant(self._builder, ts_ns, track_uuid, name, args)

    # =========================================================================
    # KerCall handler (inline resolve via _cpu_pending)
    # =========================================================================

    def _handle_kercall(self, int_event: int, cpu_id: int,
                        struct_type: int, d0: int,
                        d1: int, d2: int, ts_ns: int):
        if self._builder is None and self._jw is None:
            return

        if struct_type == StructType.SIMPLE:
            self._emit_kercall(int_event, cpu_id, [d1, d2], False, ts_ns)
        elif struct_type == StructType.COMBINE_BEGIN:
            key = ('ker', cpu_id, d0)
            self._combine_buf[key] = {
                'int_event': int_event, 'cpu_id': cpu_id,
                'ts_ns': ts_ns, 'data': [d1, d2],
            }
        elif struct_type == StructType.COMBINE_CONT:
            key = ('ker', cpu_id, d0)
            buf = self._combine_buf.get(key)
            if buf:
                buf['data'].extend([d1, d2])
        elif struct_type == StructType.COMBINE_END:
            key = ('ker', cpu_id, d0)
            buf = self._combine_buf.pop(key, None)
            if buf:
                buf['data'].extend([d1, d2])
                self._emit_kercall(
                    buf['int_event'], buf['cpu_id'],
                    buf['data'], True, buf['ts_ns'])

    def _emit_kercall(self, int_event: int, cpu_id: int,
                      data_words: list, is_wide: bool, ts_ns: int):
        call_num = int_event & 0x7F
        base_event = int_event & ~0x200  # strip _NTO_TRACE_KERCALL64
        if base_event >= 2 * _TRACE_MAX_KER_CALL_NUM:
            suffix = '_INT'
            is_enter = False
        elif base_event >= _TRACE_MAX_KER_CALL_NUM:
            suffix = '_EXIT'
            is_enter = False
        else:
            suffix = '_ENTER'
            is_enter = True

        is_64 = (int_event & 0x200) != 0
        base_name = self._kercall_names.get(call_num, f"KerCall_{call_num}")
        name = f"{base_name}{suffix}"

        fast_fields, wide_fields = get_kercall_fields(
            call_num, is_enter, is_64=is_64)
        fields = wide_fields if is_wide else fast_fields
        args = {}
        for i, val in enumerate(data_words):
            key = fields[i] if i < len(fields) else f"d{i}"
            if key != 'empty':
                args[key] = val

        # Rename data fields that collide with owner's pid/tid
        for collision_key in ('pid', 'tid'):
            if collision_key in args:
                args[f'target_{collision_key}'] = args.pop(collision_key)

        if is_enter and call_num in (11, 12):
            decode_io_msg_fields(args, 'msg')
        elif not is_enter and suffix == '_EXIT' and call_num in (14, 24):
            decode_io_msg_fields(args, 'rmsg')

        cp = self._cpu_pending.get(cpu_id)
        # Capture raw fields before formatting (needed for flow tracking)
        self._track_sync_flow(call_num, is_enter, suffix, cpu_id,
                              pid_hint=cp, raw_args=args, ts_ns=ts_ns)
        self._track_ipc_kercall(call_num, is_enter, suffix, cpu_id,
                                pid_hint=cp, raw_args=args, ts_ns=ts_ns)
        self._track_signal_kercall(call_num, is_enter, cpu_id,
                                   pid_hint=cp, raw_args=args, ts_ns=ts_ns)

        pid = cp[1] if cp else 0
        tid = cp[2] if cp else 0
        args['cpu'] = cpu_id
        if pid:
            args['pid'] = pid
            args['tid'] = tid
            args['process'] = self._get_process_label(pid)
            args['thread'] = self._get_thread_label(pid, tid)

        self._resolve_pid_tid_args(args)
        format_kercall_args(args, call_num)

        if self._pid_tid_match(pid, tid) and self._in_time_range(ts_ns):
            if self._is_json:
                if pid and tid:
                    self._jw.write_instant(pid, tid, ts_ns / 1000,
                                           name, 'kercall', args)
                else:
                    from ..exporters.json_writer import JsonTraceWriter
                    cpid = JsonTraceWriter.cpu_pid()
                    self._jw.write_instant(
                        cpid, JsonTraceWriter.cpu_irq_tid(cpu_id),
                        ts_ns / 1000, name, 'kercall', args)
            else:
                from ..exporters.perfetto.utils import write_instant
                if pid and tid:
                    track_uuid = self._tracks._ensure_thread_track(pid, tid)
                else:
                    track_uuid = self._tracks.get_or_create_cpu_irq_track(
                        cpu_id)
                if track_uuid:
                    write_instant(self._builder, ts_ns, track_uuid, name, args)
            self._stats['kercall_instants'] += 1

    # =========================================================================
    # Sync flow event tracking
    # =========================================================================

    _SYNC_WAIT_CALLS = frozenset({80, 82, 85})  # MutexLock, CondvarWait, SemWait
    _SYNC_RELEASE_CALLS = frozenset({81, 83, 84})  # MutexUnlock, CondvarSignal, SemPost

    _SYNC_STATE_MAP = {
        'MUTEX': 'mutex',
        'CONDVAR': 'condvar',
        'SEM': 'sem',
    }

    def _track_sync_flow(self, call_num: int, is_enter: bool, suffix: str,
                         cpu_id: int, pid_hint, raw_args: dict,
                         ts_ns: int):
        """Track sync waiter/releaser for flow event generation.

        Called BEFORE formatting, so raw_args values are still integers.
        """
        if not is_enter:
            if suffix == '_EXIT':
                self._active_sync_release.pop(cpu_id, None)
            return

        pid = pid_hint[1] if pid_hint else 0
        tid = pid_hint[2] if pid_hint else 0
        if not pid:
            return

        sync_ptr = raw_args.get('sync_p')
        if sync_ptr is None:
            lo = raw_args.get('sync_p_lo')
            hi = raw_args.get('sync_p_hi', 0)
            if lo is not None:
                sync_ptr = ((hi << 32) | (lo & 0xFFFFFFFF)) if hi else lo
        if not sync_ptr:
            return

        if call_num in self._SYNC_WAIT_CALLS:
            self._thread_sync_wait[(pid, tid)] = (sync_ptr, ts_ns)
        elif call_num in self._SYNC_RELEASE_CALLS:
            self._active_sync_release[cpu_id] = (sync_ptr, ts_ns, pid, tid)

    def _check_sync_flow(self, prev_state: str, new_state: str,
                         pid: int, tid: int, ts_ns: int):
        """Check if a thread state transition matches a sync flow.

        Called from _emit_thread_state when prev_state is
        MUTEX/CONDVAR/SEM and new_state is READY: the thread was woken
        by a releaser whose Unlock/Signal/Post window is still active.
        """
        sync_type = self._SYNC_STATE_MAP.get(prev_state)
        if not sync_type or new_state != 'READY':
            return

        wait_info = self._thread_sync_wait.pop((pid, tid), None)
        if not wait_info:
            return
        sync_ptr, wait_ts = wait_info

        release_info = None
        for cpu_release in self._active_sync_release.values():
            if cpu_release[0] == sync_ptr:
                release_info = cpu_release
                break

        if not release_info:
            return

        _, rel_ts, rel_pid, rel_tid = release_info
        if not self._in_time_range(ts_ns):
            return
        if self._filter_pids and not (
                self._pid_tid_match(pid, tid) or
                self._pid_tid_match(rel_pid, rel_tid)):
            return

        self._flow_id += 1
        flow_name = f'sync_{sync_type}'

        if self._jw:
            self._jw.write_flow_start(
                rel_pid, rel_tid, rel_ts / 1000,
                self._flow_id, flow_name, 'sync')
            self._jw.write_flow_end(
                pid, tid, ts_ns / 1000,
                self._flow_id, flow_name, 'sync')

        self._stats['sync_flows'] += 1

    # =========================================================================
    # IPC flow event tracking
    # =========================================================================

    _IPC_SEND_CALLS = frozenset({11, 12})  # MsgSendv, MsgSendvnc
    _IPC_RECV_CALL = 14                     # MsgReceivev

    def _track_ipc_kercall(self, call_num: int, is_enter: bool,
                           suffix: str, cpu_id: int,
                           pid_hint, raw_args: dict, ts_ns: int):
        """Track IPC message send/receive for flow event generation."""
        pid = pid_hint[1] if pid_hint else 0
        tid = pid_hint[2] if pid_hint else 0
        if not pid:
            return

        if is_enter and call_num in self._IPC_SEND_CALLS:
            self._ipc_send_pending[(pid, tid)] = ts_ns
        elif not is_enter and suffix == '_EXIT' and call_num == self._IPC_RECV_CALL:
            sender_pid = raw_args.get('info_pid', 0)
            sender_tid = raw_args.get('info_tid', 0)
            if not sender_pid or not sender_tid:
                return
            send_ts = self._ipc_send_pending.pop(
                (sender_pid, sender_tid), None)
            if send_ts is None:
                return

            if not self._in_time_range(ts_ns):
                return
            if self._filter_pids and not (
                    self._pid_tid_match(pid, tid) or
                    self._pid_tid_match(sender_pid, sender_tid)):
                return

            if self._jw:
                self._flow_id += 1
                self._jw.write_flow_start(
                    sender_pid, sender_tid, send_ts / 1000,
                    self._flow_id, 'ipc_msg', 'ipc')
                self._jw.write_flow_end(
                    pid, tid, ts_ns / 1000,
                    self._flow_id, 'ipc_msg', 'ipc')
                self._stats['ipc_msg_flows'] += 1

                self._flow_id += 1
                self._jw.write_flow_start(
                    pid, tid, ts_ns / 1000,
                    self._flow_id, 'ipc_send_ack', 'ipc')
                self._jw.write_flow_end(
                    sender_pid, sender_tid, ts_ns / 1000,
                    self._flow_id, 'ipc_send_ack', 'ipc')
                self._stats['ipc_send_ack_flows'] += 1

    def _track_ipc_comm(self, comm_code: int, pid: int, tid: int,
                        raw_args: dict, ts_ns: int):
        """Track COMM REPLY events for reply flow generation."""
        if comm_code != 10:  # REPLY
            return
        if not pid:
            return
        target_pid = raw_args.get('target_pid', 0)
        target_tid = raw_args.get('target_tid', 0)
        if target_pid and target_tid:
            self._ipc_reply_pending[(target_pid, target_tid)] = (
                pid, tid, ts_ns)

    def _check_ipc_reply_flow(self, prev_state: str, new_state: str,
                              pid: int, tid: int, ts_ns: int):
        """Check if REPLY → READY matches a pending MsgReply."""
        if prev_state != 'REPLY' or new_state != 'READY':
            return

        reply_info = self._ipc_reply_pending.pop((pid, tid), None)
        if not reply_info:
            return

        server_pid, server_tid, reply_ts = reply_info
        if not self._in_time_range(ts_ns):
            return
        if self._filter_pids and not (
                self._pid_tid_match(pid, tid) or
                self._pid_tid_match(server_pid, server_tid)):
            return

        self._flow_id += 1
        if self._jw:
            self._jw.write_flow_start(
                server_pid, server_tid, reply_ts / 1000,
                self._flow_id, 'ipc_reply', 'ipc')
            self._jw.write_flow_end(
                pid, tid, ts_ns / 1000,
                self._flow_id, 'ipc_reply', 'ipc')
        self._stats['ipc_reply_flows'] += 1

    # =========================================================================
    # Signal flow event tracking
    # =========================================================================

    _SIGNAL_KILL_CALLS = frozenset({26, 33})  # SignalKill, SignalKillSigval

    def _track_signal_kercall(self, call_num: int, is_enter: bool,
                              cpu_id: int, pid_hint, raw_args: dict,
                              ts_ns: int):
        """Track SignalKill calls for signal delivery flow generation.

        SignalKill args have 'pid' (target) before owner pid is added.
        QNX convention: pid=0 means "calling process" (self-signal).
        """
        if not is_enter or call_num not in self._SIGNAL_KILL_CALLS:
            return
        sender_pid = pid_hint[1] if pid_hint else 0
        sender_tid = pid_hint[2] if pid_hint else 0
        if not sender_pid:
            return
        target_pid = raw_args.get('pid', 0)
        if target_pid == 0:
            target_pid = sender_pid
        target_tid = raw_args.get('tid', 0)
        self._signal_pending[target_pid] = (
            sender_pid, sender_tid, ts_ns, target_tid)

    def _check_signal_flow(self, prev_state: str, new_state: str,
                           pid: int, tid: int, ts_ns: int):
        """Check if SIGWAITINFO/SIGSUSPEND → READY matches a pending signal."""
        if prev_state not in ('SIGWAITINFO', 'SIGSUSPEND') or new_state != 'READY':
            return

        sig_info = self._signal_pending.pop(pid, None)
        if not sig_info:
            return

        sender_pid, sender_tid, signal_ts, target_tid = sig_info
        if target_tid != 0 and target_tid != tid:
            return

        if not self._in_time_range(ts_ns):
            return
        if self._filter_pids and not (
                self._pid_tid_match(pid, tid) or
                self._pid_tid_match(sender_pid, sender_tid)):
            return

        self._flow_id += 1
        if self._jw:
            self._jw.write_flow_start(
                sender_pid, sender_tid, signal_ts / 1000,
                self._flow_id, 'signal', 'signal')
            self._jw.write_flow_end(
                pid, tid, ts_ns / 1000,
                self._flow_id, 'signal', 'signal')
        self._stats['signal_flows'] += 1

    # =========================================================================
    # Comm handler (inline resolve via _cpu_pending)
    # =========================================================================

    def _handle_comm(self, int_event: int, cpu_id: int,
                     struct_type: int, d0: int,
                     d1: int, d2: int, ts_ns: int):
        if self._builder is None and self._jw is None:
            return

        if struct_type == StructType.SIMPLE:
            self._emit_comm(int_event, cpu_id, [d1, d2], False, ts_ns)
        elif struct_type == StructType.COMBINE_BEGIN:
            key = ('comm', cpu_id, d0)
            self._combine_buf[key] = {
                'int_event': int_event, 'cpu_id': cpu_id,
                'ts_ns': ts_ns, 'data': [d1, d2],
            }
        elif struct_type == StructType.COMBINE_CONT:
            key = ('comm', cpu_id, d0)
            buf = self._combine_buf.get(key)
            if buf:
                buf['data'].extend([d1, d2])
        elif struct_type == StructType.COMBINE_END:
            key = ('comm', cpu_id, d0)
            buf = self._combine_buf.pop(key, None)
            if buf:
                buf['data'].extend([d1, d2])
                self._emit_comm(
                    buf['int_event'], buf['cpu_id'],
                    buf['data'], True, buf['ts_ns'])

    def _emit_comm(self, int_event: int, cpu_id: int,
                   data_words: list, is_wide: bool, ts_ns: int):
        comm_code = int_event & 0x1F
        name = _COMM_NAMES.get(comm_code, f"COMM_{int_event}")

        if is_wide and comm_code in _COMM_WIDE_FIELDS:
            fields = _COMM_WIDE_FIELDS[comm_code]
            args = {}
            for i, val in enumerate(data_words):
                key = fields[i] if i < len(fields) else f"d{i}"
                args[key] = val
        else:
            f1, f2 = _COMM_FIELDS.get(comm_code, ('d1', 'd2'))
            args = {f1: data_words[0], f2: data_words[1]} if len(
                data_words) >= 2 else {'d1': data_words[0] if data_words else 0}

        cp = self._cpu_pending.get(cpu_id)
        pid = cp[1] if cp else 0
        tid = cp[2] if cp else 0
        args['cpu'] = cpu_id
        if pid:
            args['pid'] = pid
            args['tid'] = tid

        self._track_ipc_comm(comm_code, pid, tid, args, ts_ns)

        self._resolve_pid_tid_args(args)
        format_general_args(args)

        if self._pid_tid_match(pid, tid) and self._in_time_range(ts_ns):
            if self._is_json:
                if pid and tid:
                    self._jw.write_instant(pid, tid, ts_ns / 1000,
                                           name, 'comm', args)
                else:
                    from ..exporters.json_writer import JsonTraceWriter
                    cpid = JsonTraceWriter.cpu_pid()
                    self._jw.write_instant(
                        cpid, JsonTraceWriter.cpu_irq_tid(cpu_id),
                        ts_ns / 1000, name, 'comm', args)
            else:
                from ..exporters.perfetto.utils import write_instant
                if pid and tid:
                    track_uuid = self._tracks._ensure_thread_track(pid, tid)
                else:
                    track_uuid = self._tracks.get_or_create_cpu_irq_track(
                        cpu_id)
                if track_uuid:
                    write_instant(self._builder, ts_ns, track_uuid, name, args)
            self._stats['comm_instants'] += 1

    # =========================================================================
    # System handler (streaming instant)
    # =========================================================================

    def _handle_system(self, int_event: int, cpu_id: int,
                       struct_type: int, d0: int,
                       d1: int, d2: int, ts_ns: int):
        if self._builder is None and self._jw is None:
            return

        if struct_type == StructType.SIMPLE:
            self._emit_system(int_event, cpu_id, [d1, d2], False, ts_ns)
        elif struct_type == StructType.COMBINE_BEGIN:
            key = ('sys', cpu_id, d0)
            self._combine_buf[key] = {
                'int_event': int_event, 'cpu_id': cpu_id,
                'ts_ns': ts_ns, 'data': [d1, d2],
            }
        elif struct_type == StructType.COMBINE_CONT:
            key = ('sys', cpu_id, d0)
            buf = self._combine_buf.get(key)
            if buf:
                buf['data'].extend([d1, d2])
        elif struct_type == StructType.COMBINE_END:
            key = ('sys', cpu_id, d0)
            buf = self._combine_buf.pop(key, None)
            if buf:
                buf['data'].extend([d1, d2])
                self._emit_system(
                    buf['int_event'], buf['cpu_id'],
                    buf['data'], True, buf['ts_ns'])

    def _emit_system(self, int_event: int, cpu_id: int,
                     data_words: list, is_wide: bool, ts_ns: int):
        sys_code = int_event & 0x1F
        name = _SYSTEM_NAMES.get(sys_code, f"SYSTEM_{int_event}")
        self._all_cpu_ids.add(cpu_id)

        if sys_code in _SYSTEM_FIELDS:
            fast_f, wide_f = _SYSTEM_FIELDS[sys_code]
            fields = wide_f if is_wide else fast_f
        else:
            fields = None

        args = {'cpu': cpu_id}
        if fields:
            for i, val in enumerate(data_words):
                key = fields[i] if i < len(fields) else f"d{i}"
                args[key] = val
        else:
            for i, val in enumerate(data_words):
                args[f'd{i}'] = val

        self._resolve_pid_tid_args(args)

        if self._in_time_range(ts_ns):
            if self._is_json:
                from ..exporters.json_writer import JsonTraceWriter
                cpid = JsonTraceWriter.cpu_pid()
                self._jw.write_instant(
                    cpid, JsonTraceWriter.cpu_irq_tid(cpu_id),
                    ts_ns / 1000, name, 'system', args)
            else:
                from ..exporters.perfetto.utils import write_instant
                track_uuid = self._tracks.get_or_create_cpu_irq_track(cpu_id)
                if track_uuid:
                    write_instant(self._builder, ts_ns, track_uuid, name, args)
            self._stats['system_instants'] += 1

    # =========================================================================
    # Flush pending slices
    # =========================================================================

    def _flush_pending_slices(self):
        """Close all open thread/cpu slices with capture_end_ns."""
        if self._builder is None and self._jw is None:
            return

        end_ns = self._reader.header.capture_end_ns

        for (pid, tid), prev in self._thread_pending.items():
            prev_ts, prev_state, prev_cpu, prev_pri, prev_pol = prev[:5]
            prev_vth, prev_wide = prev[5], prev[6]
            if prev_ts >= end_ns:
                continue
            if not self._pid_tid_match(pid, tid):
                continue
            if not self._in_time_range(end_ns):
                continue
            args = self._build_thread_args(
                pid, tid, prev_cpu, prev_pri, prev_pol, prev_vth, prev_wide)
            if self._is_json:
                self._jw.write_complete(
                    pid, tid, prev_ts / 1000, (end_ns - prev_ts) / 1000,
                    prev_state, 'thread', args)
            else:
                from ..exporters.perfetto.utils import write_slice
                track_uuid = self._tracks._ensure_thread_track(pid, tid)
                if track_uuid:
                    write_slice(self._builder, prev_ts, end_ns,
                                track_uuid, prev_state, args)
            self._stats['thread_slices'] += 1

        for cpu_id, cp in self._cpu_pending.items():
            if cp[0] >= end_ns:
                continue
            if not self._pid_tid_match(cp[1], cp[2]):
                continue
            if not self._in_time_range(end_ns):
                continue
            tname = self._get_thread_name(cp[1], cp[2])
            cpu_args = {
                'process': self._get_process_label(cp[1]),
                'thread': self._get_thread_label(cp[1], cp[2]),
            }
            if self._is_json:
                from ..exporters.json_writer import JsonTraceWriter
                cpid = JsonTraceWriter.cpu_pid()
                self._jw.write_complete(
                    cpid, JsonTraceWriter.cpu_main_tid(cpu_id),
                    cp[0] / 1000, (end_ns - cp[0]) / 1000,
                    tname, 'cpu', cpu_args)
            else:
                from ..exporters.perfetto.utils import write_slice
                cpu_track = self._tracks.get_cpu_track(cpu_id)
                if cpu_track:
                    write_slice(self._builder, cp[0], end_ns,
                                cpu_track, tname, cpu_args)
            self._stats['cpu_running_slices'] += 1
