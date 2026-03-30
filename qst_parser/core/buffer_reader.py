"""
Layer 1: Buffer Reader

Reads QST v4 file structure:
- FileHeader (64B)
- DATA section: scan tracebuf_t metadata (seq, num_events, file_offset)
- PINF section: parse process/thread names
"""

import struct
from typing import List, Tuple, Dict, NamedTuple

from ..constants import (
    FILE_HEADER_SIZE, SECTION_HEADER_SIZE,
    DATA_MAGIC, PINF_MAGIC,
    TRACEBUF_OFFSET_NUM_EVENTS,
    InternalClass, StructType,
)
from ..models.headers import QstFileHeader, SectionHeader
from .file_access import FileAccess


class BufferMeta(NamedTuple):
    """Metadata for one tracebuf_t block (12 bytes in memory)."""
    seq: int
    num_events: int
    file_offset: int  # offset to start of event data within the file


class BufferReader:
    """
    Reads QST v4 file structure and provides buffer metadata + PINF names.

    Usage:
        reader = BufferReader(file_access)
        reader.read_structure()
        # Now access: reader.header, reader.sorted_buffers, reader.process_names, ...
    """

    def __init__(self, fa: FileAccess, verbose: bool = False):
        self._fa = fa
        self._verbose = verbose

        self.header: QstFileHeader = None
        self.data_entry_count: int = 0
        self.sorted_buffers: List[BufferMeta] = []
        self.process_names: Dict[int, str] = {}
        self.thread_names: Dict[Tuple[int, int], str] = {}

        self._data_payload_offset: int = 0
        self._tb_size: int = 0
        self._data_off: int = 0
        self._pinf_combine: Dict[str, dict] = {}
        self.cpu_groups: List[List[BufferMeta]] = []

    def read_structure(self):
        """Read the complete file structure: FileHeader, DATA metadata, PINF names."""
        self._read_file_header()
        self._scan_data_metadata()
        self._read_pinf()
        self._sort_buffers()

    def _read_file_header(self):
        data = self._fa.read_at(0, FILE_HEADER_SIZE)
        self.header = QstFileHeader.from_bytes(data)
        self._tb_size = self.header.tracebuf_size
        self._data_off = self.header.data_offset

        if self._tb_size == 0 or self._data_off == 0:
            raise ValueError(
                f"Invalid tracebuf layout: tracebuf_size={self._tb_size}, "
                f"data_offset={self._data_off}. This file may use the old v4 format."
            )

        if self._verbose:
            h = self.header
            print(f"[BufferReader] FileHeader: os={h.os_version}, cpus={h.num_cpus}, "
                  f"freq={h.clock_freq}, tb_size={self._tb_size}, data_off={self._data_off}")

    def _scan_data_metadata(self):
        """Scan DATA section: read only (seq, num_events) per tracebuf_t block."""
        pos = FILE_HEADER_SIZE
        sh_data = self._fa.read_at(pos, SECTION_HEADER_SIZE)
        data_sh = SectionHeader.from_bytes(sh_data, expected_magic=DATA_MAGIC)
        self.data_entry_count = data_sh.entry_count

        self._data_payload_offset = pos + SECTION_HEADER_SIZE
        buffers = []

        ne_seq_fmt = struct.Struct('<II')  # num_events(+20), seq(+24)

        for i in range(data_sh.entry_count):
            block_start = self._data_payload_offset + i * self._tb_size
            ne, seq = self._fa.unpack_at(
                ne_seq_fmt, block_start + TRACEBUF_OFFSET_NUM_EVENTS
            )
            if ne == 0:
                continue
            events_offset = block_start + self._data_off
            buffers.append(BufferMeta(seq=seq, num_events=ne, file_offset=events_offset))

        self.sorted_buffers = buffers

        if self._verbose:
            print(f"[BufferReader] DATA section: {data_sh.entry_count} blocks, "
                  f"{len(buffers)} valid")

    def _read_pinf(self):
        """Read PINF section and extract process/thread names."""
        pinf_offset = (self._data_payload_offset +
                       self.data_entry_count * self._tb_size)

        sh_data = self._fa.read_at(pinf_offset, SECTION_HEADER_SIZE)
        pinf_sh = SectionHeader.from_bytes(sh_data, expected_magic=PINF_MAGIC)

        payload_start = pinf_offset + SECTION_HEADER_SIZE
        event_fmt = struct.Struct('<IIII')

        MAX_TH_STATE_NUM = 26

        for i in range(pinf_sh.entry_count):
            block_start = payload_start + i * self._tb_size
            ne = self._fa.uint32_at(block_start + TRACEBUF_OFFSET_NUM_EVENTS)
            if ne == 0:
                continue
            ev_base = block_start + self._data_off
            ev_data = self._fa.read_at(ev_base, min(ne * 16, self._tb_size - self._data_off))

            for header, d0, d1, d2 in event_fmt.iter_unpack(ev_data[:ne * 16]):
                int_class = (header >> 10) & 0x1F
                if int_class != InternalClass.PR_TH:
                    continue
                int_event = header & 0x3FF
                struct_type = (header >> 30) & 0x3

                if int_event >= 2 * MAX_TH_STATE_NUM:
                    self._extract_process_name(header, d0, d1, d2, struct_type)
                elif int_event < MAX_TH_STATE_NUM:
                    pass  # thread state events in PINF are not useful for names

        if self._verbose:
            print(f"[BufferReader] PINF: {len(self.process_names)} processes, "
                  f"{len(self.thread_names)} threads named")

    def _extract_process_name(self, header: int, d0: int, d1: int, d2: int,
                              struct_type: int):
        """Extract process/thread names from PROCESS class events in PINF."""
        int_event = header & 0x3FF
        cpu_id = (header >> 24) & 0x3F
        shift = (int_event >> 6)
        if shift < 1:
            return
        ext_event = 1 << (shift - 1)

        PROCCREATE_NAME = 4   # _NTO_TRACE_PROCCREATE_NAME = 0x04
        PROCTHREAD_NAME = 16  # _NTO_TRACE_PROCTHREAD_NAME = 0x10

        if ext_event not in (PROCCREATE_NAME, PROCTHREAD_NAME):
            return

        key = f"pinf_{cpu_id}"

        if struct_type == StructType.COMBINE_BEGIN:
            if ext_event == PROCCREATE_NAME:
                self._pinf_combine[key] = {
                    'type': 'proc', 'pid': d2, 'name_parts': [],
                }
            else:
                self._pinf_combine[key] = {
                    'type': 'thread', 'pid': d1, 'tid': d2,
                    'name_parts': [],
                }
        elif struct_type in (StructType.COMBINE_CONT, StructType.COMBINE_END):
            buf = self._pinf_combine.get(key)
            if buf is None:
                return

            buf['name_parts'].append(struct.pack('<II', d1, d2))

            if struct_type == StructType.COMBINE_END:
                raw_name = b''.join(buf['name_parts'])
                try:
                    name = raw_name.split(b'\x00')[0].decode(
                        'utf-8', errors='replace').strip()
                    name = ''.join(c for c in name if c >= ' ' or c == '\t')
                except Exception:
                    name = "unknown"

                if buf['type'] == 'proc':
                    self.process_names[buf['pid']] = name
                else:
                    self.thread_names[(buf['pid'], buf['tid'])] = name

                del self._pinf_combine[key]

    def _sort_buffers(self):
        """Sort buffer metadata by seq_buff_num."""
        h = self.header
        if h.os_version == 800 and h.bufs_per_cpu > 0:
            self._sort_per_cpu()
        else:
            self.sorted_buffers.sort(key=lambda b: b.seq)

    def _sort_per_cpu(self):
        """QNX 8.0: group by CPU partition, sort within each group."""
        K = self.header.bufs_per_cpu
        num_cpus = self.header.num_cpus
        all_buffers = self.sorted_buffers

        groups = [[] for _ in range(num_cpus)]
        for buf in all_buffers:
            rel_idx = (buf.file_offset - self._data_payload_offset -
                       self._data_off) // self._tb_size
            cpu = rel_idx // K if K > 0 else 0
            if 0 <= cpu < num_cpus:
                groups[cpu].append(buf)

        for g in groups:
            g.sort(key=lambda b: b.seq)

        self.sorted_buffers = []
        self.cpu_groups = groups
        for g in groups:
            self.sorted_buffers.extend(g)

    def get_cpu_groups(self) -> List[List[BufferMeta]]:
        """Get per-CPU buffer groups (QNX 8.0). Falls back to single group for 7.1."""
        if self.cpu_groups:
            return self.cpu_groups
        return [self.sorted_buffers]
