"""
QST Parser - 线程状态事件解析器

解析所有线程状态变化事件，支持 Fast 和 Wide 模式。

参考文档: kercall_table_Events.html
    Class: _NTO_TRACE_THREAD
    Event: _NTO_TRACE_TH*
    Fast: pid, tid
    Wide: pid, tid, priority, policy, partition id, sched_flags
    
    注意: partition_id 和 sched_flags 仅在 APS 调度器加载时存在
"""

from typing import Optional
from ....models.base import RawEvent
from ....constants import StructType
from ..models import ThreadEvent
from ..constants import THREAD_STATES


class ThreadStateEventParser:
    """
    线程状态事件解析器
    
    解析 _NTO_TRACE_THREAD 类的所有状态事件。
    
    Fast mode 数据布局:
        - data[0]: timestamp (cycles)
        - data[1]: pid
        - data[2]: tid
    
    Wide mode 数据布局 (组合事件):
        无 APS 调度器时 (2个事件):
            COMBINE_BEGIN:
                - data[0]: timestamp
                - data[1]: pid
                - data[2]: tid
            COMBINE_END:
                - data[0]: timestamp
                - data[1]: priority
                - data[2]: policy
        
        有 APS 调度器时 (3个事件):
            COMBINE_BEGIN:
                - data[0]: timestamp
                - data[1]: pid
                - data[2]: tid
            COMBINE_CONT:
                - data[0]: timestamp
                - data[1]: priority
                - data[2]: policy
            COMBINE_END:
                - data[0]: timestamp
                - data[1]: partition_id
                - data[2]: sched_flags
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._combine_buffer = {}
    
    def parse(
        self,
        raw: RawEvent,
        state_code: int,
        is_vthread: bool = False
    ) -> Optional[ThreadEvent]:
        """
        解析线程状态事件
        
        Args:
            raw: 原始事件数据
            state_code: 状态码
            is_vthread: 是否为虚拟线程
        
        Returns:
            ThreadEvent 对象，或 None（如果是组合事件的中间部分）
        """
        struct_type = raw.struct_type
        state = THREAD_STATES.get(state_code, f"STATE_{state_code}")
        
        if struct_type == StructType.SIMPLE:
            # Fast mode
            return self._parse_fast_mode(raw, state, is_vthread)
        else:
            # Wide mode (组合事件)
            return self._parse_wide_mode(raw, state, is_vthread, struct_type)
    
    def _parse_fast_mode(
        self,
        raw: RawEvent,
        state: str,
        is_vthread: bool
    ) -> ThreadEvent:
        """解析 Fast mode 事件"""
        return ThreadEvent(
            timestamp_cycles=raw.data[0],
            cpu_id=raw.cpu_id,
            pid=raw.data[1],
            tid=raw.data[2],
            state=state,
            is_vthread=is_vthread,
            is_wide=False,
        )
    
    def _parse_wide_mode(
        self,
        raw: RawEvent,
        state: str,
        is_vthread: bool,
        struct_type: int
    ) -> Optional[ThreadEvent]:
        """解析 Wide mode 事件（组合事件）"""
        key = f"thread_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            # COMBINE_BEGIN: pid, tid
            self._combine_buffer[key] = {
                'timestamp': raw.data[0],
                'cpu_id': raw.cpu_id,
                'pid': raw.data[1],
                'tid': raw.data[2],
                'state': state,
                'is_vthread': is_vthread,
                'priority': 0,
                'policy': 0,
                'partition_id': 0,
                'sched_flags': 0,
                'has_cont': False,  # 标记是否收到 CONT
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            # COMBINE_CONT: priority, policy (仅当 APS 调度器加载时存在)
            matching_key = self._find_matching_key(raw)
            if matching_key:
                buf = self._combine_buffer[matching_key]
                buf['priority'] = raw.data[1]
                buf['policy'] = raw.data[2]
                buf['has_cont'] = True
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_matching_key(raw)
            if matching_key:
                buf = self._combine_buffer[matching_key]
                
                if buf['has_cont']:
                    # 有 CONT: END 包含 partition_id, sched_flags
                    buf['partition_id'] = raw.data[1]
                    buf['sched_flags'] = raw.data[2]
                else:
                    # 无 CONT: END 直接包含 priority, policy
                    buf['priority'] = raw.data[1]
                    buf['policy'] = raw.data[2]
                
                event = ThreadEvent(
                    timestamp_cycles=buf['timestamp'],
                    cpu_id=buf['cpu_id'],
                    pid=buf['pid'],
                    tid=buf['tid'],
                    state=buf['state'],
                    is_vthread=buf['is_vthread'],
                    is_wide=True,
                    priority=buf['priority'],
                    policy=buf['policy'],
                    partition_id=buf['partition_id'],
                    sched_flags=buf['sched_flags'],
                )
                
                del self._combine_buffer[matching_key]
                return event
        
        return None
    
    def _find_matching_key(self, raw: RawEvent) -> Optional[str]:
        """查找匹配时间戳的组合事件 key"""
        prefix = f"thread_{raw.cpu_id}_"
        for k in self._combine_buffer.keys():
            if k.startswith(prefix):
                buf = self._combine_buffer[k]
                if buf.get('timestamp') == raw.data[0]:
                    return k
        return None
