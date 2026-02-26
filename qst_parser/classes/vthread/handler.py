"""
QST Parser - VThread 类事件处理器

处理 _NTO_TRACE_VTHREAD 类的所有事件。
虚拟线程用于 Transparent Distributed Processing (TDP) over Qnet。

复用 Thread 类的解析逻辑。
"""

from typing import List, Optional
from ..base import BaseClassHandler
from ...models.base import RawEvent
from ..thread.models import ThreadEvent
from ..thread.events import ThreadStateEventParser


class VThreadClassHandler(BaseClassHandler):
    """
    VThread 类事件处理器
    
    虚拟线程的状态事件格式与普通线程相同，但不包含 APS 相关字段。
    
    Fast mode:
        - data[0]: timestamp
        - data[1]: pid
        - data[2]: tid
    
    Wide mode:
        - 同 Fast mode（虚拟线程通常不使用扩展字段）
    """
    
    def __init__(self, verbose: bool = False):
        super().__init__(verbose)
        self._state_parser = ThreadStateEventParser(verbose)
        self.thread_events: List[ThreadEvent] = []
    
    @property
    def class_name(self) -> str:
        return "VTHREAD"
    
    def handle_event(
        self, 
        raw: RawEvent, 
        ext_event: int,
        timestamp_ns: int,
        **kwargs
    ) -> Optional[ThreadEvent]:
        """
        处理虚拟线程状态事件
        
        Args:
            raw: 原始事件数据
            ext_event: 线程状态码
            timestamp_ns: 已计算的 UTC 时间戳
        
        Returns:
            ThreadEvent 对象 (is_vthread=True)
        """
        event = self._state_parser.parse(raw, ext_event, is_vthread=True)
        if event:
            event.timestamp_ns = timestamp_ns
            self.thread_events.append(event)
        return event
    
    def get_events(self) -> List[ThreadEvent]:
        """获取所有已解析的虚拟线程事件"""
        return self.thread_events
