"""
QST Parser - IntEnter 事件解析器

参考文档: kercall_table_Events.html

_NTO_TRACE_INTENTER:
    Fast: interrupt_number, ip
    Wide: interrupt_number, ip
    
_NTO_TRACE_INTENTER_64:
    Fast: interrupt_number, empty, ip (64)
    Wide: interrupt_number, empty, ip (64)
    
64位 Wide 模式下，IP 地址通过组合事件传递：
    COMBINE_BEGIN: timestamp, interrupt_number, empty
    COMBINE_END: timestamp, ip_low, ip_high
"""

from typing import Optional, Dict, Any
from ....models.base import RawEvent
from ....constants import StructType
from ..models import InterruptEnterEvent


class IntEnterEventParser:
    """中断进入事件解析器"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._combine_buffers: Dict[str, Dict[str, Any]] = {}
    
    def parse(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        interrupt_number: int,
        is_64bit: bool = False
    ) -> Optional[InterruptEnterEvent]:
        """
        解析中断进入事件
        
        Args:
            raw: 原始事件数据
            timestamp_ns: 已计算的时间戳
            interrupt_number: 中断号（从外部事件号获取）
            is_64bit: 是否为 64 位模式
        
        Returns:
            InterruptEnterEvent 或 None（组合事件未完成时）
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            # Fast 模式 或 32位 Wide 模式（单个事件）
            if is_64bit:
                # 64-bit Fast: interrupt_number, empty, ip (64)
                # data[1] = interrupt_number, data[2] = empty
                # 但 Fast 模式下 64-bit IP 不可用，只能用 0
                return InterruptEnterEvent(
                    timestamp_cycles=raw.data[0],
                    timestamp_ns=timestamp_ns,
                    cpu_id=raw.cpu_id,
                    interrupt_number=interrupt_number,
                    ip=0,  # Fast 模式下无法获取完整的 64 位 IP
                    is_64bit=True,
                    is_wide=False,
                )
            else:
                # 32-bit: interrupt_number, ip
                return InterruptEnterEvent(
                    timestamp_cycles=raw.data[0],
                    timestamp_ns=timestamp_ns,
                    cpu_id=raw.cpu_id,
                    interrupt_number=interrupt_number,
                    ip=raw.data[2],
                    is_64bit=False,
                    is_wide=False,
                )
        
        # Wide 模式组合事件
        return self._parse_wide(raw, timestamp_ns, interrupt_number, is_64bit, struct_type)
    
    def _parse_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        interrupt_number: int,
        is_64bit: bool,
        struct_type: int
    ) -> Optional[InterruptEnterEvent]:
        """解析 Wide 模式组合事件"""
        key = f"intenter_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            # COMBINE_BEGIN: timestamp, interrupt_number, empty (64-bit) 或 ip (32-bit)
            self._combine_buffers[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'interrupt_number': interrupt_number,
                'is_64bit': is_64bit,
                'data': [raw.data[1], raw.data[2]],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            # COMBINE_CONT: 中间数据
            matching_key = self._find_key(raw, "intenter_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            # COMBINE_END: 最后的数据
            matching_key = self._find_key(raw, "intenter_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['data'].extend([raw.data[1], raw.data[2]])
                
                # 解析 IP
                if buf['is_64bit']:
                    # 64-bit Wide: ip_low, ip_high
                    data = buf['data']
                    ip = data[-2] | (data[-1] << 32) if len(data) >= 2 else 0
                else:
                    # 32-bit Wide: ip
                    ip = buf['data'][1] if len(buf['data']) > 1 else 0
                
                event = InterruptEnterEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    interrupt_number=buf['interrupt_number'],
                    ip=ip,
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
