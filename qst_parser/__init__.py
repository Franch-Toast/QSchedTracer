"""
QST Parser - QNX Trace Event 解析器

模块化的 QST 文件解析器，支持多种事件类型的解析和导出。
"""

__version__ = "1.0.0"
__author__ = "QSchedTracer Team"

from .parser import QstParser

__all__ = ["QstParser"]
