"""
QST Parser - SEC 类事件处理器

处理 _NTO_TRACE_SEC 类事件 (Security)

安全相关事件，用于追踪安全检查和权限操作。
事件编号范围: 0x000 - 0x3ff

事件类型:
    - SEC_ABLE: 能力检查
    - SEC_ABLE_LOOKUP: 能力查找
    - SEC_PATH_ATTACH: 路径附加
    - SEC_QNET_CONNECT: Qnet 连接
    - SEC_PERM_LOOKUP: 权限查找
    - SEC_PERM_TEST: 权限测试
    - SEC_UNREG_EVENT: 未注册事件
"""

from .handler import SecClassHandler
from .models import SecEvent
from .constants import SecEventType

__all__ = [
    "SecClassHandler",
    "SecEvent",
    "SecEventType",
]
