"""
QST Parser - ProcThreadName 事件解析器

解析 PROCTHREAD_NAME 事件。

参考文档: kercall_table_Events.html

PROCTHREAD_NAME:
    Fast: pid, tid, name
    Wide: pid, tid, name (组合事件)
"""

import struct
from typing import Dict, Optional, Tuple
from ....models.base import RawEvent
from ....constants import StructType
from ..models import ThreadNameEvent


class ProcThreadNameEventParser:
    """
    ProcThreadName 事件解析器
    
    解析线程命名事件。
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._combine_buffer: Dict[str, dict] = {}
        self.thread_info: Dict[Tuple[int, int], str] = {}  # (pid, tid) -> name
    
    def parse(self, raw: RawEvent) -> Optional[ThreadNameEvent]:
        """
        解析 ProcThreadName 事件
        
        Args:
            raw: 原始事件数据
        
        Returns:
            ThreadNameEvent 对象，或 None
        """
        struct_type = raw.struct_type
        if struct_type == StructType.SIMPLE:
            # 简单事件，没有额外名称数据
            return ThreadNameEvent(
                timestamp_cycles=raw.data[0],
                cpu_id=raw.cpu_id,
                pid=raw.data[1],
                tid=raw.data[2],
            )

        key = f"thread_{raw.cpu_id}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffer[key] = {
                'timestamp': raw.data[0],
                'cpu_id': raw.cpu_id,
                'pid': raw.data[1],
                'tid': raw.data[2],
                'name_bytes': bytearray(),
            }
            return None
        
        elif struct_type in (StructType.COMBINE_CONT, StructType.COMBINE_END):
            if key in self._combine_buffer:
                buf = self._combine_buffer[key]
                buf['name_bytes'].extend(struct.pack('<II', raw.data[1], raw.data[2]))
                
                if struct_type == StructType.COMBINE_END:
                    name = buf['name_bytes'].rstrip(b'\x00').decode('utf-8', errors='replace')
                    
                    event = ThreadNameEvent(
                        timestamp_cycles=buf['timestamp'],
                        cpu_id=buf['cpu_id'],
                        pid=buf['pid'],
                        tid=buf['tid'],
                        name=name,
                    )
                    
                    self.thread_info[(buf['pid'], buf['tid'])] = name
                    
                    if self.verbose:
                        print(f"[PROCTHREAD_NAME] PID={buf['pid']}, TID={buf['tid']}, name={name}")
                    
                    del self._combine_buffer[key]
                    return event
        
        return None
