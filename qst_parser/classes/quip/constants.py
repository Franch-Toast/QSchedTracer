"""
QST Parser - QUIP 类常量定义

参考: sys/trace.h
"""

# QUIP 事件编号范围
QUIP_FIRST = 0x000  # _NTO_TRACE_QUIPFIRST
QUIP_LAST = 0x3ff   # _NTO_TRACE_QUIPLAST
MAX_QUIP_NUM = QUIP_LAST + 1
