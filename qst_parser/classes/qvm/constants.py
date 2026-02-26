"""
QST Parser - QVM 类常量定义

参考: sys/trace.h
"""

from enum import IntEnum


class QvmEventType(IntEnum):
    """
    QVM 事件类型
    
    参考 sys/trace.h: _NTO_TRACE_QVM_*
    """
    GUEST_ENTER = 0         # _NTO_TRACE_QVM_GUEST_ENTER
    GUEST_EXIT = 1          # _NTO_TRACE_QVM_GUEST_EXIT
    CREATE_VCPU_THREAD = 2  # _NTO_TRACE_QVM_CREATE_VCPU_THREAD
    RAISE_INTR = 3          # _NTO_TRACE_QVM_RAISE_INTR
    LOWER_INTR = 4          # _NTO_TRACE_QVM_LOWER_INTR
    TIMER_CREATE = 5        # _NTO_TRACE_QVM_TIMER_CREATE
    TIMER_FIRE = 6          # _NTO_TRACE_QVM_TIMER_FIRE
    CYCLES = 7              # _NTO_TRACE_QVM_CYCLES


# QVM 事件编号范围
QVM_FIRST = 0x000  # _NTO_TRACE_QVMFIRST
QVM_LAST = 0x3ff   # _NTO_TRACE_QVMLAST
MAX_QVM_NUM = QVM_LAST + 1
