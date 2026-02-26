"""
线程状态事件写入器

将线程状态事件写入为 Slice Events
通过计算相邻状态之间的时间差来确定每个状态的持续时间
"""

from typing import List, Dict, Tuple, TYPE_CHECKING
from collections import defaultdict

from .tracks import TrackManager
from .utils import write_slice, write_instant, format_policy

if TYPE_CHECKING:
    from perfetto.trace_builder.proto_builder import TraceProtoBuilder


class ThreadEventWriter:
    """线程状态事件写入器"""
    
    def __init__(self, builder: "TraceProtoBuilder", track_manager: TrackManager):
        self._builder = builder
        self._tracks = track_manager
        self._slice_count = 0
    
    def write_thread_states(self, events: List) -> int:
        """
        写入线程状态事件为 Slices
        
        按 (pid, tid) 分组，计算每个状态的持续时间
        """
        self._slice_count = 0
        
        if not events:
            return 0
        
        # 按 (pid, tid) 分组
        events_by_thread: Dict[Tuple[int, int], List] = defaultdict(list)
        for event in events:
            events_by_thread[(event.pid, event.tid)].append(event)
        
        # 每个线程内按时间排序并生成 slices
        for (pid, tid), thread_events in events_by_thread.items():
            track_uuid = self._tracks.get_thread_track(pid, tid)
            if not track_uuid:
                print(f"Warning: No track found for thread {pid}/{tid}")
                continue
            
            # 按时间戳排序
            thread_events.sort(key=lambda e: e.timestamp_ns)
            
            # 计算每个状态的持续时间
            for i, event in enumerate(thread_events):
                # 计算 duration
                if i + 1 < len(thread_events):
                    next_ts = thread_events[i + 1].timestamp_ns
                    duration = next_ts - event.timestamp_ns
                else:
                    # 最后一个状态，使用默认 duration
                    duration = 1000  # 1us
                
                # 确保 duration > 0
                if duration <= 0:
                    duration = 100  # 100ns minimum
                
                end_ts = event.timestamp_ns + duration
                
                # 构建 debug annotations
                # 格式: 进程名 (pid), 线程名 (tid)
                process_name = self._tracks.format_process_name(event.pid)
                thread_name = self._tracks.format_thread_name(event.pid, event.tid)
                
                args = {
                    "process": process_name,
                    "thread": thread_name,
                    "pid": event.pid,
                    "tid": event.tid,
                    "cpu": event.cpu_id,
                    "priority": getattr(event, 'priority', "unknown"),
                    "policy": format_policy(getattr(event, 'policy', 0)),
                    "partition_id": getattr(event, 'partition_id', "useless"),
                    "sched_flags": getattr(event, 'sched_flags', "useless"),
                    "is_vthread": getattr(event, 'is_vthread', False),
                    "is_wide": getattr(event, 'is_wide', False),
                }
                
                # 写入 slice
                write_slice(
                    self._builder,
                    event.timestamp_ns,
                    end_ts,
                    track_uuid,
                    event.state,
                    args
                )
                self._slice_count += 1
        
        return self._slice_count
    
    @property
    def event_count(self) -> int:
        return self._slice_count
