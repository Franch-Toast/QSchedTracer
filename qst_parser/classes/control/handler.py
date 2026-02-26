"""
QST Parser - Control 类事件处理器实现

处理 trace 控制事件。
"""

from typing import Optional, Any, List
from ..base import BaseClassHandler
from ...models.base import RawEvent
from .constants import ControlEventType


class ControlClassHandler(BaseClassHandler):
    """
    Control 类事件处理器
    
    处理 _NTO_TRACE_CONTROL 事件:
        - CONTROLTIME: 时间同步 (msbtime, lsbtime)
        - CONTROLBUFFER: 缓冲区信息 (buffer_seq, num_events)
    
    buffer_seq 连续性检查:
        - 正常情况下 buffer_seq 应该连续递增
        - 如果出现跳跃，说明可能有缓冲区数据丢失
    """
    
    def __init__(self, verbose: bool = False):
        super().__init__(verbose)
        self._last_buffer_seq: Optional[int] = None
        self._discontinuity_count: int = 0
        self._discontinuities: List[tuple] = []  # [(expected, actual, timestamp_ns), ...]
        self._total_buffer_count: int = 0
    
    @property
    def class_name(self) -> str:
        return "CONTROL"
    
    @property
    def discontinuity_count(self) -> int:
        """返回检测到的不连续次数"""
        return self._discontinuity_count
    
    @property
    def discontinuities(self) -> List[tuple]:
        """返回所有不连续事件的详情"""
        return self._discontinuities
    
    @property
    def total_buffer_count(self) -> int:
        """返回处理的缓冲区总数"""
        return self._total_buffer_count
    
    def handle_event(
        self, 
        raw: RawEvent, 
        ext_event: int,
        timestamp_ns: int = 0,
        **kwargs
    ) -> Optional[Any]:
        """
        处理控制事件
        
        Args:
            raw: 原始事件数据
            ext_event: 控制事件类型
            timestamp_ns: 已计算的 UTC 时间戳
        
        Returns:
            目前控制事件不产生输出，返回 None
        """
        if ext_event == ControlEventType.CONTROLTIME:
            msbtime = raw.data[1]
            lsbtime = raw.data[2]
            self.log(f"CONTROLTIME: msb={msbtime}, lsb={lsbtime}")
        
        elif ext_event == ControlEventType.CONTROLBUFFER:
            # CONTROLBUFFER 数据布局 (参考 samples_parser.html):
            # data[0] = timestamp (cycles)
            # data[1] = buffer sequence number (buffer_seq)
            # data[2] = number of events (num_events)
            # 数据是小端序，由 struct.unpack('<I', ...) 正确解析
            buffer_seq = raw.data[1]  # sequence number
            num_events = raw.data[2]  # number of events
            
            self._total_buffer_count += 1
            
            # 检查 buffer_seq 连续性
            if self._last_buffer_seq is not None:
                expected_seq = self._last_buffer_seq + 1
                if buffer_seq != expected_seq:
                    self._discontinuity_count += 1
                    self._discontinuities.append((expected_seq, buffer_seq, timestamp_ns))
                    
                    # 计算跳过的缓冲区数量
                    if buffer_seq > expected_seq:
                        skipped = buffer_seq - expected_seq
                        print(f"[WARN] CONTROLBUFFER 不连续! "
                              f"期望 seq={expected_seq}, 实际 seq={buffer_seq}, "
                              f"跳过了 {skipped} 个缓冲区, "
                              f"events={num_events} (timestamp_ns={timestamp_ns})")
                    else:
                        # buffer_seq 回绕或乱序
                        print(f"[WARN] CONTROLBUFFER 异常! "
                              f"期望 seq={expected_seq}, 实际 seq={buffer_seq} "
                              f"(可能是序号回绕或乱序, timestamp_ns={timestamp_ns})")
            
            self._last_buffer_seq = buffer_seq
            self.log(f"CONTROLBUFFER: seq={buffer_seq}, events={num_events}")
        
        return None
    
    def get_summary(self) -> dict:
        """获取缓冲区连续性检查摘要"""
        return {
            "total_buffers": self._total_buffer_count,
            "discontinuities": self._discontinuity_count,
            "first_seq": self._discontinuities[0][1] if self._discontinuities else None,
            "last_seq": self._last_buffer_seq,
            "is_continuous": self._discontinuity_count == 0,
        }