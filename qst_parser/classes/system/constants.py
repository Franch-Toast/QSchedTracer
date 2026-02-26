"""
QST Parser - System 类常量定义

系统事件类型常量。
参考: kercall_table_Events.html
"""

from enum import IntEnum


class SystemEventType(IntEnum):
    """
    系统事件类型
    """
    SYS_PATHMGR = 1
    SYS_APS_NAME = 2
    SYS_APS_BUDGETS = 3
    SYS_APS_PSTATS = 4
    SYS_APS_OSTATS = 5
    SYS_APS_INFO = 6
    SYS_APS_JOIN = 7
    SYS_APS_THREAD = 8
    SYS_APS_PROCESS = 9
    SYS_APS_BNKR = 10
    SYS_SCHED_CONF = 11
    SYS_MMAP = 12
    SYS_MUNMAP = 13
    SYS_MAPNAME = 14
    SYS_MAPNAME_64 = 15
    SYS_ADDRESS = 16
    SYS_FUNC_ENTER = 17
    SYS_FUNC_EXIT = 18
    SYS_SLOG = 19
    SYS_RUNSTATE = 20
    SYS_POWER = 21
    SYS_IPI = 22
    SYS_IPI_64 = 23
    SYS_PROFILE = 24
    SYS_PROFILE_64 = 25
    SYS_PAGEWAIT = 26
    SYS_TIMER = 27
    SYS_DEFRAG_END = 28
