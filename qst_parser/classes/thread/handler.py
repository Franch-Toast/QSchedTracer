"""
QST Parser - Thread 类事件处理器

处理 _NTO_TRACE_THREAD 类的所有事件。
"""

from typing import List, Optional
from ..base import BaseClassHandler
from ...models.base import RawEvent
from .models import ThreadEvent
from .events import ThreadStateEventParser


class ThreadClassHandler(BaseClassHandler):
    """
    Thread 类事件处理器
    
    处理 _NTO_TRACE_THREAD 类的所有线程状态变化事件。
    """
    
    def __init__(self, verbose: bool = False):
        super().__init__(verbose)
        self._state_parser = ThreadStateEventParser(verbose)
        self.thread_events: List[ThreadEvent] = []
    
    @property
    def class_name(self) -> str:
        return "THREAD"
    
    def handle_event(
        self, 
        raw: RawEvent, 
        ext_event: int,
        timestamp_ns: int,
        **kwargs
    ) -> Optional[ThreadEvent]:
        """
        处理线程状态事件
        
        Args:
            raw: 原始事件数据
            ext_event: 线程状态码
            timestamp_ns: 已计算的 UTC 时间戳
        
        Returns:
            ThreadEvent 对象
        """
        event = self._state_parser.parse(raw, ext_event, is_vthread=False)
        if event:
            event.timestamp_ns = timestamp_ns
            self.thread_events.append(event)
        return event
    
    def get_events(self) -> List[ThreadEvent]:
        """获取所有已解析的线程事件"""
        return self.thread_events
    
    def clear_events(self) -> None:
        """清空已解析的事件"""
        self.thread_events.clear()
