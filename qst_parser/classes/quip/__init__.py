"""
QST Parser - QUIP 类事件处理器

处理 _NTO_TRACE_QUIP 类事件 (QNX Unified Instrumentation Platform)

QUIP 是 QNX 的统一检测平台，用于追踪用户定义的事件。
事件编号范围: 0x000 - 0x3ff (1024 个事件)
"""

from .handler import QuipClassHandler
from .models import QuipEvent

__all__ = [
    "QuipClassHandler",
    "QuipEvent",
]
