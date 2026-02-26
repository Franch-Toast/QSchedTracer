"""
QST Parser - Interrupt 类事件处理器

处理所有中断相关事件:
- INTENTER / INTENTER_64: 中断进入
- INTEXIT: 中断退出
- INT_HANDLER_ENTER / INT_HANDLER_ENTER_64: ISR handler 进入
- INT_HANDLER_EXIT: ISR handler 退出

关键理解 (参考 sys/trace.h, samples_parser.html):
- 内部事件号 (int_event) 决定事件类型:
    1 = _TRACE_INT_ENTRY (32-bit)
    2 = _TRACE_INT_EXIT
    3 = _TRACE_INT_HANDLER_ENTRY (32-bit)
    4 = _TRACE_INT_HANDLER_EXIT
    5 = _TRACE_INT_ENTRY_64 (64-bit)
    6 = _TRACE_INT_HANDLER_ENTRY_64 (64-bit)
- 实际的中断号存储在 data[1] 中，不在事件号中
- 外部类没有 INTENTER_64 等，64位模式通过 int_event 区分
"""

from typing import List, Optional, Union
from ..base import BaseClassHandler
from ...models.base import RawEvent
from ...constants import ExternalClass, IntEventType
from .models import (
    InterruptBaseEvent,
    InterruptEnterEvent, InterruptExitEvent,
    InterruptHandlerEnterEvent, InterruptHandlerExitEvent,
    InterruptEvent,  # 兼容旧代码
)
from .events import IntEnterEventParser, IntExitEventParser, IntHandlerEventParser


class InterruptClassHandler(BaseClassHandler):
    """
    Interrupt 类事件处理器
    
    处理所有中断相关事件。
    
    外部类映射 (注意: 没有单独的 _64 外部类):
        - INTENTER (7): 中断进入 (int_event=1 为 32-bit, int_event=5 为 64-bit)
        - INTEXIT (8): 中断退出
        - INT_HANDLER_ENTER (15): ISR handler 进入 (int_event=3 为 32-bit, int_event=6 为 64-bit)
        - INT_HANDLER_EXIT (16): ISR handler 退出
    """
    
    def __init__(self, verbose: bool = False):
        super().__init__(verbose)
        
        self._enter_parser = IntEnterEventParser(verbose)
        self._exit_parser = IntExitEventParser(verbose)
        self._handler_parser = IntHandlerEventParser(verbose)
        
        # 分类存储事件
        self.enter_events: List[InterruptEnterEvent] = []
        self.exit_events: List[InterruptExitEvent] = []
        self.handler_enter_events: List[InterruptHandlerEnterEvent] = []
        self.handler_exit_events: List[InterruptHandlerExitEvent] = []
        
        # 兼容旧代码
        self.interrupt_events: List[InterruptEvent] = []
    
    @property
    def class_name(self) -> str:
        return "INTERRUPT"
    
    def handle_event(
        self, 
        raw: RawEvent, 
        ext_event: int,
        timestamp_ns: int,
        ext_class: int = ExternalClass.INTENTER,
        **kwargs
    ) -> Optional[InterruptBaseEvent]:
        """
        处理中断事件
        
        Args:
            raw: 原始事件数据
            ext_event: 内部事件号 (int_event)，用于判断 32/64 位模式
            timestamp_ns: 已计算的 UTC 时间戳
            ext_class: 外部事件类
        
        Returns:
            InterruptBaseEvent 的子类
        """
        event = None
        int_event = ext_event  # ext_event 实际上是 int_event
        
        # 中断号从 data[1] 获取
        interrupt_number = raw.data[1] if len(raw.data) > 1 else 0
        
        # 根据外部类和 int_event 判断事件类型和 64 位模式
        if ext_class == ExternalClass.INTENTER:
            # 中断进入事件
            # int_event=1: 32-bit, int_event=5: 64-bit
            is_64bit = (int_event == IntEventType.ENTRY_64)
            event = self._enter_parser.parse(raw, timestamp_ns, interrupt_number, is_64bit=is_64bit)
            if event:
                self.enter_events.append(event)
        
        elif ext_class == ExternalClass.INTEXIT:
            # 中断退出事件
            event = self._exit_parser.parse(raw, timestamp_ns, interrupt_number)
            if event:
                self.exit_events.append(event)
        
        elif ext_class == ExternalClass.INT_HANDLER_ENTER:
            # ISR handler 进入事件
            # int_event=3: 32-bit, int_event=6: 64-bit
            is_64bit = (int_event == IntEventType.HANDLER_ENTRY_64)
            event = self._handler_parser.parse_enter(raw, timestamp_ns, interrupt_number, is_64bit=is_64bit)
            if event:
                self.handler_enter_events.append(event)
        
        elif ext_class == ExternalClass.INT_HANDLER_EXIT:
            # ISR handler 退出事件
            event = self._handler_parser.parse_exit(raw, timestamp_ns, interrupt_number)
            if event:
                self.handler_exit_events.append(event)
        
        return event
    
    # ========================================================================
    # 事件查询接口
    # ========================================================================
    
    def get_all_events(self) -> List[InterruptBaseEvent]:
        """获取所有中断事件"""
        return (self.enter_events + self.exit_events + 
                self.handler_enter_events + self.handler_exit_events)
    
    def get_events_by_interrupt(self, interrupt_number: int) -> List[InterruptBaseEvent]:
        """获取指定中断号的所有事件"""
        return [e for e in self.get_all_events() if e.interrupt_number == interrupt_number]
