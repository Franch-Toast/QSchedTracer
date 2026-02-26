"""
QST Parser - 基础事件模型

定义原始事件和所有事件的基类。
"""

import struct
from abc import ABC
from dataclasses import dataclass
from typing import Tuple


@dataclass
class RawEvent:
    """
    原始事件 (16 字节)
    
    事件头布局 (32 bits):
        - bits [0:9]: internal_event (10 bits)
        - bits [10:14]: internal_class (5 bits)
        - bits [15:23]: reserved
        - bits [24:29]: cpu_id (6 bits)
        - bits [30:31]: struct_type (2 bits)
    
    数据部分:
        - data[0]: 通常是 timestamp (cycles)
        - data[1], data[2]: 事件特定数据
    """
    header: int = 0
    data: Tuple[int, int, int] = (0, 0, 0)
    cpu_id: int = 0
    struct_type: int = 0
    internal_class: int = 0
    internal_event: int = 0
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'RawEvent':
        """从 16 字节数据解析原始事件"""
        header, d0, d1, d2 = struct.unpack('<IIII', data[:16])
        
        # 解析事件头
        cpu_id = (header >> 24) & 0x3F
        struct_type = (header >> 30) & 0x3
        internal_class = (header >> 10) & 0x1F
        internal_event = header & 0x3FF
        
        return cls(
            header=header,
            data=(d0, d1, d2),
            cpu_id=cpu_id,
            struct_type=struct_type,
            internal_class=internal_class,
            internal_event=internal_event,
        )


@dataclass
class BaseEvent(ABC):
    """
    所有解析后事件的基类
    
    定义了事件的基本属性。
    """
    timestamp_cycles: int       # 原始 32 位时间戳 (cycles)
    timestamp_ns: int = 0       # 计算后的 UNIX 纳秒时间戳
    cpu_id: int = 0             # CPU ID
