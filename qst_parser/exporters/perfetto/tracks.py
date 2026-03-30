"""
Perfetto Track Manager

Creates and manages all tracks (CPU, Process, Thread, sub-tracks).
Supports both in-memory and streaming protobuf builders.
"""

from typing import Dict, List, Set, Tuple

from perfetto.protos.perfetto.trace.perfetto_trace_pb2 import TrackDescriptor

from .utils import generate_uuid


class TrackManager:

    def __init__(self, builder):
        self._builder = builder
        
        # CPU Tracks: cpu_id -> uuid
        self._cpu_tracks: Dict[int, int] = {}
        self._cpu_group_uuid: int = 0
        
        # CPU IRQ 子 Tracks: cpu_id -> uuid
        # 中断事件与 Running Thread 重叠，需要单独的子 Track
        self._cpu_irq_tracks: Dict[int, int] = {}
        
        # Process Tracks: pid -> uuid
        self._process_tracks: Dict[int, int] = {}
        
        # Thread Tracks: (pid, tid) -> uuid
        self._thread_tracks: Dict[Tuple[int, int], int] = {}
        
        # 名称映射
        self._process_names: Dict[int, str] = {}
        self._thread_names: Dict[Tuple[int, int], str] = {}
    
    def set_names(self, process_names: Dict[int, str], 
                 thread_names: Dict[Tuple[int, int], str]):
        """
        设置进程和线程名称映射
        
        对所有名称进行清理，移除空字节和不可打印控制字符，
        防止在 Protobuf 序列化或 Perfetto UI 显示时被截断。
        """
        self._process_names = {k: self._sanitize_name(v) for k, v in process_names.items()}
        self._thread_names = {k: self._sanitize_name(v) for k, v in thread_names.items()}
    
    @staticmethod
    def _sanitize_name(name: str) -> str:
        """
        清理名称字符串
        
        移除空字节（\x00）和其他不可打印控制字符，
        避免 Protobuf 字符串被截断导致格式化后缀 (tid) 丢失。
        """
        # 先在空字节处截断（取第一段有效内容）
        name = name.split('\x00')[0]
        # 移除其他不可打印控制字符（保留空格和常规可打印字符）
        name = ''.join(c for c in name if c == ' ' or c.isprintable())
        # 移除首尾空白
        name = name.strip()
        return name if name else "unknown"
    
    def get_process_name(self, pid: int) -> str:
        """获取进程名"""
        return self._process_names.get(pid, f"Process {pid}")
    
    def get_thread_name(self, pid: int, tid: int) -> str:
        """
        获取线程名
        
        如果线程有特定名称，返回该名称；
        否则返回进程名（如果有）或默认的 "Thread" 前缀
        """
        if (pid, tid) in self._thread_names:
            return self._thread_names[(pid, tid)]
        # 如果没有线程名，使用进程名（如有）
        if pid in self._process_names:
            return f"{self._process_names[pid]}-unknown"
        return f"Thread-unknown"
    
    def format_process_name(self, pid: int) -> str:
        """格式化进程名称为 '名称 (pid)' 格式"""
        name = self.get_process_name(pid)
        return f"{name} ({pid})"
    
    def format_thread_name(self, pid: int, tid: int) -> str:
        """
        格式化线程名称为 '名称 (tid)' 格式
        
        如果有线程特定名称，显示：线程名 (tid)
        如果只有进程名，显示：进程名 (tid)
        如果都没有，显示：Thread (tid)
        """
        name = self.get_thread_name(pid, tid)
        return f"{name} ({tid})"
    
    # =========================================================================
    # CPU Tracks
    # =========================================================================
    
    def create_cpu_tracks(self, cpu_ids: Set[int]):
        """
        创建 CPU Tracks
        
        CPU Tracks 使用最低的排序索引，确保在 UI 中显示在最顶部。
        """
        if not cpu_ids:
            return
        
        # CPU 组 - 使用 sibling_order_rank 而不是 process.legacy_sort_index
        # 因为 process descriptor 需要 pid 字段
        self._cpu_group_uuid = generate_uuid()
        packet = self._builder.add_packet()
        packet.timestamp = 0
        desc = packet.track_descriptor
        desc.uuid = self._cpu_group_uuid
        desc.name = "CPU scheduling"
        desc.child_ordering = TrackDescriptor.EXPLICIT
        desc.sibling_order_rank = -1000000

        for cpu_id in sorted(cpu_ids):
            uuid = generate_uuid()
            self._cpu_tracks[cpu_id] = uuid

            packet = self._builder.add_packet()
            packet.timestamp = 0
            desc = packet.track_descriptor
            desc.uuid = uuid
            desc.name = f"CPU {cpu_id}"
            desc.parent_uuid = self._cpu_group_uuid
            desc.sibling_order_rank = cpu_id

    def get_cpu_track(self, cpu_id: int) -> int:
        """获取 CPU Track UUID"""
        return self._cpu_tracks.get(cpu_id, 0)
    
    def get_or_create_cpu_irq_track(self, cpu_id: int) -> int:
        """
        获取或创建 CPU 的 IRQ 子 Track
        
        中断事件发生在 Running Thread 期间，会导致时间重叠。
        将中断放在子 Track 上避免重叠错误。
        
        Args:
            cpu_id: CPU ID
        
        Returns:
            IRQ 子 Track 的 UUID
        """
        if cpu_id in self._cpu_irq_tracks:
            return self._cpu_irq_tracks[cpu_id]
        
        parent_uuid = self.get_cpu_track(cpu_id)
        if not parent_uuid:
            return 0
        
        uuid = generate_uuid()
        self._cpu_irq_tracks[cpu_id] = uuid
        
        packet = self._builder.add_packet()
        packet.timestamp = 0
        desc = packet.track_descriptor
        desc.uuid = uuid
        desc.name = "IRQ"
        desc.parent_uuid = parent_uuid

        return uuid

    # =========================================================================
    # Process/Thread Tracks
    # =========================================================================
    
    def create_process_thread_tracks(self, threads: List[Tuple[int, int]]):
        """
        创建进程和线程 Tracks
        
        层次结构：
        Process (pid)
        └── Thread [tid]        ← Thread State slices + instant events
        
        Args:
            threads: [(pid, tid), ...] 列表
        """
        # 收集所有进程并按 PID 排序
        pids = sorted(set(pid for pid, tid in threads))
        
        # 创建进程 Tracks（按 PID 排序）
        for sort_index, pid in enumerate(pids):
            uuid = generate_uuid()
            self._process_tracks[pid] = uuid
            
            name = self.get_process_name(pid)
            
            packet = self._builder.add_packet()
            packet.timestamp = 0
            desc = packet.track_descriptor
            desc.uuid = uuid
            desc.name = f"{name} ({pid})"
            desc.process.pid = pid
            desc.process.process_name = name
            desc.process.legacy_sort_index = sort_index
            desc.child_ordering = TrackDescriptor.CHRONOLOGICAL

        sorted_threads = sorted(threads, key=lambda x: (x[0], x[1]))

        for sort_index, (pid, tid) in enumerate(sorted_threads):
            if pid not in self._process_tracks:
                continue

            uuid = generate_uuid()
            self._thread_tracks[(pid, tid)] = uuid

            name = self.get_thread_name(pid, tid)
            parent_uuid = self._process_tracks[pid]

            packet = self._builder.add_packet()
            packet.timestamp = 0
            desc = packet.track_descriptor
            desc.uuid = uuid
            desc.parent_uuid = parent_uuid
            desc.name = f"{name} ({tid})"
            desc.sibling_order_rank = sort_index
            desc.child_ordering = TrackDescriptor.CHRONOLOGICAL
            desc.disallow_merging_with_system_tracks = True

    def get_thread_track(self, pid: int, tid: int) -> int:
        """获取线程 Track UUID"""
        return self._thread_tracks.get((pid, tid), 0)
    
    def has_thread_track(self, pid: int, tid: int) -> bool:
        """检查是否有该线程的 Track"""
        return (pid, tid) in self._thread_tracks
    
    def _ensure_thread_track(self, pid: int, tid: int) -> int:
        """
        确保线程 Track 存在，如不存在则创建
        
        用于处理 Sync/IPC 事件中出现的、但在 thread_events 中没有的线程
        """
        if (pid, tid) in self._thread_tracks:
            return self._thread_tracks[(pid, tid)]
        
        # 确保进程 Track 存在
        if pid not in self._process_tracks:
            # 创建进程 Track
            proc_uuid = generate_uuid()
            self._process_tracks[pid] = proc_uuid
            
            name = self.get_process_name(pid)
            packet = self._builder.add_packet()
            packet.timestamp = 0
            desc = packet.track_descriptor
            desc.uuid = proc_uuid
            desc.name = f"{name} ({pid})"
            desc.process.pid = pid
            desc.process.process_name = name
            desc.child_ordering = TrackDescriptor.CHRONOLOGICAL

        uuid = generate_uuid()
        self._thread_tracks[(pid, tid)] = uuid

        name = self.get_thread_name(pid, tid)
        parent_uuid = self._process_tracks[pid]

        packet = self._builder.add_packet()
        packet.timestamp = 0
        desc = packet.track_descriptor
        desc.uuid = uuid
        desc.parent_uuid = parent_uuid
        desc.name = f"{name} ({tid})"
        desc.child_ordering = TrackDescriptor.CHRONOLOGICAL
        desc.disallow_merging_with_system_tracks = True

        return uuid

    # =========================================================================
    # 统计
    # =========================================================================
    
    @property
    def cpu_track_count(self) -> int:
        return len(self._cpu_tracks)
    
    @property
    def process_track_count(self) -> int:
        return len(self._process_tracks)
    
    @property
    def thread_track_count(self) -> int:
        return len(self._thread_tracks)
