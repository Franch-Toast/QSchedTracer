"""
QST Parser - IntHandler 事件解析器

参考文档: kercall_table_Events.html

_NTO_TRACE_INT_HANDLER_ENTER:
    Fast: pid, interrupt_number, ip, area
    Wide: pid, interrupt_number, ip, area

_NTO_TRACE_INT_HANDLER_ENTER_64:
    Fast: pid, interrupt_number, ip (64), area (64)
    Wide: pid, interrupt_number, ip (64), area (64)
    
    64位 Wide 模式下数据通过组合事件传递:
    COMBINE_BEGIN: timestamp, pid, interrupt_number
    COMBINE_CONT:  timestamp, ip_low, ip_high
    COMBINE_END:   timestamp, area_low, area_high

_NTO_TRACE_INT_HANDLER_EXIT:
    Fast: interrupt_number, sigevent
    Wide: interrupt_number, sigevent
"""

from typing import Optional, Dict, Any
from ....models.base import RawEvent
from ....constants import StructType
from ..models import InterruptHandlerEnterEvent, InterruptHandlerExitEvent


class IntHandlerEventParser:
    """中断处理器事件解析器"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._combine_buffers: Dict[str, Dict[str, Any]] = {}
    
    def parse_enter(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        interrupt_number: int,
        is_64bit: bool = False
    ) -> Optional[InterruptHandlerEnterEvent]:
        """
        解析中断处理器进入事件
        
        Args:
            raw: 原始事件数据
            timestamp_ns: 已计算的时间戳
            interrupt_number: 中断号（从外部事件号获取）
            is_64bit: 是否为 64 位模式
        
        Returns:
            InterruptHandlerEnterEvent 或 None（组合事件未完成时）
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            # Fast 模式
            if is_64bit:
                # 64-bit Fast: pid, interrupt_number, ip (64), area (64)
                # 无法在单个 SIMPLE 事件中完整获取 64 位数据
                return InterruptHandlerEnterEvent(
                    timestamp_cycles=raw.data[0],
                    timestamp_ns=timestamp_ns,
                    cpu_id=raw.cpu_id,
                    interrupt_number=interrupt_number,
                    pid=raw.data[1],
                    ip=0,
                    area=0,
                    is_64bit=True,
                    is_wide=False,
                )
            else:
                # 32-bit: pid, interrupt_number, ip, area
                return InterruptHandlerEnterEvent(
                    timestamp_cycles=raw.data[0],
                    timestamp_ns=timestamp_ns,
                    cpu_id=raw.cpu_id,
                    interrupt_number=interrupt_number,
                    pid=raw.data[1],
                    ip=raw.data[2],
                    area=0,  # area 可能在后续数据中
                    is_64bit=False,
                    is_wide=False,
                )
        
        # Wide 模式组合事件
        return self._parse_enter_wide(raw, timestamp_ns, interrupt_number, is_64bit, struct_type)
    
    def _parse_enter_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        interrupt_number: int,
        is_64bit: bool,
        struct_type: int
    ) -> Optional[InterruptHandlerEnterEvent]:
        """解析 Handler Enter Wide 模式组合事件"""
        key = f"handler_enter_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            # COMBINE_BEGIN: timestamp, pid, interrupt_number
            self._combine_buffers[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'interrupt_number': interrupt_number,
                'is_64bit': is_64bit,
                'pid': raw.data[1],
                'data': [raw.data[2]],  # interrupt_number 或其他数据
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            # COMBINE_CONT: 中间数据
            matching_key = self._find_key(raw, "handler_enter_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            # COMBINE_END: 最后的数据
            matching_key = self._find_key(raw, "handler_enter_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['data'].extend([raw.data[1], raw.data[2]])
                
                data = buf['data']
                if buf['is_64bit']:
                    # 64-bit: ip (64), area (64)
                    # data 排列: interrupt_number, ip_low, ip_high, area_low, area_high
                    ip = (data[1] | (data[2] << 32)) if len(data) > 2 else 0
                    area = (data[3] | (data[4] << 32)) if len(data) > 4 else 0
                else:
                    # 32-bit: ip, area
                    ip = data[1] if len(data) > 1 else 0
                    area = data[2] if len(data) > 2 else 0
                
                event = InterruptHandlerEnterEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    interrupt_number=buf['interrupt_number'],
                    pid=buf['pid'],
                    ip=ip,
                    area=area,
                    is_64bit=buf['is_64bit'],
                    is_wide=True,
                )
                
                del self._combine_buffers[matching_key]
                return event
        
        return None
    
    def parse_exit(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        interrupt_number: int
    ) -> InterruptHandlerExitEvent:
        """
        解析中断处理器退出事件
        
        Args:
            raw: 原始事件数据
            timestamp_ns: 已计算的时间戳
            interrupt_number: 中断号（从外部事件号获取）
        
        Returns:
            InterruptHandlerExitEvent
        """
        # INT_HANDLER_EXIT 的 Fast 和 Wide 模式格式相同
        # data[1] = interrupt_number, data[2] = sigevent
        return InterruptHandlerExitEvent(
            timestamp_cycles=raw.data[0],
            timestamp_ns=timestamp_ns,
            cpu_id=raw.cpu_id,
            interrupt_number=interrupt_number,
            sigevent=raw.data[2],
            is_64bit=False,
            is_wide=(raw.struct_type != StructType.SIMPLE),
        )
    
    def _find_key(self, raw: RawEvent, prefix: str) -> Optional[str]:
        """查找匹配的组合事件 key"""
        search_prefix = f"{prefix}{raw.cpu_id}_"
        for k in self._combine_buffers.keys():
            if k.startswith(search_prefix):
                buf = self._combine_buffers[k]
                if buf.get('timestamp_cycles') == raw.data[0]:
                    return k
        return None
