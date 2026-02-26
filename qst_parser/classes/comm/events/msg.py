"""
QST Parser - Comm 消息事件解析器

参考文档: kercall_table_Events.html

_NTO_TRACE_COMM_SMSG:
    Fast/Wide: rcvid, pid
    
_NTO_TRACE_COMM_RMSG:
    Fast/Wide: rcvid, pid
"""

from typing import Optional
from ....models.base import RawEvent
from ....constants import StructType
from ..models import CommMsgEvent


class CommMsgEventParser:
    """消息事件解析器 (SMSG/RMSG)"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
    
    def parse(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        event_type: str
    ) -> CommMsgEvent:
        """
        解析消息事件
        
        Args:
            raw: 原始事件数据
            timestamp_ns: 已计算的时间戳
            event_type: 事件类型 (smsg / rmsg)
        
        Returns:
            CommMsgEvent
        """
        # Fast/Wide 格式相同: rcvid, pid
        return CommMsgEvent(
            timestamp_cycles=raw.data[0],
            timestamp_ns=timestamp_ns,
            cpu_id=raw.cpu_id,
            event_type=event_type,
            rcvid=raw.data[1],
            pid=raw.data[2],
            is_wide=(raw.struct_type != StructType.SIMPLE),
        )
