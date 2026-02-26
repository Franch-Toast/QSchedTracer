"""
QST Parser - SEC 类数据模型

安全事件数据结构。
"""

from dataclasses import dataclass
from ...models.base import BaseEvent


@dataclass
class SecEvent(BaseEvent):
    """
    安全事件基类
    """
    event_type: int = 0     # SecEventType
    event_type_name: str = ""


@dataclass
class SecAbleEvent(BaseEvent):
    """
    能力检查事件 (SEC_ABLE / SEC_ABLE_LOOKUP)
    
    数据格式:
        - ability: 能力编号
        - flags: 检查标志 (SecAbleFlags)
        - result: 检查结果
    """
    ability: int = 0
    flags: int = 0
    result: int = 0


@dataclass
class SecPathAttachEvent(BaseEvent):
    """
    路径附加事件 (SEC_PATH_ATTACH)
    
    数据格式:
        - status: 附加状态 (_TRACE_PATH_ATTACH_STATUS)
        - path: 路径字符串 (组合事件)
    """
    status: int = 0
    path: str = ""


@dataclass
class SecQnetConnectEvent(BaseEvent):
    """
    Qnet 连接事件 (SEC_QNET_CONNECT)
    
    数据格式:
        - status: 连接状态 (_TRACE_QNET_CONNECT_STATUS)
    """
    status: int = 0
