"""
QST Parser - 函数跟踪事件解析器
"""

from typing import Optional
from ....models.base import RawEvent
from ..models import FuncTraceEvent


class FuncTraceEventParser:
    """函数跟踪事件解析器"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
    
    def parse(self, raw: RawEvent, is_enter: bool, is_64bit: bool = False) -> FuncTraceEvent:
        """解析函数跟踪事件"""
        event_type = "enter" if is_enter else "exit"
        
        if is_64bit:
            # 64-bit: thisfn (64), call_site (64)
            thisfn = raw.data[0] | (raw.data[1] << 32)
            call_site = raw.data[2]  # 需要更多数据
        else:
            thisfn = raw.data[0]
            call_site = raw.data[1]
        
        return FuncTraceEvent(
            timestamp_cycles=raw.data[0],
            cpu_id=raw.cpu_id,
            event_type=event_type,
            thisfn=thisfn,
            call_site=call_site,
        )
