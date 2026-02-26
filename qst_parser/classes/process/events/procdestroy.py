"""
QST Parser - ProcDestroy 事件解析器

解析 PROCDESTROY 和 PROCDESTROY_NAME 事件。

参考文档: kercall_table_Events.html

PROCDESTROY:
    Fast: ppid, pid
    Wide: ppid, pid

PROCDESTROY_NAME:
    Fast: ppid, pid, name
    Wide: ppid, pid, name (组合事件)
"""

import struct
from typing import Dict, Optional
from ....models.base import RawEvent
from ....constants import StructType
from ..models import ProcessDestroyEvent


class ProcDestroyEventParser:
    """
    ProcDestroy 事件解析器
    
    解析进程销毁事件，提取进程名称。
    这是获取进程名称的主要来源。
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._combine_buffer: Dict[str, dict] = {}
        self.process_info: Dict[int, str] = {}  # pid -> name
    
    def parse(self, raw: RawEvent, has_name: bool = False) -> Optional[ProcessDestroyEvent]:
        """
        解析 ProcDestroy 事件
        
        Args:
            raw: 原始事件数据
            has_name: 是否是带名称的事件
        
        Returns:
            ProcessDestroyEvent 对象，或 None
        """
        struct_type = raw.struct_type
        
        # PROCDESTROY 通常是组合事件（即使是 PROCDESTROY 也可能带名称）
        return self._parse_with_name(raw, struct_type)
    
    def _parse_with_name(
        self, 
        raw: RawEvent, 
        struct_type: int
    ) -> Optional[ProcessDestroyEvent]:
        """解析带名称的进程销毁事件"""
        key = f"procdestroy_{raw.cpu_id}"
        
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
                    # 解析完整路径，只保留文件名
                    full_path = buf['name_bytes'].rstrip(b'\x00').decode('utf-8', errors='replace')
                    name = full_path.split('/')[-1] if '/' in full_path else full_path
                    name = name.split('\x00')[0].strip()
                    
                    event = ProcessDestroyEvent(
                        timestamp_cycles=buf['timestamp'],
                        cpu_id=buf['cpu_id'],
                        ppid=buf['ppid'],
                        pid=buf['pid'],
                        name=name,
                    )
                    
                    # 只有当进程名未知时才添加
                    if buf['pid'] not in self.process_info:
                        self.process_info[buf['pid']] = name
                        
                        if self.verbose:
                            print(f"[PROCDESTROY] PID={buf['pid']}, name={name}")
                    
                    del self._combine_buffer[key]
                    return event
        
        elif struct_type == StructType.SIMPLE:
            # 简单事件，没有名称
            return ProcessDestroyEvent(
                timestamp_cycles=raw.data[0],
                cpu_id=raw.cpu_id,
                ppid=raw.data[1],
                pid=raw.data[2],
            )
        
        return None
