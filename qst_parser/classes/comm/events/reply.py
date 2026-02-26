"""
QST Parser - Comm 回复/错误事件解析器

参考文档: kercall_table_Events.html

_NTO_TRACE_COMM_REPLY:
    Fast/Wide: tid, pid
    
_NTO_TRACE_COMM_ERROR:
    Fast/Wide: tid, pid
    
注意：QNX 的 "tid" 字段是复合值 (0xNNNNTTTT)：
- 高 16 位 (nd): 节点描述符
- 低 16 位 (tid): 真正的线程 ID
"""

from typing import Optional
from ....models.base import RawEvent
from ....constants import StructType
from ..models import CommReplyEvent


class CommReplyEventParser:
    """回复/错误事件解析器 (REPLY/ERROR)"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
    
    def parse(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        event_type: str
    ) -> CommReplyEvent:
        """
        解析回复/错误事件
        
        Args:
            raw: 原始事件数据
            timestamp_ns: 已计算的时间戳
            event_type: 事件类型 (reply / error)
        
        Returns:
            CommReplyEvent
        """
        # Fast/Wide 格式相同: tid, pid
        # tid 是复合值：高 16 位=nd，低 16 位=真正的 tid
        tid_raw = raw.data[1]
        tid = tid_raw & 0xFFFF       # 低 16 位是真正的 tid
        nd = (tid_raw >> 16) & 0xFFFF  # 高 16 位是节点描述符
        
        return CommReplyEvent(
            timestamp_cycles=raw.data[0],
            timestamp_ns=timestamp_ns,
            cpu_id=raw.cpu_id,
            event_type=event_type,
            tid_raw=tid_raw,
            tid=tid,
            nd=nd,
            pid=raw.data[2],
            is_wide=(raw.struct_type != StructType.SIMPLE),
        )
