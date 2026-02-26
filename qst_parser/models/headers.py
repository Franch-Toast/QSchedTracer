"""
QST Parser - 文件头数据模型

定义 QST 文件的各种头部结构。
"""

import struct
from dataclasses import dataclass
from datetime import datetime, timezone
from typing import Optional

from ..constants import (
    FILE_HEADER_SIZE,
    PROCINFO_HEADER_SIZE,
    MAINDATA_HEADER_SIZE,
    QST_MAGIC,
    QST_VERSION,
    PROCINFO_MAGIC,
    MAINDATA_MAGIC,
)


@dataclass
class FileHeader:
    """
    QST 文件头 (v3, 64 字节)
    
    布局:
        - magic: 4 bytes ('QST3')
        - version: 4 bytes
        - clock_freq: 8 bytes (时钟频率 Hz)
        - wallclock_sec: 8 bytes (采集结束时的秒数)
        - wallclock_nsec: 8 bytes (采集结束时的纳秒数)
        - sync_cycles: 4 bytes (同步时的 cycles)
        - reserved: 28 bytes
    """
    magic: int = 0
    version: int = 0
    clock_freq: int = 0
    wallclock_sec: int = 0
    wallclock_nsec: int = 0
    sync_cycles: int = 0
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'FileHeader':
        """从字节数据解析文件头"""
        if len(data) < FILE_HEADER_SIZE:
            raise ValueError(f"文件头数据太小: {len(data)} < {FILE_HEADER_SIZE}")
        
        # 解析: magic(4) + version(4) + clock_freq(8) + wallclock_sec(8) + wallclock_nsec(8) + sync_cycles(4)
        magic, version, clock_freq, wallclock_sec, wallclock_nsec, sync_cycles = \
            struct.unpack('<IIQqqI', data[:36])
        
        if magic != QST_MAGIC:
            raise ValueError(f"无效的文件魔数: 0x{magic:08X} (期望 0x{QST_MAGIC:08X})")
        
        if version != QST_VERSION:
            raise ValueError(f"不支持的文件版本: {version} (仅支持 v{QST_VERSION})")
        
        return cls(
            magic=magic,
            version=version,
            clock_freq=clock_freq,
            wallclock_sec=wallclock_sec,
            wallclock_nsec=wallclock_nsec,
            sync_cycles=sync_cycles,
        )
    
    def get_wallclock_ns(self) -> int:
        """获取 wallclock 纳秒时间戳"""
        return self.wallclock_sec * 1_000_000_000 + self.wallclock_nsec
    
    def get_wallclock_datetime(self) -> datetime:
        """获取 wallclock 日期时间"""
        return datetime.fromtimestamp(
            self.wallclock_sec + self.wallclock_nsec / 1e9,
            tz=timezone.utc
        )


@dataclass
class ProcInfoHeader:
    """
    进程/线程信息头 (16 字节)
    
    布局:
        - magic: 4 bytes ('PINF')
        - event_count: 4 bytes
        - reserved: 8 bytes
    """
    magic: int = 0
    event_count: int = 0
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'ProcInfoHeader':
        """从字节数据解析 ProcInfoHeader"""
        if len(data) < PROCINFO_HEADER_SIZE:
            raise ValueError(f"ProcInfoHeader 数据太小")
        
        magic, event_count, reserved = struct.unpack('<IIQ', data[:16])
        
        if magic != PROCINFO_MAGIC:
            raise ValueError(f"无效的 ProcInfoHeader 魔数: 0x{magic:08X}")
        
        return cls(magic=magic, event_count=event_count)


@dataclass
class MainDataHeader:
    """
    主调度数据头 (16 字节)
    
    布局:
        - magic: 4 bytes ('MAIN')
        - event_count: 4 bytes
        - reserved: 8 bytes
    """
    magic: int = 0
    event_count: int = 0
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'MainDataHeader':
        """从字节数据解析 MainDataHeader"""
        if len(data) < MAINDATA_HEADER_SIZE:
            raise ValueError(f"MainDataHeader 数据太小")
        
        magic, event_count, reserved = struct.unpack('<IIQ', data[:16])
        
        if magic != MAINDATA_MAGIC:
            raise ValueError(f"无效的 MainDataHeader 魔数: 0x{magic:08X}")
        
        return cls(magic=magic, event_count=event_count)
