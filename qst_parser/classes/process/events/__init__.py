"""
QST Parser - Process 类事件解析器

每个进程事件都有独立的解析器，支持 Fast 和 Wide 模式。

事件列表:
    - proccreate.py: PROCCREATE, PROCCREATE_NAME
    - procdestroy.py: PROCDESTROY, PROCDESTROY_NAME
    - procthread_name.py: PROCTHREAD_NAME
"""

from .proccreate import ProcCreateEventParser
from .procdestroy import ProcDestroyEventParser
from .procthread_name import ProcThreadNameEventParser

__all__ = [
    "ProcCreateEventParser",
    "ProcDestroyEventParser",
    "ProcThreadNameEventParser",
]
