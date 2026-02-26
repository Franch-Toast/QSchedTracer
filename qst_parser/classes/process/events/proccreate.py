"""
QST Parser - ProcCreate 事件解析器

解析 PROCCREATE 和 PROCCREATE_NAME 事件。

参考文档: kercall_table_Events.html

PROCCREATE:
    Fast: ppid, pid
    Wide: ppid, pid

PROCCREATE_NAME:
    Fast: ppid, pid, name
    Wide: ppid, pid, name (组合事件)
"""

import struct
from typing import Dict, Optional
from ....models.base import RawEvent
from ....constants import StructType
from ..models import ProcessCreateEvent


class ProcCreateEventParser:
    """
    ProcCreate 事件解析器
    
    解析进程创建事件，提取进程名称。
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._combine_buffer: Dict[str, dict] = {}
        self.process_info: Dict[int, str] = {}  # pid -> name
    
    def parse(self, raw: RawEvent, has_name: bool = False) -> Optional[ProcessCreateEvent]:
        """
        解析 ProcCreate 事件
        
        Args:
            raw: 原始事件数据
            has_name: 是否是 PROCCREATE_NAME 事件
        
        Returns:
            ProcessCreateEvent 对象，或 None
        """
        struct_type = raw.struct_type
        
        if not has_name:
            # PROCCREATE 简单事件
            return ProcessCreateEvent(
                timestamp_cycles=raw.data[0],
                cpu_id=raw.cpu_id,
                ppid=raw.data[1],
                pid=raw.data[2],
            )
        
        # PROCCREATE_NAME 组合事件
        return self._parse_with_name(raw, struct_type)
    
    def _parse_with_name(
        self, 
        raw: RawEvent, 
        struct_type: int
    ) -> Optional[ProcessCreateEvent]:
        """解析带名称的进程创建事件"""

        if struct_type == StructType.SIMPLE:
            # 简单事件，没有额外名称数据
            return ProcessCreateEvent(
                timestamp_cycles=raw.data[0],
                cpu_id=raw.cpu_id,
                ppid=raw.data[1],
                pid=raw.data[2],
            )

        key = f"proccreate_{raw.cpu_id}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffer[key] = {
                'timestamp': raw.data[0],
                'cpu_id': raw.cpu_id,
                'ppid': raw.data[1],
                'pid': raw.data[2],
                'name_bytes': bytearray(),
            }
            return None
        
        elif struct_type in (StructType.COMBINE_CONT, StructType.COMBINE_END):
            if key in self._combine_buffer:
                buf = self._combine_buffer[key]
                buf['name_bytes'].extend(struct.pack('<II', raw.data[1], raw.data[2]))
                
                if struct_type == StructType.COMBINE_END:
                    name = buf['name_bytes'].rstrip(b'\x00').decode('utf-8', errors='replace')
                    
                    event = ProcessCreateEvent(
                        timestamp_cycles=buf['timestamp'],
                        cpu_id=buf['cpu_id'],
                        ppid=buf['ppid'],
                        pid=buf['pid'],
                        name=name,
                    )
                    
                    self.process_info[buf['pid']] = name
                    
                    if self.verbose:
                        print(f"[PROCCREATE] PID={buf['pid']}, name={name}")
                    
                    del self._combine_buffer[key]
                    return event
        
        return None
