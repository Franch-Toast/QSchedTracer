"""
QST Parser - 公共常量定义

仅包含全局共享的常量，事件类特定的常量应放在各自的 class 目录中。
"""

from enum import IntEnum

# ============================================================================
# 文件格式常量
# ============================================================================

QST_MAGIC = 0x51535433      # 'QST3'
QST_VERSION = 3
QST_EVENT_SIZE = 16

# 文件头大小
FILE_HEADER_SIZE = 64
PROCINFO_HEADER_SIZE = 16
MAINDATA_HEADER_SIZE = 16

# Section 魔数
PROCINFO_MAGIC = 0x50494E46  # 'PINF'
MAINDATA_MAGIC = 0x4D41494E  # 'MAIN'

# 32 位最大值（用于处理时间回环）
MAX_UINT32 = 0xFFFFFFFF

# ============================================================================
# 内核调用相关常量
# 参考: sys/trace.h
# ============================================================================

# 内核调用数量限制（用于区分 ENTER/EXIT/INT）
_TRACE_MAX_KER_CALL_NUM = 128

# 64 位内核调用标志
KERCALL_64 = 0x200  # _NTO_TRACE_KERCALL64 (bit 9)


# ============================================================================
# 内部事件类定义 (用于事件头解析)
# 参考: sys/trace.h - _TRACE_*_C 定义
# ============================================================================

class InternalClass(IntEnum):
    """
    内部事件类
    
    QNX trace 系统使用的内部类别编号，从事件头的 bits[10:14] 提取。
    参考 sys/trace.h 中的 _TRACE_*_C 定义。
    """
    EMPTY = 0       # _TRACE_EMPTY_C
    CONTROL = 1     # _TRACE_CONTROL_C
    KER_CALL = 2    # _TRACE_KER_CALL_C
    INT = 3         # _TRACE_INT_C
    PR_TH = 4       # _TRACE_PR_TH_C (PROCESS/THREAD/VTHREAD 共用)
    SYSTEM = 5      # _TRACE_SYSTEM_C
    USER = 6        # _TRACE_USER_C
    COMM = 7        # _TRACE_COMM_C
    QUIP = 8        # _TRACE_QUIP_C (QNX Unified Instrumentation Platform)
    SEC = 9         # _TRACE_SEC_C (Security)
    QVM = 10        # _TRACE_QVM_C (Hypervisor/QVM)


# ============================================================================
# 外部事件类定义
# 参考: sys/trace.h enum 定义
# ============================================================================

class ExternalClass(IntEnum):
    """
    外部事件类
    
    用于事件分类的外部类别，从内部类转换而来。
    参考 sys/trace.h 中的 _NTO_TRACE_* enum。
    """
    EMPTY = 0           # _NTO_TRACE_EMPTY
    CONTROL = 1         # _NTO_TRACE_CONTROL
    KERCALL = 2         # _NTO_TRACE_KERCALL
    KERCALLENTER = 3    # _NTO_TRACE_KERCALLENTER
    KERCALLEXIT = 4     # _NTO_TRACE_KERCALLEXIT
    KERCALLINT = 5      # _NTO_TRACE_KERCALLINT
    INT = 6             # _NTO_TRACE_INT
    INTENTER = 7        # _NTO_TRACE_INTENTER
    INTEXIT = 8         # _NTO_TRACE_INTEXIT
    PROCESS = 9         # _NTO_TRACE_PROCESS
    THREAD = 10         # _NTO_TRACE_THREAD
    VTHREAD = 11        # _NTO_TRACE_VTHREAD
    USER = 12           # _NTO_TRACE_USER
    SYSTEM = 13         # _NTO_TRACE_SYSTEM
    COMM = 14           # _NTO_TRACE_COMM
    INT_HANDLER_ENTER = 15  # _NTO_TRACE_INT_HANDLER_ENTER
    INT_HANDLER_EXIT = 16   # _NTO_TRACE_INT_HANDLER_EXIT
    QUIP = 17           # _NTO_TRACE_QUIP
    SEC = 18            # _NTO_TRACE_SEC
    QVM = 19            # _NTO_TRACE_QVM
    # 注意: INTENTER_64 和 INT_HANDLER_ENTER_64 不是独立的外部类，
    # 而是通过内部事件号 (int_event) 来区分的:
    # - _TRACE_INT_ENTRY (1) vs _TRACE_INT_ENTRY_64 (5)
    # - _TRACE_INT_HANDLER_ENTRY (3) vs _TRACE_INT_HANDLER_ENTRY_64 (6)


# ============================================================================
# 事件结构类型 (用于组合事件)
# ============================================================================

class StructType(IntEnum):
    """
    事件结构类型
    
    用于标识事件是简单事件还是组合事件的一部分。
    从事件头的 bits[30:31] 提取。
    参考 sys/trace.h: _TRACE_STRUCT_*
    """
    SIMPLE = 0           # _TRACE_STRUCT_S - 简单事件（Fast mode）
    COMBINE_BEGIN = 1    # _TRACE_STRUCT_CB - 组合事件开始（Wide mode）
    COMBINE_CONT = 2     # _TRACE_STRUCT_CC - 组合事件继续
    COMBINE_END = 3      # _TRACE_STRUCT_CE - 组合事件结束


# ============================================================================
# 事件模式
# ============================================================================

class EventMode(IntEnum):
    """事件记录模式"""
    FAST = 0    # Fast mode - 精简参数
    WIDE = 1    # Wide mode - 完整参数（组合事件）


# ============================================================================
# 中断事件子类型
# 参考: sys/trace.h - _TRACE_INT_*
# ============================================================================

class IntEventType(IntEnum):
    """中断事件子类型"""
    ENTRY = 1           # _TRACE_INT_ENTRY
    EXIT = 2            # _TRACE_INT_EXIT
    HANDLER_ENTRY = 3   # _TRACE_INT_HANDLER_ENTRY
    HANDLER_EXIT = 4    # _TRACE_INT_HANDLER_EXIT
    ENTRY_64 = 5        # _TRACE_INT_ENTRY_64
    HANDLER_ENTRY_64 = 6  # _TRACE_INT_HANDLER_ENTRY_64


# ============================================================================
# Perfetto 颜色 (用于导出)
# ============================================================================

STATE_COLORS = {
    "RUNNING": "good",
    "READY": "olive",
    "RECEIVE": "rail_load",
    "SEND": "rail_response",
    "REPLY": "rail_animation",
    "MUTEX": "bad",
    "CONDVAR": "terrible",
    "SEM": "bad",
    "NANOSLEEP": "thread_state_sleeping",
    "DEAD": "black",
    "STOPPED": "grey",
    "INTR": "yellow",
}
