"""
QST Parser - QVM 类事件处理器
"""

from typing import List, Optional, Any
from ..base import BaseClassHandler
from .models import QvmEvent, QvmGuestEvent, QvmIntrEvent, QvmTimerEvent
from .constants import QvmEventType


class QvmClassHandler(BaseClassHandler):
    """
    QVM 类事件处理器
    
    处理虚拟化/Hypervisor 相关事件。
    """
    
    def __init__(self, verbose: bool = False):
        super().__init__(verbose)
        self.qvm_events: List[QvmEvent] = []
    
    @property
    def class_name(self) -> str:
        return "QVM"
    
    def handle_event(
        self, 
        raw, 
        ext_event: int, 
        timestamp_ns: int = 0,
        **kwargs
    ) -> Optional[Any]:
        """
        处理 QVM 事件
        
        Args:
            raw: 原始事件数据
            ext_event: 事件编号
            timestamp_ns: 已计算的 UTC 时间戳
        """
        event = None
        
        if ext_event == QvmEventType.GUEST_ENTER:
            event = QvmGuestEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                event_type="enter",
                guest_id=raw.data[1],
            )
        
        elif ext_event == QvmEventType.GUEST_EXIT:
            event = QvmGuestEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                event_type="exit",
                guest_id=raw.data[1],
            )
        
        elif ext_event == QvmEventType.RAISE_INTR:
            event = QvmIntrEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                event_type="raise",
                intr_num=raw.data[1],
            )
        
        elif ext_event == QvmEventType.LOWER_INTR:
            event = QvmIntrEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                event_type="lower",
                intr_num=raw.data[1],
            )
        
        elif ext_event == QvmEventType.TIMER_CREATE:
            event = QvmTimerEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                event_type="create",
                timer_id=raw.data[1],
            )
        
        elif ext_event == QvmEventType.TIMER_FIRE:
            event = QvmTimerEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                event_type="fire",
                timer_id=raw.data[1],
            )
        
        else:
            # 其他 QVM 事件
            event_type_name = QvmEventType(ext_event).name if ext_event <= 7 else f"QVM_{ext_event}"
            event = QvmEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                event_type=ext_event,
                event_type_name=event_type_name,
            )
        
        if event:
            self.qvm_events.append(event)
        
        return event
