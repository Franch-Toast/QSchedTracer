"""
QST Parser - QUIP 类数据模型

QUIP (QNX Unified Instrumentation Platform) 事件数据结构。
"""

from dataclasses import dataclass
from ...models.base import BaseEvent


@dataclass
class QuipEvent(BaseEvent):
    """
    QUIP 事件
    
    QUIP 事件用于用户自定义的追踪点，数据格式取决于具体的事件类型。
    
    通用格式:
        Fast: event_id, data1, data2
        Wide: event_id, data1, data2, ... (组合事件)
    """
    event_id: int = 0       # QUIP 事件编号
    data1: int = 0          # 事件数据 1
    data2: int = 0          # 事件数据 2
    extra_data: tuple = ()  # Wide mode 额外数据
