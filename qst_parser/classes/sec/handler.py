"""
QST Parser - SEC 类事件处理器
"""

from typing import List, Optional, Any
from ..base import BaseClassHandler
from .models import SecEvent, SecAbleEvent, SecPathAttachEvent, SecQnetConnectEvent
from .constants import SecEventType
from ...constants import StructType


class SecClassHandler(BaseClassHandler):
    """
    SEC 类事件处理器
    
    处理安全相关事件。
    """
    
    def __init__(self, verbose: bool = False):
        super().__init__(verbose)
        self.sec_events: List[SecEvent] = []
    
    @property
    def class_name(self) -> str:
        return "SEC"
    
    def handle_event(
        self, 
        raw, 
        ext_event: int, 
        timestamp_ns: int = 0,
        **kwargs
    ) -> Optional[Any]:
        """
        处理安全事件
        
        Args:
            raw: 原始事件数据
            ext_event: 事件编号
            timestamp_ns: 已计算的 UTC 时间戳
        """
        event = None
        
        if ext_event in (SecEventType.ABLE, SecEventType.ABLE_LOOKUP):
            event = SecAbleEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                ability=raw.data[1],
                flags=raw.data[2] if ext_event == SecEventType.ABLE else 0,
            )
        
        elif ext_event == SecEventType.PATH_ATTACH:
            event = self._handle_path_attach(raw, timestamp_ns)
        
        elif ext_event == SecEventType.QNET_CONNECT:
            event = SecQnetConnectEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                status=raw.data[1],
            )
        
        else:
            # 其他安全事件
            event = SecEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                event_type=ext_event,
                event_type_name=SecEventType(ext_event).name if ext_event <= 6 else f"SEC_{ext_event}",
            )
        
        if event:
            self.sec_events.append(event)
        
        return event
    
    def _handle_path_attach(self, raw, timestamp_ns: int) -> Optional[SecPathAttachEvent]:
        """处理路径附加事件（可能是组合事件）"""
        struct_type = raw.struct_type
        key = f"sec_path_{raw.cpu_id}"
        
        if struct_type == StructType.SIMPLE:
            return SecPathAttachEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                status=raw.data[1],
            )
        
        elif struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffer[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'status': raw.data[1],
                'path_bytes': bytearray(),
            }
            return None
        
        elif struct_type in (StructType.COMBINE_CONT, StructType.COMBINE_END):
            if key in self._combine_buffer:
                import struct as st
                buf = self._combine_buffer[key]
                buf['path_bytes'].extend(st.pack('<II', raw.data[1], raw.data[2]))
                
                if struct_type == StructType.COMBINE_END:
                    path = buf['path_bytes'].rstrip(b'\x00').decode('utf-8', errors='replace')
                    event = SecPathAttachEvent(
                        timestamp_cycles=buf['timestamp_cycles'],
                        timestamp_ns=buf['timestamp_ns'],
                        cpu_id=buf['cpu_id'],
                        status=buf['status'],
                        path=path,
                    )
                    del self._combine_buffer[key]
                    return event
        
        return None
