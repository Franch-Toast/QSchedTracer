"""
QST Parser - QVM 类事件处理器

处理 _NTO_TRACE_QVM 类事件 (Hypervisor/QVM)

QVM 是 QNX 的虚拟化解决方案，用于追踪虚拟机相关事件。
事件编号范围: 0x000 - 0x3ff

事件类型:
    - GUEST_ENTER: 进入 guest
    - GUEST_EXIT: 退出 guest
    - CREATE_VCPU_THREAD: 创建 VCPU 线程
    - RAISE_INTR: 触发中断
    - LOWER_INTR: 降低中断
    - TIMER_CREATE: 创建定时器
    - TIMER_FIRE: 定时器触发
    - CYCLES: cycles 事件
"""

from .handler import QvmClassHandler
from .models import QvmEvent
from .constants import QvmEventType

__all__ = [
    "QvmClassHandler",
    "QvmEvent",
    "QvmEventType",
]
