"""
QST Parser - System 类事件处理器

处理所有系统相关事件:
- SYS_MMAP / SYS_MUNMAP: 内存映射
- SYS_FUNC_ENTER / SYS_FUNC_EXIT: 函数跟踪
- SYS_IPI / SYS_IPI_64: 处理器间中断
- SYS_PAGEWAIT: 页面等待

参考: kercall_table_Events.html
"""

from typing import List, Optional, Any, Union
from ..base import BaseClassHandler
from ...models.base import RawEvent
from .models import (
    SystemEvent, MmapEvent, FuncTraceEvent, 
    IPIEvent, PageWaitEvent, TimerEvent, ProfileEvent,
)
from .constants import SystemEventType
from .events import (
    MmapEventParser, FuncTraceEventParser,
    IPIEventParser, PageWaitEventParser,
)


class SystemClassHandler(BaseClassHandler):
    """
    System 类事件处理器
    
    处理所有系统相关事件。
    """
    
    def __init__(self, verbose: bool = False):
        super().__init__(verbose)
        
        self._mmap_parser = MmapEventParser(verbose)
        self._func_trace_parser = FuncTraceEventParser(verbose)
        self._ipi_parser = IPIEventParser(verbose)
        self._pagewait_parser = PageWaitEventParser(verbose)
        
        # 分类存储事件
        self.mmap_events: List[MmapEvent] = []
        self.func_trace_events: List[FuncTraceEvent] = []
        self.ipi_events: List[IPIEvent] = []
        self.pagewait_events: List[PageWaitEvent] = []
        
        # 兼容旧代码
        self.system_events: List[SystemEvent] = []
    
    @property
    def class_name(self) -> str:
        return "SYSTEM"
    
    def handle_event(
        self, 
        raw: RawEvent, 
        ext_event: int,
        timestamp_ns: int,
        **kwargs
    ) -> Optional[Any]:
        """
        处理系统事件
        
        Args:
            raw: 原始事件数据
            ext_event: 系统事件类型
            timestamp_ns: 已计算的 UTC 时间戳
        
        Returns:
            事件对象
        """
        event = None
        
        # 内存映射事件
        if ext_event == SystemEventType.SYS_MMAP:
            event = self._mmap_parser.parse_mmap(raw)
            if event:
                event.timestamp_ns = timestamp_ns
                self.mmap_events.append(event)
        
        elif ext_event == SystemEventType.SYS_MUNMAP:
            event = self._mmap_parser.parse_munmap(raw)
            if event:
                event.timestamp_ns = timestamp_ns
                self.mmap_events.append(event)
        
        # 函数跟踪事件
        elif ext_event == SystemEventType.SYS_FUNC_ENTER:
            event = self._func_trace_parser.parse(raw, is_enter=True)
            if event:
                event.timestamp_ns = timestamp_ns
                self.func_trace_events.append(event)
        
        elif ext_event == SystemEventType.SYS_FUNC_EXIT:
            event = self._func_trace_parser.parse(raw, is_enter=False)
            if event:
                event.timestamp_ns = timestamp_ns
                self.func_trace_events.append(event)
        
        # 处理器间中断事件 (32-bit)
        elif ext_event == SystemEventType.SYS_IPI:
            event = self._ipi_parser.parse(raw, timestamp_ns, is_64bit=False)
            if event:
                self.ipi_events.append(event)
        
        # 处理器间中断事件 (64-bit)
        elif ext_event == SystemEventType.SYS_IPI_64:
            event = self._ipi_parser.parse(raw, timestamp_ns, is_64bit=True)
            if event:
                self.ipi_events.append(event)
        
        # 页面等待事件
        elif ext_event == SystemEventType.SYS_PAGEWAIT:
            event = self._pagewait_parser.parse(raw, timestamp_ns)
            if event:
                self.pagewait_events.append(event)
        
        # TODO: 实现其他系统事件的解析
        # - SYS_PROFILE / SYS_PROFILE_64
        # - SYS_TIMER
        # - SYS_POWER
        # - SYS_APS_*
        # - SYS_SCHED_CONF
        
        return event
    
    # ========================================================================
    # 事件查询接口
    # ========================================================================
    
    def get_all_events(self) -> List[Any]:
        """获取所有系统事件"""
        return (self.mmap_events + self.func_trace_events + 
                self.ipi_events + self.pagewait_events)
