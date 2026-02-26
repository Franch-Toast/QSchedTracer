"""
QST Parser - QUIP 类事件处理器
"""

from typing import List, Optional
from ..base import BaseClassHandler
from .models import QuipEvent
from ...constants import StructType


class QuipClassHandler(BaseClassHandler):
    """
    QUIP 类事件处理器
    
    处理 QNX Unified Instrumentation Platform 事件。
    """
    
    def __init__(self, verbose: bool = False):
        super().__init__(verbose)
        self.quip_events: List[QuipEvent] = []
    
    @property
    def class_name(self) -> str:
        return "QUIP"
    
    def handle_event(
        self, 
        raw, 
        ext_event: int, 
        timestamp_ns: int = 0,
        **kwargs
    ) -> Optional[QuipEvent]:
        """
        处理 QUIP 事件
        
        Args:
            raw: 原始事件数据
            ext_event: 事件编号
            timestamp_ns: 已计算的 UTC 时间戳
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            event = QuipEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                event_id=ext_event,
                data1=raw.data[1],
                data2=raw.data[2],
            )
            self.quip_events.append(event)
            return event
        
        # Wide mode - 需要处理组合事件
        return self._handle_wide_mode(raw, ext_event, timestamp_ns, struct_type)
    
    def _handle_wide_mode(
        self, 
        raw, 
        ext_event: int, 
        timestamp_ns: int,
        struct_type: int
    ) -> Optional[QuipEvent]:
        """处理 Wide mode 组合事件"""
        key = f"quip_{raw.cpu_id}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffer[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'event_id': ext_event,
                'data': [raw.data[1], raw.data[2]],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            if key in self._combine_buffer:
                self._combine_buffer[key]['data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            if key in self._combine_buffer:
                buf = self._combine_buffer[key]
                buf['data'].extend([raw.data[1], raw.data[2]])
                
                event = QuipEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    event_id=buf['event_id'],
                    data1=buf['data'][0] if len(buf['data']) > 0 else 0,
                    data2=buf['data'][1] if len(buf['data']) > 1 else 0,
                    extra_data=tuple(buf['data'][2:]),
                )
                self.quip_events.append(event)
                del self._combine_buffer[key]
                return event
        
        return None
