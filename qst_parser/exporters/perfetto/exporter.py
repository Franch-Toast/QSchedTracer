"""
Perfetto 主导出器

协调各个事件写入器，将 QST 数据导出为 Perfetto protobuf 格式

Track 结构：
- CPU Tracks: Running Thread slices
  - IRQ 子 Track: Interrupt slices / instants
- Thread Tracks: Thread State slices
  - SYNC 子 Track: 同步原语 instants (MutexLock, SemWait, CondvarWait)
  - IPC 子 Track: IPC slices/instants (MsgSend, MsgRecv, MsgReply)
"""

from typing import Dict, Any, List, Tuple, Set, TYPE_CHECKING
from dataclasses import dataclass
from collections import defaultdict

from perfetto.trace_builder.proto_builder import TraceProtoBuilder

from .tracks import TrackManager
from .thread_events import ThreadEventWriter
from .cpu_events import CpuEventWriter
from .sync_events import SyncEventWriter
from .ipc_events import IpcEventWriter
from .comm_events import CommEventWriter
from .system_events import SystemEventWriter

if TYPE_CHECKING:
    from ...parser import QstParser


@dataclass
class ExportStats:
    """导出统计"""
    cpu_tracks: int = 0
    process_tracks: int = 0
    thread_tracks: int = 0
    # 线程状态事件 (slices)
    thread_state_slices: int = 0
    thread_state_input: int = 0
    # CPU 运行事件 (slices)
    cpu_running_slices: int = 0
    cpu_running_input: int = 0
    # 中断事件
    interrupt_slices: int = 0
    interrupt_instants: int = 0
    interrupt_input: int = 0
    # 同步原语事件 (instants, 仅 ENTER)
    sync_enter: int = 0
    sync_exit: int = 0
    sync_input: int = 0
    # IPC 事件 (slices + instants)
    ipc_slices: int = 0
    ipc_instants: int = 0
    ipc_input: int = 0
    # 调度事件 (instants)
    sched_instants: int = 0
    sched_input: int = 0
    # Comm 事件
    comm_msg: int = 0
    comm_reply: int = 0
    comm_pulse: int = 0
    comm_signal: int = 0
    # System 事件
    sys_ipi: int = 0
    sys_pagewait: int = 0
    # 文件
    file_size_kb: float = 0.0
    
    def to_dict(self) -> Dict[str, Any]:
        return {k: v for k, v in self.__dict__.items()}


