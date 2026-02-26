"""
QST Parser - Process 类事件处理器

处理 _NTO_TRACE_PROCESS 类事件，包括:
    - PROCCREATE: 进程创建
    - PROCCREATE_NAME: 进程创建（带名称）
    - PROCDESTROY: 进程销毁
    - PROCDESTROY_NAME: 进程销毁（带名称）
    - PROCTHREAD_NAME: 线程命名

目录结构:
    - constants.py: 进程事件类型常量
    - models.py: 进程事件数据模型
    - handler.py: 类处理器
    - events/: 各个进程事件的解析器
"""

from .handler import ProcessClassHandler
from .constants import ProcessEventType

__all__ = [
    "ProcessClassHandler",
    "ProcessEventType",
]
