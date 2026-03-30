"""
QST Parser — Streaming QNX Trace Event Parser (QST v4 format)
"""

__version__ = "4.0.0"

from .core.streaming_parser import StreamingParser

__all__ = ["StreamingParser"]
