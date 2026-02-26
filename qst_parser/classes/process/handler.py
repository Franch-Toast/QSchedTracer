"""
QST Parser - Process 类事件处理器

处理 _NTO_TRACE_PROCESS 类的所有事件。
"""

from typing import Dict, Optional, Tuple, Any
from ..base import BaseClassHandler
from ...models.base import RawEvent
from .constants import ProcessEventType
from .events import (
    ProcCreateEventParser,
    ProcDestroyEventParser,
    ProcThreadNameEventParser,
)


class ProcessClassHandler(BaseClassHandler):
    """
    Process 类事件处理器
    
    处理所有进程相关事件，提取进程/线程名称信息。
    """
    
    def __init__(self, verbose: bool = False):
        super().__init__(verbose)
        
        self._create_parser = ProcCreateEventParser(verbose)
        self._destroy_parser = ProcDestroyEventParser(verbose)
        self._thread_name_parser = ProcThreadNameEventParser(verbose)
    
    @property
    def class_name(self) -> str:
        return "PROCESS"
    
    @property
    def process_info(self) -> Dict[int, str]:
        """获取进程信息字典 (pid -> name)"""
        # 合并来自 create 和 destroy 解析器的信息
        info = {}
        info.update(self._create_parser.process_info)
        info.update(self._destroy_parser.process_info)
        return info
    
    @property
    def thread_info(self) -> Dict[Tuple[int, int], str]:
        """获取线程信息字典 ((pid, tid) -> name)"""
        return self._thread_name_parser.thread_info
    
    def handle_event(
        self, 
        raw: RawEvent, 
        ext_event: int,
        timestamp_ns: int = 0,
        **kwargs
    ) -> Optional[Any]:
        """
        处理进程事件
        
        Args:
            raw: 原始事件数据
            ext_event: 进程事件类型
            timestamp_ns: 已计算的 UTC 时间戳
        """
        event = None
        
        if ext_event == ProcessEventType.PROCCREATE:
            event = self._create_parser.parse(raw, has_name=False)
        
        elif ext_event == ProcessEventType.PROCCREATE_NAME:
            event = self._create_parser.parse(raw, has_name=True)
        
        elif ext_event == ProcessEventType.PROCDESTROY:
            event = self._destroy_parser.parse(raw, has_name=False)
        
        elif ext_event == ProcessEventType.PROCDESTROY_NAME:
            event = self._destroy_parser.parse(raw, has_name=True)
        
        elif ext_event == ProcessEventType.PROCTHREAD_NAME:
            event = self._thread_name_parser.parse(raw)
        
        # 设置时间戳
        if event:
            event.timestamp_ns = timestamp_ns
        
        return event
    
    def get_process_name(self, pid: int) -> str:
        """获取进程名称"""
        info = self.process_info
        return info.get(pid, f"unknown:{pid}")
    
    def get_thread_name(self, pid: int, tid: int) -> str:
        """获取线程名称"""
        if (pid, tid) in self.thread_info:
            return self.thread_info[(pid, tid)]
        
        info = self.process_info
        if pid in info:
            name = info[pid]
            return name if tid == 1 else f"{name}:{tid}"
        
        return f"unknown:{pid}:{tid}"
