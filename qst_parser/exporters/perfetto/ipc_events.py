"""
IPC 事件写入器

根据 TRACE_EVENT_SELECTION_GUIDE.md:
- MSG_SENDV (ENTER + EXIT) - 配对为 Slice
- MSG_RECEIVEV (ENTER + EXIT) - 配对为 Slice
- MSG_REPLYV (ENTER + EXIT) - 配对为 Slice

配对逻辑：按 (pid, tid) 分组，在同一线程内按时间顺序配对 ENTER/EXIT
未配对的事件作为 Instant Events 写入
"""

from typing import List, Dict, Tuple, TYPE_CHECKING
from collections import defaultdict

from .tracks import TrackManager
from .utils import write_slice, write_instant

if TYPE_CHECKING:
    from perfetto.trace_builder.proto_builder import TraceProtoBuilder


class IpcEventWriter:
    """IPC 事件写入器"""
    
    def __init__(self, builder: "TraceProtoBuilder", track_manager: TrackManager):
        self._builder = builder
        self._tracks = track_manager
        self._slice_count = 0
        self._instant_count = 0
        # 丢弃统计
        self._dropped_no_thread = 0  # 找不到对应线程
        self._dropped_no_track = 0   # 找不到对应 Track
        self._total_input = 0        # 输入事件总数
    
    def write_ipc_events(
        self,
        msg_events: List,
        running_map: Dict[int, List[Tuple[int, int, int]]]
    ) -> Tuple[int, int]:
        """
        写入所有 IPC 事件
        
        尝试配对 ENTER/EXIT 生成 Slices，未配对的作为 Instant Events
        
        Args:
            msg_events: 所有消息事件（ENTER + EXIT）
            running_map: CPU -> [(timestamp, pid, tid), ...] 映射
        
        Returns:
            (slice_count, instant_count)
        """
        self._slice_count = 0
        self._instant_count = 0
        self._dropped_no_thread = 0
        self._dropped_no_track = 0
        self._total_input = len(msg_events)
        
        if not msg_events:
            return 0, 0
        
        # 为每个事件推断 pid/tid
        events_with_thread = []
        for e in msg_events:
            pid, tid = self._find_thread(running_map, e.cpu_id, e.timestamp_ns)
            if pid == 0 and tid == 0:
                # 使用 fallback 线程，避免丢弃事件
                pid, tid = self._get_fallback_thread(e.cpu_id)
            events_with_thread.append((e, pid, tid))
        
        # 按事件类型分组处理
        self._process_msg_sendv(events_with_thread)
        self._process_msg_receivev(events_with_thread)
        self._process_msg_replyv(events_with_thread)
        
        # 打印丢弃统计
        total_written = self._slice_count + self._instant_count
        if self._dropped_no_thread > 0 or self._dropped_no_track > 0:
            print(f"[WARN] IPC事件丢弃统计: "
                  f"输入={self._total_input}, 写入slice={self._slice_count}, instant={self._instant_count}, "
                  f"找不到线程={self._dropped_no_thread}, 找不到Track={self._dropped_no_track}")
        
        return self._slice_count, self._instant_count
    
    def _find_thread(
        self, 
        running_map: Dict[int, List[Tuple[int, int, int]]],
        cpu_id: int, 
        timestamp_ns: int
    ) -> Tuple[int, int]:
        """
        根据 cpu_id 和时间戳找到当时正在运行的线程
        
        如果该时间点之前没有 RUNNING 事件（trace 开头），
        则回退到该 CPU 上最早的 RUNNING 事件。
        """
        running_list = running_map.get(cpu_id, [])
        if not running_list:
            return 0, 0
        
        left, right = 0, len(running_list) - 1
        result = None
        
        while left <= right:
            mid = (left + right) // 2
            if running_list[mid][0] <= timestamp_ns:
                result = running_list[mid]
                left = mid + 1
            else:
                right = mid - 1
        
        if result:
            return result[1], result[2]
        
        # 回退：使用该 CPU 上最早的 RUNNING 事件
        return running_list[0][1], running_list[0][2]
    
    def _process_msg_sendv(self, events_with_thread: List):
        """处理 MsgSendv 事件，配对 ENTER/EXIT"""
        # 按 (pid, tid) 分组
        enters_by_thread: Dict[Tuple[int, int], List] = defaultdict(list)
        exits_by_thread: Dict[Tuple[int, int], List] = defaultdict(list)
        
        for (e, pid, tid) in events_with_thread:
            etype = type(e).__name__
            if etype == 'MsgSendvEnterEvent':
                enters_by_thread[(pid, tid)].append(e)
            elif etype == 'MsgSendvExitEvent':
                exits_by_thread[(pid, tid)].append(e)
        
        # 配对并写入
        self._pair_and_write(
            enters_by_thread, exits_by_thread,
            "MsgSend",
            lambda e: {
                "coid": getattr(e, 'coid', 0),
                "sparts": getattr(e, 'sparts', 0),
                "rparts": getattr(e, 'rparts', 0),
                "msg_0": e.msg[0] if hasattr(e, 'msg') and e.msg else 0,
            },
            lambda e: {
                "ret_val": getattr(e, 'ret_val', 0),
                "errno_val": getattr(e, 'errno_val', 0),
            }
        )
    
    def _process_msg_receivev(self, events_with_thread: List):
        """处理 MsgReceivev 事件，配对 ENTER/EXIT"""
        enters_by_thread: Dict[Tuple[int, int], List] = defaultdict(list)
        exits_by_thread: Dict[Tuple[int, int], List] = defaultdict(list)
        
        for (e, pid, tid) in events_with_thread:
            etype = type(e).__name__
            if etype == 'MsgReceivevEnterEvent':
                enters_by_thread[(pid, tid)].append(e)
            elif etype == 'MsgReceivevExitEvent':
                exits_by_thread[(pid, tid)].append(e)
        
        self._pair_and_write(
            enters_by_thread, exits_by_thread,
            "MsgRecv",
            lambda e: {
                "chid": getattr(e, 'chid', 0),
                "rparts": getattr(e, 'rparts', 0),
            },
            lambda e: {
                "rcvid": getattr(e, 'rcvid', 0),
                "ret_val": getattr(e, 'ret_val', 0),
                "info_pid": getattr(e, 'info_pid', 0),
                "info_tid": getattr(e, 'info_tid', 0),
                "info_chid": getattr(e, 'info_chid', 0),
                "info_coid": getattr(e, 'info_coid', 0),
            }
        )
    
    def _process_msg_replyv(self, events_with_thread: List):
        """处理 MsgReplyv 事件，配对 ENTER/EXIT"""
        enters_by_thread: Dict[Tuple[int, int], List] = defaultdict(list)
        exits_by_thread: Dict[Tuple[int, int], List] = defaultdict(list)
        
        for (e, pid, tid) in events_with_thread:
            etype = type(e).__name__
            if etype == 'MsgReplyvEnterEvent':
                enters_by_thread[(pid, tid)].append(e)
            elif etype == 'MsgReplyvExitEvent':
                exits_by_thread[(pid, tid)].append(e)
        
        self._pair_and_write(
            enters_by_thread, exits_by_thread,
            "MsgReply",
            lambda e: {
                "rcvid": getattr(e, 'rcvid', 0),
                "sparts": getattr(e, 'sparts', 0),
                "status": getattr(e, 'status', 0),
            },
            lambda e: {
                "ret_val": getattr(e, 'ret_val', 0),
                "errno_val": getattr(e, 'errno_val', 0),
            }
        )
    
    def _pair_and_write(
        self,
        enters_by_thread: Dict[Tuple[int, int], List],
        exits_by_thread: Dict[Tuple[int, int], List],
        event_name: str,
        get_enter_args,
        get_exit_args
    ):
        """
        配对 ENTER/EXIT 并写入
        
        配对规则：在同一线程内，按时间顺序，ENTER 和之后最近的 EXIT 配对
        """
        all_threads = set(enters_by_thread.keys()) | set(exits_by_thread.keys())
        
        for (pid, tid) in all_threads:
            track_uuid = self._tracks.get_thread_ipc_track(pid, tid)
            if not track_uuid:
                continue
            
            # 使用格式化的名称
            thread_name = self._tracks.format_thread_name(pid, tid)
            
            enters = sorted(enters_by_thread.get((pid, tid), []), 
                           key=lambda e: e.timestamp_ns)
            exits = sorted(exits_by_thread.get((pid, tid), []), 
                          key=lambda e: e.timestamp_ns)
            
            enter_idx = 0
            exit_idx = 0
            
            while enter_idx < len(enters):
                enter = enters[enter_idx]
                
                # 找到时间戳大于 ENTER 的最近 EXIT
                matched_exit = None
                while exit_idx < len(exits):
                    exit_e = exits[exit_idx]
                    if exit_e.timestamp_ns > enter.timestamp_ns:
                        matched_exit = exit_e
                        exit_idx += 1
                        break
                    exit_idx += 1
                
                if matched_exit:
                    # 生成 slice
                    args = {
                        "thread": thread_name,
                        "cpu_enter": enter.cpu_id,
                        "cpu_exit": matched_exit.cpu_id,
                        **get_enter_args(enter),
                        **get_exit_args(matched_exit),
                    }
                    
                    write_slice(
                        self._builder,
                        enter.timestamp_ns,
                        matched_exit.timestamp_ns,
                        track_uuid,
                        event_name,
                        args
                    )
                    self._slice_count += 1
                else:
                    # 无法配对，写入 ENTER 作为 instant
                    args = {
                        "type": "ENTER",
                        "thread": thread_name,
                        "cpu": enter.cpu_id,
                        **get_enter_args(enter),
                    }
                    write_instant(self._builder, enter.timestamp_ns, track_uuid, 
                                  f"{event_name}_ENTER", args)
                    self._instant_count += 1
                
                enter_idx += 1
            
            # 处理剩余未配对的 EXIT
            while exit_idx < len(exits):
                exit_e = exits[exit_idx]
                args = {
                    "type": "EXIT",
                    "thread": thread_name,
                    "cpu": exit_e.cpu_id,
                    **get_exit_args(exit_e),
                }
                write_instant(self._builder, exit_e.timestamp_ns, track_uuid, 
                              f"{event_name}_EXIT", args)
                self._instant_count += 1
                exit_idx += 1
    
    def _get_fallback_thread(self, cpu_id: int) -> Tuple[int, int]:
        """
        当无法推断线程时，返回一个 fallback 的 (pid, tid)
        
        使用一个虚拟进程 (pid=0xFFFF) 来收纳无法归属的事件，
        tid 使用 cpu_id+1 来区分不同 CPU 上的未知线程。
        """
        fallback_pid = 0xFFFF
        fallback_tid = cpu_id + 1
        return fallback_pid, fallback_tid
    
    # =========================================================================
    # 兼容旧接口
    # =========================================================================
    
    def write_all_ipc_events_as_instant(
        self,
        msg_events: List,
        running_map: Dict[int, List[Tuple[int, int, int]]]
    ) -> Tuple[int, int]:
        """兼容旧接口"""
        return self.write_ipc_events(msg_events, running_map)
    
    def write_paired_ipc_events(
        self, 
        paired_events: Dict,
        instant_events: List
    ) -> Tuple[int, int]:
        """[已废弃]"""
        return 0, 0
    
    @property
    def enter_count(self) -> int:
        return self._slice_count
    
    @property
    def exit_count(self) -> int:
        return self._instant_count
    
    @property
    def slice_count(self) -> int:
        return self._slice_count
    
    @property
    def instant_count(self) -> int:
        return self._instant_count
