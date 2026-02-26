"""
QST Parser - Comm 信号事件解析器

参考文档: kercall_table_Events.html

_NTO_TRACE_COMM_SIGNAL:
    Fast: si_signo, si_code
    Wide: si_signo, si_code, si_errno, __data.__pad[0], __data.__pad[1], 
          __data.__pad[2], __data.__pad[3], __data.__pad[4], __data.__pad[5]
"""

from typing import Optional, Dict, Any, List
from ....models.base import RawEvent
from ....constants import StructType
from ..models import CommSignalEvent


class CommSignalEventParser:
    """信号事件解析器 (COMM_SIGNAL)"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._combine_buffers: Dict[str, Dict[str, Any]] = {}
    
    def parse(
        self,
        raw: RawEvent,
        timestamp_ns: int
    ) -> Optional[CommSignalEvent]:
        """
        解析信号事件
        
        Args:
            raw: 原始事件数据
            timestamp_ns: 已计算的时间戳
        
        Returns:
            CommSignalEvent 或 None（组合事件未完成时）
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            # Fast mode: si_signo, si_code
            return CommSignalEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                si_signo=raw.data[1],
                si_code=raw.data[2],
                is_wide=False,
            )
        
        # Wide mode 组合事件
        return self._parse_wide(raw, timestamp_ns, struct_type)
    
    def _parse_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        struct_type: int
    ) -> Optional[CommSignalEvent]:
        """解析 Wide mode 组合事件"""
        key = f"comm_signal_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            # COMBINE_BEGIN: timestamp, si_signo, si_code
            self._combine_buffers[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'si_signo': raw.data[1],
                'si_code': raw.data[2],
                'extra_data': [],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            # COMBINE_CONT: 中间数据
            matching_key = self._find_key(raw, "comm_signal_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            # COMBINE_END: 最后的数据
            matching_key = self._find_key(raw, "comm_signal_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
                
                # Wide: si_signo, si_code, si_errno, __data.__pad[0-5]
                extra = buf['extra_data']
                si_errno = extra[0] if len(extra) > 0 else 0
                si_data = extra[1:] if len(extra) > 1 else []
                
                event = CommSignalEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    si_signo=buf['si_signo'],
                    si_code=buf['si_code'],
                    si_errno=si_errno,
                    si_data=list(si_data),
                    is_wide=True,
                )
                
                del self._combine_buffers[matching_key]
                return event
        
        return None
    
    def _find_key(self, raw: RawEvent, prefix: str) -> Optional[str]:
        """查找匹配的组合事件 key"""
        search_prefix = f"{prefix}{raw.cpu_id}_"
        for k in self._combine_buffers.keys():
            if k.startswith(search_prefix):
                buf = self._combine_buffers[k]
                if buf.get('timestamp_cycles') == raw.data[0]:
                    return k
        return None
