"""
QST Parser - Control 类常量定义

控制事件类型常量。
"""

from enum import IntEnum


class ControlEventType(IntEnum):
    """控制事件类型"""
    CONTROLTIME = 1     # 时间同步
    CONTROLBUFFER = 2   # 缓冲区信息
