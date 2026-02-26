"""
QST Parser - SignalKill 事件解析器

解析 __KER_SIGNAL_KILL 和 __KER_SIGNAL_KILL_SIGVAL 内核调用事件。

参考文档: kercall_table_Events.html

__KER_SIGNAL_KILL:
    Class: _NTO_TRACE_KERCALLENTER
    Fast: pid, signo
    Wide: nd, pid, tid, signo, code, value
    
__KER_SIGNAL_KILL_SIGVAL:
    Class: _NTO_TRACE_KERCALLENTER
    Fast: pid, signo
    Wide: nd, pid, tid, signo, code, value
    Wide (64-bit): nd, pid, tid, signo, code, value (64)
"""

from typing import Dict, List, Optional

from ....models.base import RawEvent
from ....constants import StructType
from ..models import SignalKillEvent


class SignalKillEventParser:
    """
    SignalKill 事件解析器
    
    解析信号发送相关的内核调用事件，支持 Fast 和 Wide 模式。
    时间戳由 parser 实时计算并传入。
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._combine_buffer: Dict[str, dict] = {}
        self.signal_events: List[SignalKillEvent] = []
    
    def parse(
        self, 
        raw: RawEvent,
        timestamp_ns: int = 0
    ) -> Optional[SignalKillEvent]:
        """
        解析 SignalKill 事件
        
        Args:
            raw: 原始事件数据
            timestamp_ns: 已计算的 UTC 时间戳
        
        Returns:
            SignalKillEvent 对象，或 None
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            return self._parse_fast_mode(raw, timestamp_ns)
        else:
            return self._parse_wide_mode(raw, timestamp_ns, struct_type)
    
    def _parse_fast_mode(
        self, 
        raw: RawEvent,
        timestamp_ns: int
    ) -> Optional[SignalKillEvent]:
        """
        解析 Fast mode 事件
        
        数据布局:
            - data[0]: timestamp
            - data[1]: pid
            - data[2]: signo
        """
        signo = raw.data[2]
        
        # 过滤信号 0 (null signal，仅用于检查进程是否存在)
        if signo == 0:
            return None
        
        event = SignalKillEvent(
            timestamp_cycles=raw.data[0],
            timestamp_ns=timestamp_ns,
            cpu_id=raw.cpu_id,
            target_pid=raw.data[1],
            signo=signo,
        )
        self.signal_events.append(event)
        
        if self.verbose:
            print(f"[SIGNAL_KILL] Fast mode: PID={event.target_pid}, "
                  f"SIG={event.signal_name}")
        
        return event
    
    def _parse_wide_mode(
        self, 
        raw: RawEvent,
        timestamp_ns: int,
        struct_type: int
    ) -> Optional[SignalKillEvent]:
        """
        解析 Wide mode 事件（组合事件）
        
        数据布局:
            COMBINE_BEGIN: timestamp, nd, pid
            COMBINE_CONT:  timestamp, tid, signo
            COMBINE_END:   timestamp, code, value
        """
        key = f"signalkill_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffer[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'nd': raw.data[1],
                'pid': raw.data[2],
                'tid': 0,
                'signo': 0,
                'code': 0,
                'value': 0,
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_matching_key(raw)
            if matching_key:
                buf = self._combine_buffer[matching_key]
                buf['tid'] = raw.data[1]
                buf['signo'] = raw.data[2]
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_matching_key(raw)
            if matching_key:
                buf = self._combine_buffer[matching_key]
                buf['code'] = raw.data[1]
                buf['value'] = raw.data[2]
                
                # 过滤信号 0
                if buf['signo'] == 0:
                    del self._combine_buffer[matching_key]
                    return None
                
                event = SignalKillEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    nd=buf['nd'],
                    target_pid=buf['pid'],
                    target_tid=buf['tid'],
                    signo=buf['signo'],
                    code=buf['code'],
                    value=buf['value'],
                )
                self.signal_events.append(event)
                
                if self.verbose:
                    print(f"[SIGNAL_KILL] Wide mode: PID={event.target_pid}, "
                          f"TID={event.target_tid}, SIG={event.signal_name}, "
                          f"code={event.code}, value={event.value}")
                
                del self._combine_buffer[matching_key]
                return event
        
        return None
    
    def _find_matching_key(self, raw: RawEvent) -> Optional[str]:
        """查找匹配时间戳的组合事件 key"""
        prefix = f"signalkill_{raw.cpu_id}_"
        for k in self._combine_buffer.keys():
            if k.startswith(prefix):
                buf = self._combine_buffer[k]
                if buf.get('timestamp_cycles') == raw.data[0]:
                    return k
        return None
    
    def get_events(self) -> List[SignalKillEvent]:
        """获取所有已解析的信号事件"""
        return self.signal_events
    
    def clear_events(self) -> None:
        """清空已解析的事件"""
        self.signal_events.clear()
        self._combine_buffer.clear()
