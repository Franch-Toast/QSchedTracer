"""
QST Parser - Interrupt 类常量定义

中断事件类型常量。
参考: sys/trace.h, kercall_table_Events.html

关键理解:
- 内部事件号 (int_event) 决定事件类型和 32/64 位模式
- 没有单独的外部类 INTENTER_64 或 INT_HANDLER_ENTER_64
- 64位模式通过 int_event=5 (ENTRY_64) 或 int_event=6 (HANDLER_ENTRY_64) 标识
"""

from enum import IntEnum


class InterruptEventType(IntEnum):
    """
    中断内部事件类型 (int_event)
    
    参考 sys/trace.h:
        #define _TRACE_INT_ENTRY            (1)
        #define _TRACE_INT_EXIT             (2)
        #define _TRACE_INT_HANDLER_ENTRY    (3)
        #define _TRACE_INT_HANDLER_EXIT     (4)
        #define _TRACE_INT_ENTRY_64         (5)
        #define _TRACE_INT_HANDLER_ENTRY_64 (6)
    """
    INTENTER = 1                # _TRACE_INT_ENTRY (32-bit)
    INTEXIT = 2                 # _TRACE_INT_EXIT
    INT_HANDLER_ENTER = 3       # _TRACE_INT_HANDLER_ENTRY (32-bit)
    INT_HANDLER_EXIT = 4        # _TRACE_INT_HANDLER_EXIT
    INTENTER_64 = 5             # _TRACE_INT_ENTRY_64 (64-bit)
    INT_HANDLER_ENTER_64 = 6    # _TRACE_INT_HANDLER_ENTRY_64 (64-bit)


# 中断号范围
# 参考 sys/trace.h:
#   #define _NTO_TRACE_INTFIRST         (0x00000000u)
#   #define _NTO_TRACE_INTLAST          (0xffffffffu)
TRACE_INTFIRST = 0x00000000
TRACE_INTLAST = 0xFFFFFFFF
