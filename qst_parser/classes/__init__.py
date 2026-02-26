"""
QST Parser - 事件类别处理器

按 QNX trace event 类别组织的解析器模块:
    - thread: _NTO_TRACE_THREAD 线程状态事件
    - vthread: _NTO_TRACE_VTHREAD 虚拟线程状态事件
    - process: _NTO_TRACE_PROCESS 进程事件
    - kercall: _NTO_TRACE_KERCALL* 内核调用事件
    - control: _NTO_TRACE_CONTROL 控制事件
    - interrupt: _NTO_TRACE_INT* 中断事件
    - comm: _NTO_TRACE_COMM 通信事件
    - system: _NTO_TRACE_SYSTEM 系统事件
    - user: _NTO_TRACE_USER 用户自定义事件
    - quip: _NTO_TRACE_QUIP QNX Unified Instrumentation Platform
    - sec: _NTO_TRACE_SEC 安全事件
    - qvm: _NTO_TRACE_QVM 虚拟化/Hypervisor 事件
"""

from .thread import ThreadClassHandler
from .vthread import VThreadClassHandler
from .process import ProcessClassHandler
from .kercall import KerCallClassHandler
from .control import ControlClassHandler
from .interrupt import InterruptClassHandler
from .comm import CommClassHandler
from .system import SystemClassHandler
from .quip import QuipClassHandler
from .sec import SecClassHandler
from .qvm import QvmClassHandler

__all__ = [
    "ThreadClassHandler",
    "VThreadClassHandler",
    "ProcessClassHandler",
    "KerCallClassHandler",
    "ControlClassHandler",
    "InterruptClassHandler",
    "CommClassHandler",
    "SystemClassHandler",
    "QuipClassHandler",
    "SecClassHandler",
    "QvmClassHandler",
]
