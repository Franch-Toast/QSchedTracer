"""
QST Parser - Comm 脉冲事件解析器

参考文档: kercall_table_Events.html

_NTO_TRACE_COMM_SPULSE / RPULSE / SPULSE_EXE / SPULSE_DIS / 
SPULSE_DEA / SPULSE_UN / SPULSE_QUN:
    Fast/Wide: scoid, pid
"""

from typing import Optional
from ....models.base import RawEvent
from ....constants import StructType
from ..models import CommPulseEvent


class CommPulseEventParser:
    """脉冲事件解析器 (SPULSE/RPULSE/...)"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
    
    def parse(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        event_type: str
    ) -> CommPulseEvent:
        """
        解析脉冲事件
        
        Args:
            raw: 原始事件数据
            timestamp_ns: 已计算的时间戳
            event_type: 事件类型 (spulse / rpulse / spulse_exe / ...)
        
        Returns:
            CommPulseEvent
        """
        # Fast/Wide 格式相同: scoid, pid
        return CommPulseEvent(
            timestamp_cycles=raw.data[0],
            timestamp_ns=timestamp_ns,
            cpu_id=raw.cpu_id,
            event_type=event_type,
            scoid=raw.data[1],
            pid=raw.data[2],
            is_wide=(raw.struct_type != StructType.SIMPLE),
        )
