"""
QST Parser - Comm 类事件处理器

处理 _NTO_TRACE_COMM 类事件（通信相关）:
    - COMM_SMSG: 发送消息
    - COMM_RMSG: 接收消息
    - COMM_REPLY: 回复
    - COMM_ERROR: 错误
    - COMM_SPULSE: 发送脉冲
    - COMM_RPULSE: 接收脉冲
    - COMM_SPULSE_*: 各种脉冲类型
    - COMM_SIGNAL: 信号通信
"""

from .handler import CommClassHandler
from .models import CommEvent

__all__ = [
    "CommClassHandler",
    "CommEvent",
]
