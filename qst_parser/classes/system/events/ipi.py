"""
QST Parser - IPI (处理器间中断) 事件解析器

参考文档: kercall_table_Events.html

_NTO_TRACE_SYS_IPI (32-bit):
    Fast: ipicmd (32), ip (32), tid (32), pid (32)
    Wide: ipicmd (32), ip (32), tid (32), pid (32)
    
_NTO_TRACE_SYS_IPI_64 (64-bit):
    Fast: N/A
    Wide: ipicmd (32), pad (32), interrupted ip (64), tid (32), pid (32)
"""

from typing import Optional, Dict, Any
from ....models.base import RawEvent
from ....constants import StructType
from ..models import IPIEvent


class IPIEventParser:
    """IPI 事件解析器"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._combine_buffers: Dict[str, Dict[str, Any]] = {}
    
    def parse(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[IPIEvent]:
        """
        解析 IPI 事件
        
        Args:
            raw: 原始事件数据
            timestamp_ns: 已计算的时间戳
            is_64bit: 是否为 64 位模式
        
        Returns:
            IPIEvent 或 None（组合事件未完成时）
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            if is_64bit:
                # 64-bit Fast mode 不可用
                return IPIEvent(
                    timestamp_cycles=raw.data[0],
                    timestamp_ns=timestamp_ns,
                    cpu_id=raw.cpu_id,
                    ipicmd=raw.data[1],
                    ip=0,
                    tid=0,
                    pid=0,
                    is_64bit=True,
                    is_wide=False,
                )
            else:
                # 32-bit Fast: ipicmd, ip, tid, pid
                # 但 SIMPLE 只有 3 个数据槽位，可能数据不完整
                return IPIEvent(
                    timestamp_cycles=raw.data[0],
                    timestamp_ns=timestamp_ns,
                    cpu_id=raw.cpu_id,
                    ipicmd=raw.data[1],
                    ip=raw.data[2],
                    tid=0,  # 可能在后续数据中
                    pid=0,
                    is_64bit=False,
                    is_wide=False,
                )
        
        # Wide mode 组合事件
        return self._parse_wide(raw, timestamp_ns, is_64bit, struct_type)
    
    def _parse_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool,
        struct_type: int
    ) -> Optional[IPIEvent]:
        """解析 Wide mode 组合事件"""
        key = f"ipi_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffers[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'is_64bit': is_64bit,
                'data': [raw.data[1], raw.data[2]],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, "ipi_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, "ipi_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['data'].extend([raw.data[1], raw.data[2]])
                
                data = buf['data']
                if buf['is_64bit']:
                    # 64-bit Wide: ipicmd (32), pad (32), ip (64), tid (32), pid (32)
                    ipicmd = data[0] if len(data) > 0 else 0
                    # pad = data[1]
                    ip = (data[2] | (data[3] << 32)) if len(data) > 3 else 0
                    tid = data[4] if len(data) > 4 else 0
                    pid = data[5] if len(data) > 5 else 0
                else:
                    # 32-bit Wide: ipicmd, ip, tid, pid
                    ipicmd = data[0] if len(data) > 0 else 0
                    ip = data[1] if len(data) > 1 else 0
                    tid = data[2] if len(data) > 2 else 0
                    pid = data[3] if len(data) > 3 else 0
                
                event = IPIEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    ipicmd=ipicmd,
                    ip=ip,
                    tid=tid,
                    pid=pid,
                    is_64bit=buf['is_64bit'],
                    is_wide=True,
                )
                
                del self._combine_buffers[matching_key]
                return event
        
        return None
    
    def _find_key(self, raw: RawEvent, prefix: str) -> Optional[str]:
        """查找匹配的组合事件 key"""
        search_prefix = f"{prefix}{raw.cpu_id}_"
        for k in self._combine_buffers.keys():
            if k.startswith(search_prefix):
                buf = self._combine_buffers[k]
                if buf.get('timestamp_cycles') == raw.data[0]:
                    return k
        return None
