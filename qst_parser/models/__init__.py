"""
QST Parser - 公共数据模型

仅包含文件头等公共结构，事件特定的模型应放在各自的 class 目录中。
"""

from .headers import FileHeader, ProcInfoHeader, MainDataHeader
from .base import RawEvent, BaseEvent

__all__ = [
    "FileHeader",
    "ProcInfoHeader", 
    "MainDataHeader",
    "RawEvent",
    "BaseEvent",
]
