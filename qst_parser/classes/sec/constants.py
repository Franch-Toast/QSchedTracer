"""
QST Parser - SEC 类常量定义

参考: sys/trace.h
"""

from enum import IntEnum


class SecEventType(IntEnum):
    """
    安全事件类型
    
    参考 sys/trace.h: _NTO_TRACE_SEC_*
    """
    ABLE = 0            # _NTO_TRACE_SEC_ABLE - 能力检查
    ABLE_LOOKUP = 1     # _NTO_TRACE_SEC_ABLE_LOOKUP - 能力查找
    PATH_ATTACH = 2     # _NTO_TRACE_SEC_PATH_ATTACH - 路径附加
    QNET_CONNECT = 3    # _NTO_TRACE_SEC_QNET_CONNECT - Qnet 连接
    PERM_LOOKUP = 4     # _NTO_TRACE_SEC_PERM_LOOKUP - 权限查找
    PERM_TEST = 5       # _NTO_TRACE_SEC_PERM_TEST - 权限测试
    UNREG_EVENT = 6     # _NTO_TRACE_SEC_UNREG_EVENT - 未注册事件


class SecAbleFlags(IntEnum):
    """
    能力检查标志
    
    参考 sys/trace.h: _TRACE_SEC_ABLE_FLAGS
    """
    ALLOW = 1           # _TRACE_SEC_ABLE_ALLOW
    IS_ROOT = 2         # _TRACE_SEC_ABLE_IS_ROOT
    UNCREATED = 4       # _TRACE_SEC_ABLE_UNCREATED


# 安全事件编号范围
SEC_FIRST = 0x000  # _NTO_TRACE_SECFIRST
SEC_LAST = 0x3ff   # _NTO_TRACE_SECLAST
MAX_SEC_NUM = SEC_LAST + 1
