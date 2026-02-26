"""
QST Parser - KerCall 类事件处理器

处理 _NTO_TRACE_KERCALL* 类事件（内核调用）:
    - _NTO_TRACE_KERCALLENTER: 内核调用进入
    - _NTO_TRACE_KERCALLEXIT: 内核调用退出
    - _NTO_TRACE_KERCALLINT: 内核调用中断

事件号范围区分（参考 samples_parser.html）:
    - 0-127: KERCALLENTER
    - 128-255: KERCALLEXIT
    - 256-383: KERCALLINT

支持 Fast mode 和 Wide mode 的解析。

目录结构:
    - constants.py: 内核调用编号常量
    - models.py: 内核调用事件数据模型
    - handler.py: 类处理器
    - events/: 各个内核调用的解析器
"""

from .handler import KerCallClassHandler
from .models import (
    SignalKillEvent, 
    KerCallEvent,
    KerCallBaseEvent,
    KerCallEnterEvent,
    KerCallExitEvent,
    KerCallIntEvent,
)
from .constants import KernelCall, SIGNAL_NAMES

__all__ = [
    "KerCallClassHandler",
    "SignalKillEvent",
    "KerCallEvent",
    "KerCallBaseEvent",
    "KerCallEnterEvent",
    "KerCallExitEvent",
    "KerCallIntEvent",
    "KernelCall",
    "SIGNAL_NAMES",
]
