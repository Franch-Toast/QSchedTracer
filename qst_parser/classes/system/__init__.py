"""
QST Parser - System 类事件处理器

处理 _NTO_TRACE_SYSTEM 类事件（系统相关）:
    - SYS_PATHMGR: 路径管理
    - SYS_APS_*: APS 调度器相关
    - SYS_MMAP/MUNMAP: 内存映射
    - SYS_MAPNAME: 动态加载
    - SYS_ADDRESS: 断点
    - SYS_FUNC_ENTER/EXIT: 函数跟踪
    - SYS_SLOG: 系统日志
    - SYS_RUNSTATE: CPU 运行状态
    - SYS_POWER: 电源管理
    - SYS_IPI: 处理器间中断
    - SYS_PROFILE: 统计分析
    - SYS_PAGEWAIT: 页面等待
    - SYS_TIMER: 定时器
"""

from .handler import SystemClassHandler
from .models import SystemEvent

__all__ = [
    "SystemClassHandler",
    "SystemEvent",
]
