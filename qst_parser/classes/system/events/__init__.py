"""
QST Parser - System 类事件解析器

事件类型:
- mmap: 内存映射事件 (MMAP/MUNMAP)
- func_trace: 函数跟踪事件 (FUNC_ENTER/FUNC_EXIT)
- ipi: 处理器间中断事件 (IPI/IPI_64)
- pagewait: 页面等待事件 (PAGEWAIT)
"""

from .mmap import MmapEventParser
from .func_trace import FuncTraceEventParser
from .ipi import IPIEventParser
from .pagewait import PageWaitEventParser

__all__ = [
    "MmapEventParser",
    "FuncTraceEventParser",
    "IPIEventParser",
    "PageWaitEventParser",
]
