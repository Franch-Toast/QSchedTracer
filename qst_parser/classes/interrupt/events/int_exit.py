"""
QST Parser - IntExit 事件解析器

参考文档: kercall_table_Events.html

_NTO_TRACE_INTEXIT:
    Fast: interrupt_number, kernel_flag
    Wide: interrupt_number, kernel_flag
    
kernel_flag 含义:
    - 0: 中断在用户态发生
    - 非0: 中断在内核态发生
"""

from typing import Optional
from ....models.base import RawEvent
from ....constants import StructType
from ..models import InterruptExitEvent


class IntExitEventParser:
    """中断退出事件解析器"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
    
    def parse(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        interrupt_number: int
    ) -> Optional[InterruptExitEvent]:
        """
        解析中断退出事件
        
        Args:
            raw: 原始事件数据
            timestamp_ns: 已计算的时间戳
            interrupt_number: 中断号（从外部事件号获取）
        
        Returns:
            InterruptExitEvent
        """
        # INTEXIT 的 Fast 和 Wide 模式格式相同
        # data[1] = interrupt_number, data[2] = kernel_flag
        return InterruptExitEvent(
            timestamp_cycles=raw.data[0],
            timestamp_ns=timestamp_ns,
            cpu_id=raw.cpu_id,
            interrupt_number=interrupt_number,
            kernel_flag=raw.data[2],
            is_64bit=False,
            is_wide=(raw.struct_type != StructType.SIMPLE),
        )
