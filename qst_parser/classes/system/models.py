"""
QST Parser - System 类数据模型

系统事件的数据结构定义。
"""

from dataclasses import dataclass
from ...models.base import BaseEvent


@dataclass
class SystemEvent(BaseEvent):
    """
    系统事件基类
    """
    event_type: str = ""


@dataclass
class MmapEvent(BaseEvent):
    """
    内存映射事件 (SYS_MMAP / SYS_MUNMAP)
    
    MMAP:
        Fast: pid, addr (64), len (64), flags
        Wide: pid, addr (64), len (64), flags, prot, fd, align (64), offset (64), name
    
    MUNMAP:
        Fast: pid, addr (64), len (64)
        Wide: pid, addr (64), len (64)
    """
    event_type: str = ""
    pid: int = 0
    addr: int = 0
    length: int = 0
    flags: int = 0
    prot: int = 0
    fd: int = 0
    align: int = 0
    offset: int = 0
    name: str = ""


@dataclass
class FuncTraceEvent(BaseEvent):
    """
    函数跟踪事件 (SYS_FUNC_ENTER / SYS_FUNC_EXIT)
    
    Fast: thisfn (32/64), call_site (32/64)
    Wide: thisfn (32/64), call_site (32/64)
    """
    event_type: str = ""  # enter/exit
    thisfn: int = 0       # 当前函数地址
    call_site: int = 0    # 调用位置


@dataclass
class IPIEvent(BaseEvent):
    """
    处理器间中断事件 (SYS_IPI / SYS_IPI_64)
    
    SYS_IPI (32-bit):
        Fast: ipicmd (32), ip (32), tid (32), pid (32)
        Wide: same
        
    SYS_IPI_64 (64-bit):
        Fast: N/A
        Wide: ipicmd (32), pad (32), interrupted ip (64), tid (32), pid (32)
    
    用途:
    - 跟踪处理器间中断（如跨核调度触发）
    - ipicmd 是 IPI 命令类型
    - ip 是被中断时的指令指针
    """
    ipicmd: int = 0          # IPI 命令
    ip: int = 0              # 被中断时的 IP
    tid: int = 0             # 被中断的线程 ID
    pid: int = 0             # 被中断的进程 ID
    is_64bit: bool = False   # 是否为 64 位模式
    is_wide: bool = False    # 是否为 Wide 模式


@dataclass
class ProfileEvent(BaseEvent):
    """
    统计分析事件 (SYS_PROFILE)
    
    Fast: ip (32/64), tid (32), pid (32)
    Wide: same
    """
    ip: int = 0
    tid: int = 0
    pid: int = 0


@dataclass
class PageWaitEvent(BaseEvent):
    """
    页面等待事件 (SYS_PAGEWAIT)
    
    Fast: pid (32), tid (32), ip (32), vaddr (32)
    Wide: pid (32), tid (32), ip (32), vaddr (32), fault_type (32), mmap_flags (32),
          object_offset (64), object_name (string)
    
    用途:
    - 跟踪页面错误处理过程中的等待
    - 可用于分析内存 I/O 引起的调度延迟
    """
    pid: int = 0             # 进程 ID
    tid: int = 0             # 线程 ID
    ip: int = 0              # 发生页面错误时的指令指针
    vaddr: int = 0           # 引起页面错误的虚拟地址
    fault_type: int = 0      # 错误类型 (Wide mode)
    mmap_flags: int = 0      # mmap 标志 (Wide mode)
    object_offset: int = 0   # 对象偏移 (Wide mode)
    object_name: str = ""    # 对象名称 (Wide mode)
    is_wide: bool = False


@dataclass
class TimerEvent(BaseEvent):
    """
    定时器事件 (SYS_TIMER)
    
    Fast: pid (32), tid (32), timer_id (32), flags (32)
    Wide: same
    """
    pid: int = 0
    tid: int = 0
    timer_id: int = 0
    flags: int = 0
