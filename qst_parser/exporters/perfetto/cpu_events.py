"""
CPU 事件写入器

将 CPU Running Thread 和中断事件写入为 Slice Events
"""

from typing import List, Dict, Tuple, TYPE_CHECKING
from collections import defaultdict

from .tracks import TrackManager
from .utils import write_slice, write_instant, format_policy

if TYPE_CHECKING:
    from perfetto.trace_builder.proto_builder import TraceProtoBuilder


class CpuEventWriter:
    """CPU 事件写入器"""
    
    def __init__(self, builder: "TraceProtoBuilder", track_manager: TrackManager):
        self._builder = builder
        self._tracks = track_manager
        self._running_count = 0
        self._interrupt_slice_count = 0
        self._interrupt_instant_count = 0
    
    def write_running_threads(self, events: List) -> int:
        """
        写入 CPU 上运行的线程事件为 Slices
        
        只处理 RUNNING 状态的事件，计算每个 CPU 上的线程运行持续时间
        """
        self._running_count = 0
        
        if not events:
            return 0
        
        # 筛选 RUNNING 事件并按 CPU 分组
        running_events_by_cpu: Dict[int, List] = defaultdict(list)
        for event in events:
            if event.state == "RUNNING":
                running_events_by_cpu[event.cpu_id].append(event)
        
        # 每个 CPU 内按时间排序并生成 slices
        for cpu_id, cpu_events in running_events_by_cpu.items():
            track_uuid = self._tracks.get_cpu_track(cpu_id)
            if not track_uuid:
                print(f"Warning: No track found for cpu {cpu_id}")
                continue
            
            # 按时间戳排序
            cpu_events.sort(key=lambda e: e.timestamp_ns)
            
            # 计算每个 RUNNING 事件的持续时间
            for i, event in enumerate(cpu_events):
                # 计算 duration（到下一个 RUNNING 事件的时间）
                if i + 1 < len(cpu_events):
                    next_ts = cpu_events[i + 1].timestamp_ns
                    duration = next_ts - event.timestamp_ns
                else:
                    # 最后一个，使用默认 duration
                    duration = 1000  # 1us
                
                # 确保 duration > 0
                if duration <= 0:
                    duration = 100  # 100ns minimum
                
                end_ts = event.timestamp_ns + duration
                
                # 获取线程名（用于 slice 名称）
                thread_name = self._tracks.get_thread_name(event.pid, event.tid)
                
                # 构建 debug annotations
                # 使用格式化的名称：进程/线程名称 (pid/tid)
                process_name = self._tracks.format_process_name(event.pid)
                thread_full_name = self._tracks.format_thread_name(event.pid, event.tid)
                
                args = {
                    "process": process_name,
                    "thread": thread_full_name,
                    "priority": getattr(event, 'priority', 0),
                    "policy": format_policy(getattr(event, 'policy', 0)),
                    "partition_id": getattr(event, 'partition_id', 0),
                }
                
                # 写入 slice
                write_slice(
                    self._builder,
                    event.timestamp_ns,
                    end_ts,
                    track_uuid,
                    thread_name,
                    args
                )
                self._running_count += 1
        
        return self._running_count
    
    def write_interrupts(self, enters: List, exits: List) -> int:
        """
        写入中断事件
        
        尝试配对 ENTER/EXIT 生成 Slices，未配对的作为 Instant Events
        """
        self._interrupt_slice_count = 0
        self._interrupt_instant_count = 0
        
        if not enters and not exits:
            return 0
        
        # 按 (cpu_id, interrupt_number) 分组
        enters_by_key: Dict[Tuple[int, int], List] = defaultdict(list)
        exits_by_key: Dict[Tuple[int, int], List] = defaultdict(list)
        
        for e in enters:
            enters_by_key[(e.cpu_id, e.interrupt_number)].append(e)
        for e in exits:
            exits_by_key[(e.cpu_id, e.interrupt_number)].append(e)
        
        # 配对并写入
        all_keys = set(enters_by_key.keys()) | set(exits_by_key.keys())
        
        for key in all_keys:
            cpu_id, irq_num = key
            track_uuid = self._tracks.get_or_create_cpu_irq_track(cpu_id)
            if not track_uuid:
                continue
            
            key_enters = sorted(enters_by_key.get(key, []), key=lambda e: e.timestamp_ns)
            key_exits = sorted(exits_by_key.get(key, []), key=lambda e: e.timestamp_ns)
            
            # 尝试配对
            enter_idx = 0
            exit_idx = 0
            
            while enter_idx < len(key_enters):
                enter = key_enters[enter_idx]
                
                # 找到对应的 EXIT（时间戳大于 ENTER）
                matched_exit = None
                while exit_idx < len(key_exits):
                    exit = key_exits[exit_idx]
                    if exit.timestamp_ns > enter.timestamp_ns:
                        matched_exit = exit
                        exit_idx += 1
                        break
                    exit_idx += 1
                
                if matched_exit:
                    # 生成 slice
                    args = {
                        "irq": irq_num,
                        "cpu": cpu_id,
                    }
                    if hasattr(enter, 'ip') and enter.ip:
                        args["ip"] = hex(enter.ip)
                    if hasattr(matched_exit, 'kernel_flag'):
                        args["kernel_flag"] = matched_exit.kernel_flag
                    
                    write_slice(
                        self._builder,
                        enter.timestamp_ns,
                        matched_exit.timestamp_ns,
                        track_uuid,
                        f"IRQ{irq_num}",
                        args
                    )
                    self._interrupt_slice_count += 1
                else:
                    # 无法配对，写入 ENTER 作为 instant
                    args = {
                        "type": "ENTER",
                        "irq": irq_num,
                        "cpu": cpu_id,
                    }
                    write_instant(self._builder, enter.timestamp_ns, track_uuid, 
                                  f"IRQ{irq_num}_ENTER", args)
                    self._interrupt_instant_count += 1
                
                enter_idx += 1
            
            # 处理剩余未配对的 EXIT
            while exit_idx < len(key_exits):
                exit = key_exits[exit_idx]
                args = {
                    "type": "EXIT",
                    "irq": exit.interrupt_number,
                    "cpu": cpu_id,
                }
                write_instant(self._builder, exit.timestamp_ns, track_uuid, 
                              f"IRQ{exit.interrupt_number}_EXIT", args)
                self._interrupt_instant_count += 1
                exit_idx += 1
        
        return self._interrupt_slice_count + self._interrupt_instant_count
    
    @property
    def running_count(self) -> int:
        return self._running_count
    
    @property
    def interrupt_slice_count(self) -> int:
        return self._interrupt_slice_count
    
    @property
    def interrupt_instant_count(self) -> int:
        return self._interrupt_instant_count
    
    # 兼容旧接口
    @property
    def interrupt_enter_count(self) -> int:
        return self._interrupt_slice_count
    
    @property
    def interrupt_exit_count(self) -> int:
        return self._interrupt_instant_count