class PerfettoExporter:
    """Perfetto Protobuf 格式导出器"""
    
    def __init__(self, parser: "QstParser"):
        self._parser = parser
        self._builder = TraceProtoBuilder()
        self._stats = ExportStats()
        
        # Track 管理器
        self._track_manager = TrackManager(self._builder)
        
        # 提取数据（需要在创建事件写入器之前）
        self._extract_data()
        
        # 事件写入器
        self._thread_writer = ThreadEventWriter(self._builder, self._track_manager)
        self._cpu_writer = CpuEventWriter(self._builder, self._track_manager)
        self._sync_writer = SyncEventWriter(self._builder, self._track_manager)
        self._ipc_writer = IpcEventWriter(self._builder, self._track_manager)
        self._comm_writer = CommEventWriter(self._builder, self._track_manager)
        self._system_writer = SystemEventWriter(self._builder, self._track_manager)
    
    def _extract_data(self):
        """从 parser 提取数据"""
        # 进程/线程名称
        process_names = {}
        thread_names = {}
        
        if hasattr(self._parser, 'process_info'):
            process_names = dict(self._parser.process_info)
        if hasattr(self._parser, 'thread_info'):
            thread_names = dict(self._parser.thread_info)
        
        self._track_manager.set_names(process_names, thread_names)
        
        # 线程事件
        self._thread_events = []
        if hasattr(self._parser, '_thread_handler'):
            handler = self._parser._thread_handler
            if hasattr(handler, 'thread_events'):
                self._thread_events = list(handler.thread_events)
        
        # VThread 事件
        if hasattr(self._parser, '_vthread_handler'):
            handler = self._parser._vthread_handler
            if hasattr(handler, 'thread_events'):
                self._thread_events.extend(list(handler.thread_events))
        
        # 中断事件
        self._int_enters = []
        self._int_exits = []
        if hasattr(self._parser, '_interrupt_handler'):
            handler = self._parser._interrupt_handler
            self._int_enters = list(getattr(handler, 'enter_events', []))
            self._int_exits = list(getattr(handler, 'exit_events', []))
        
        # 各类 Handler
        self._kercall_handler = getattr(self._parser, '_kercall_handler', None)
        self._comm_handler = getattr(self._parser, '_comm_handler', None)
        self._system_handler = getattr(self._parser, '_system_handler', None)
        
        # 构建 running_map（用于线程推断）
        self._running_map = self._build_running_map()
    
    def _build_running_map(self) -> Dict[int, List[Tuple[int, int, int]]]:
        """
        构建 CPU -> [(timestamp, pid, tid), ...] 映射
        
        同时收集所有 CPU IDs 和线程 (pid, tid) 对，
        避免后续 _create_tracks 中的重复遍历。
        """
        running_map: Dict[int, List[Tuple[int, int, int]]] = defaultdict(list)
        self._all_cpu_ids: Set[int] = set()
        self._all_threads: Set[Tuple[int, int]] = set()
        
        for e in self._thread_events:
            self._all_cpu_ids.add(e.cpu_id)
            self._all_threads.add((e.pid, e.tid))
            if e.state == "RUNNING":
                running_map[e.cpu_id].append((e.timestamp_ns, e.pid, e.tid))
        
        for cpu_id in running_map:
            running_map[cpu_id].sort(key=lambda x: x[0])
        
        return running_map
    
    def export(self, output_path: str) -> Dict[str, Any]:
        """
        导出为 Perfetto protobuf 格式
        
        Args:
            output_path: 输出文件路径
            
        Returns:
            导出统计信息
        """
        # 1. 收集信息并创建 Tracks
        self._create_tracks()
        
        # 2. 写入事件
        self._write_events()
        
        # 3. 保存文件
        data = self._builder.serialize()
        with open(output_path, 'wb') as f:
            f.write(data)
        
        self._stats.file_size_kb = len(data) / 1024
        return self._stats.to_dict()
    
    def _create_tracks(self):
        """创建所有 Tracks（使用 _build_running_map 中预收集的数据）"""
        if not self._thread_events:
            return
        
        # 创建 CPU Tracks（使用预收集的 CPU IDs）
        self._track_manager.create_cpu_tracks(self._all_cpu_ids)
        self._stats.cpu_tracks = self._track_manager.cpu_track_count
        
        # 创建 Process/Thread Tracks（使用预收集的线程列表）
        self._track_manager.create_process_thread_tracks(list(self._all_threads))
        self._stats.process_tracks = self._track_manager.process_track_count
        self._stats.thread_tracks = self._track_manager.thread_track_count
    
    def _write_events(self):
        """写入所有事件"""
        # 1. 线程状态事件 (Slices)
        self._stats.thread_state_slices = self._thread_writer.write_thread_states(
            self._thread_events
        )
        
        # 2. CPU 运行线程事件 (Slices)
        self._stats.cpu_running_slices = self._cpu_writer.write_running_threads(
            self._thread_events
        )
        
        # 3. 中断事件 (Slices + Instants)
        self._cpu_writer.write_interrupts(self._int_enters, self._int_exits)
        self._stats.interrupt_slices = self._cpu_writer.interrupt_slice_count
        self._stats.interrupt_instants = self._cpu_writer.interrupt_instant_count
        
        # 4. 同步原语事件 (Instants, 仅 ENTER)
        if self._kercall_handler:
            sync_enter, sync_exit = self._sync_writer.write_sync_events(
                self._kercall_handler.mutex_events,
                self._kercall_handler.sem_events,
                self._kercall_handler.condvar_events,
                self._running_map
            )
            self._stats.sync_enter = sync_enter
            self._stats.sync_exit = sync_exit
        
        # 5. IPC 事件 (Slices + Instants)
        if self._kercall_handler:
            ipc_slices, ipc_instants = self._ipc_writer.write_ipc_events(
                self._kercall_handler.msg_events,
                self._running_map
            )
            self._stats.ipc_slices = ipc_slices
            self._stats.ipc_instants = ipc_instants
        
        # 6. 调度事件 (Instants)
        if self._kercall_handler:
            self._stats.sched_instants = self._write_sched_events()
        
        # 7. 通信事件（写入到 Thread Track）
        if self._comm_handler:
            comm_counts = self._comm_writer.write_comm_events(
                self._comm_handler, self._running_map
            )
            self._stats.comm_msg = comm_counts.get("msg", 0)
            self._stats.comm_reply = comm_counts.get("reply", 0)
            self._stats.comm_pulse = comm_counts.get("pulse", 0)
            self._stats.comm_signal = comm_counts.get("signal", 0)
        
        # 8. 系统事件
        if self._system_handler:
            sys_counts = self._system_writer.write_system_events(self._system_handler)
            self._stats.sys_ipi = sys_counts.get("ipi", 0)
            self._stats.sys_pagewait = sys_counts.get("pagewait", 0)
    
    def _write_sched_events(self) -> int:
        """写入调度事件 (SchedYield) - Instant Events"""
        from .utils import write_instant
        
        count = 0
        sched_events = getattr(self._kercall_handler, 'sched_events', [])
        
        for e in sched_events:
            etype = type(e).__name__
            
            # 找到执行该操作的线程
            pid, tid = self._find_thread(e.cpu_id, e.timestamp_ns)
            if pid == 0 and tid == 0:
                continue
            
            track_uuid = self._track_manager.get_thread_track(pid, tid)
            if not track_uuid:
                continue
            
            if etype == 'SchedYieldEnterEvent':
                args = {
                    "cpu": e.cpu_id,
                    "is_wide": getattr(e, 'is_wide', False),
                    "is_64bit": getattr(e, 'is_64bit', False),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "SchedYield", args)
                count += 1
                
            elif etype == 'SchedYieldExitEvent':
                args = {
                    "cpu": e.cpu_id,
                    "ret_val": getattr(e, 'ret_val', 0),
                    "errno_val": getattr(e, 'errno_val', 0),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "SchedYield_EXIT", args)
                count += 1
        
        return count
    
    def _find_thread(self, cpu_id: int, timestamp_ns: int) -> Tuple[int, int]:
        """
        根据 cpu_id 和时间戳找到当时正在运行的线程
        
        如果该时间点之前没有 RUNNING 事件（trace 开头），
        则回退到该 CPU 上最早的 RUNNING 事件。
        """
        running_list = self._running_map.get(cpu_id, [])
        if not running_list:
            return 0, 0
        
        left, right = 0, len(running_list) - 1
        result = None
        
        while left <= right:
            mid = (left + right) // 2
            if running_list[mid][0] <= timestamp_ns:
                result = running_list[mid]
                left = mid + 1
            else:
                right = mid - 1
        
        if result:
            return result[1], result[2]
        
        # 回退：使用该 CPU 上最早的 RUNNING 事件
        return running_list[0][1], running_list[0][2]


def export_to_perfetto(parser: "QstParser", output_path: str) -> Dict[str, Any]:
    """
    便捷导出函数
    
    Args:
        parser: QST 解析器实例
        output_path: 输出文件路径
        
    Returns:
        导出统计信息
    """
    exporter = PerfettoExporter(parser)
    return exporter.export(output_path)
