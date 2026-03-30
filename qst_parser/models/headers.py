"""
QST Parser - 文件头数据模型 (v4)

定义 QST v4 文件的各种头部结构。

支持两种 v4 变体:
  旧 v4: QstFileHeader(64B) → KBUF Section → MAIN Section → PINF Section
  新 v4: QstFileHeader(64B) → DATA Section → PINF Section

通过 peek 第一个 Section 的 magic 区分:
  - KBUF (0x4B425546): 旧 v4, KBUF + MAIN + PINF
  - DATA (0x44415441): 新 v4, raw tracebuf_t blocks
"""

import struct
from dataclasses import dataclass, field
from datetime import datetime, timezone
from typing import List, Optional

from ..constants import (
    FILE_HEADER_SIZE,
    SECTION_HEADER_SIZE,
    KBUF_ENTRY_SIZE,
    QST_MAGIC,
    QST_VERSION,
    KBUF_MAGIC,
    MAIN_MAGIC,
    DATA_MAGIC,
    PINF_MAGIC,
)


@dataclass
class QstFileHeader:
    """
    QST v4 文件头 (64 字节)
    
    布局 (little-endian, packed):
        magic:              4B  'QST4' (0x51535434)
        version:            4B  4
        header_size:        4B  64
        flags:              4B  bit0: endianness (0=LE)
        clock_freq:         8B  ClockCycles() 频率 (Hz)
        capture_start_ns:   8B  采集开始 wall clock (epoch nanoseconds, signed)
        capture_end_ns:     8B  采集结束 wall clock (epoch nanoseconds, signed)
        num_cpus:           4B
        os_version:         4B  710 or 800
        tracebuf_size:      4B  sizeof(tracebuf_t) (新 v4), 0 (旧 v4)
        data_offset:        4B  offsetof(tracebuf_t, data) (新 v4), 0 (旧 v4)
        bufs_per_cpu:       4B  QNX 8.0 每 CPU buffer 数 (reserved[0..3])
        reserved_tail:      4B  保留
    """
    magic: int = 0
    version: int = 0
    header_size: int = 0
    flags: int = 0
    clock_freq: int = 0
    capture_start_ns: int = 0
    capture_end_ns: int = 0
    num_cpus: int = 0
    os_version: int = 0
    tracebuf_size: int = 0
    data_offset: int = 0
    bufs_per_cpu: int = 0
    
    # 4I Q 2q 2I 2I I 4x = 16+8+16+8+8+4+4 = 64 bytes
    _FMT = '<IIIIQqqIIIII4x'
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'QstFileHeader':
        if len(data) < FILE_HEADER_SIZE:
            raise ValueError(f"文件头数据太小: {len(data)} < {FILE_HEADER_SIZE}")
        
        vals = struct.unpack(cls._FMT, data[:FILE_HEADER_SIZE])
        (magic, version, header_size, flags,
         clock_freq,
         capture_start_ns, capture_end_ns,
         num_cpus, os_version,
         tracebuf_size, data_offset,
         bufs_per_cpu) = vals
        
        if magic != QST_MAGIC:
            raise ValueError(f"无效的文件魔数: 0x{magic:08X} (期望 0x{QST_MAGIC:08X} 'QST4')")
        
        if version != QST_VERSION:
            raise ValueError(f"不支持的文件版本: {version} (仅支持 v{QST_VERSION})")
        
        return cls(
            magic=magic, version=version, header_size=header_size, flags=flags,
            clock_freq=clock_freq,
            capture_start_ns=capture_start_ns, capture_end_ns=capture_end_ns,
            num_cpus=num_cpus, os_version=os_version,
            tracebuf_size=tracebuf_size, data_offset=data_offset,
            bufs_per_cpu=bufs_per_cpu,
        )
    
    def get_capture_end_ns(self) -> int:
        """获取采集结束的 wallclock 纳秒时间戳 (用于反向时间戳推算锚点)"""
        return self.capture_end_ns
    
    def get_capture_start_ns(self) -> int:
        """获取采集开始的 wallclock 纳秒时间戳"""
        return self.capture_start_ns
    
    def get_capture_end_datetime(self) -> datetime:
        return datetime.fromtimestamp(
            self.capture_end_ns / 1e9,
            tz=timezone.utc
        )
    
    def get_capture_start_datetime(self) -> datetime:
        return datetime.fromtimestamp(
            self.capture_start_ns / 1e9,
            tz=timezone.utc
        )


@dataclass
class SectionHeader:
    """
    Section 通用头 (24 字节)
    
    布局:
        magic:          4B
        version:        4B  (section version, 当前 = 1)
        payload_size:   8B  payload 字节数 (不含本 header)
        entry_count:    4B
        reserved:       4B
    """
    magic: int = 0
    version: int = 0
    payload_size: int = 0
    entry_count: int = 0
    
    _FMT = '<IIQII'
    
    @classmethod
    def from_bytes(cls, data: bytes, expected_magic: Optional[int] = None) -> 'SectionHeader':
        if len(data) < SECTION_HEADER_SIZE:
            raise ValueError(f"SectionHeader 数据太小: {len(data)} < {SECTION_HEADER_SIZE}")
        
        magic, version, payload_size, entry_count, _ = struct.unpack(cls._FMT, data[:SECTION_HEADER_SIZE])
        
        if expected_magic is not None and magic != expected_magic:
            raise ValueError(
                f"Section 魔数不匹配: 0x{magic:08X} (期望 0x{expected_magic:08X})")
        
        return cls(magic=magic, version=version, payload_size=payload_size, entry_count=entry_count)


@dataclass
class KBufEntry:
    """
    KBUF Section 条目 (24 字节): 每个有效内核 buffer 的元数据
    
    布局:
        buf_index:      4B  内核数组中的索引
        seq_buff_num:   4B  buffer 填充序号 (单调递增)
        num_events:     4B  该 buffer 中的事件数
        flags:          4B  buffer 标志 (overrun 等)
        cpu_id:         4B  CPU ID (7.1 填 0xFF)
        reserved:       4B
    """
    buf_index: int = 0
    seq_buff_num: int = 0
    num_events: int = 0
    flags: int = 0
    cpu_id: int = 0xFF
    
    _FMT = '<IIIIII'
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'KBufEntry':
        if len(data) < KBUF_ENTRY_SIZE:
            raise ValueError(f"KBufEntry 数据太小")
        
        buf_index, seq_buff_num, num_events, flags, cpu_id, _ = struct.unpack(
            cls._FMT, data[:KBUF_ENTRY_SIZE])
        
        return cls(
            buf_index=buf_index, seq_buff_num=seq_buff_num,
            num_events=num_events, flags=flags, cpu_id=cpu_id)
