"""
QST Parser - VThread 类事件处理器

处理 _NTO_TRACE_VTHREAD 类事件，虚拟线程状态变化。
复用 Thread 类的解析逻辑，但标记为 is_vthread=True。
"""

from .handler import VThreadClassHandler
from ..thread.models import ThreadEvent  # 复用 ThreadEvent
from ..thread.constants import ThreadState, THREAD_STATES

__all__ = [
    "VThreadClassHandler",
    "ThreadEvent",
    "ThreadState",
    "THREAD_STATES",
]
