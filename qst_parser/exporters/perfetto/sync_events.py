"""
同步原语事件写入器

根据 TRACE_EVENT_SELECTION_GUIDE.md:
- 同步原语仅采集 ENTER 事件（不采集 EXIT）
- 因此全部作为 Instant Events 写入

事件类型：
- MutexLock ENTER
- MutexUnlock ENTER
- SemWait ENTER
- SemPost ENTER
- CondvarWait ENTER
- CondvarSignal ENTER
"""

from typing import List, Dict, Tuple, TYPE_CHECKING

from .tracks import TrackManager
from .utils import write_instant

if TYPE_CHECKING:
    from perfetto.trace_builder.proto_builder import TraceProtoBuilder


class SyncEventWriter:
    """同步原语事件写入器"""
    
    def __init__(self, builder: "TraceProtoBuilder", track_manager: TrackManager):
        self._builder = builder
        self._tracks = track_manager
        self._enter_count = 0
        self._exit_count = 0
        # 丢弃统计
        self._dropped_no_thread = 0  # 找不到对应线程
        self._dropped_no_track = 0   # 找不到对应 Track
        self._total_input = 0        # 输入事件总数
    
    def write_sync_events(
        self,
        mutex_events: List,
        sem_events: List,
        condvar_events: List,
        running_map: Dict[int, List[Tuple[int, int, int]]]
    ) -> Tuple[int, int]:
        """
        写入所有同步原语事件
        
        根据配置，仅采集 ENTER 事件，作为 Instant 写入
        EXIT 事件（如果有）也作为 Instant 写入
        
        Args:
            mutex_events: 所有 Mutex 事件
            sem_events: 所有 Semaphore 事件
            condvar_events: 所有 Condvar 事件
            running_map: CPU -> [(timestamp, pid, tid), ...] 映射
        
        Returns:
            (enter_count, exit_count)
        """
        self._enter_count = 0
        self._exit_count = 0
        self._dropped_no_thread = 0
        self._dropped_no_track = 0
        self._total_input = len(mutex_events) + len(sem_events) + len(condvar_events)
        
        # Mutex 事件
        self._write_mutex_events(mutex_events, running_map)
        
        # Semaphore 事件
        self._write_sem_events(sem_events, running_map)
        
        # Condvar 事件
        self._write_condvar_events(condvar_events, running_map)
        
        # 打印丢弃统计
        total_written = self._enter_count + self._exit_count
        if self._dropped_no_thread > 0 or self._dropped_no_track > 0:
            print(f"[WARN] Sync事件丢弃统计: "
                  f"输入={self._total_input}, 写入={total_written}, "
                  f"找不到线程={self._dropped_no_thread}, 找不到Track={self._dropped_no_track}")
        
        return self._enter_count, self._exit_count
    
    def _find_thread(
        self, 
        running_map: Dict[int, List[Tuple[int, int, int]]],
        cpu_id: int, 
        timestamp_ns: int
    ) -> Tuple[int, int]:
        """
        根据 cpu_id 和时间戳找到当时正在运行的线程
        
        如果该时间点之前没有 RUNNING 事件（trace 开头），
        则回退到该 CPU 上最早的 RUNNING 事件。
        """
        running_list = running_map.get(cpu_id, [])
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
    
    def _write_mutex_events(self, events: List, running_map: Dict):
        """写入 Mutex 事件 (全部为 Instant)"""
        for e in events:
            etype = type(e).__name__
            pid, tid = self._find_thread(running_map, e.cpu_id, e.timestamp_ns)
            if pid == 0 and tid == 0:
                # 仍然无法推断线程，使用 CPU 级别的 fallback
                pid, tid = self._get_fallback_thread(e.cpu_id)
            
            track_uuid = self._tracks.get_thread_sync_track(pid, tid)
            if not track_uuid:
                self._dropped_no_track += 1
                continue
            
            # 使用格式化的名称
            thread_name = self._tracks.format_thread_name(pid, tid)
            
            if etype == 'MutexLockEnterEvent':
                args = {
                    "type": "ENTER",
                    "thread": thread_name,
                    "cpu": e.cpu_id,
                    "sync_ptr": hex(getattr(e, 'sync_ptr', 0)),
                    "count": getattr(e, 'count', 0),
                    "owner": getattr(e, 'owner', 0),
                    "is_wide": getattr(e, 'is_wide', False),
                    "is_64bit": getattr(e, 'is_64bit', False),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "MutexLock", args)
                self._enter_count += 1
                
            elif etype == 'MutexLockExitEvent':
                args = {
                    "type": "EXIT",
                    "thread": thread_name,
                    "cpu": e.cpu_id,
                    "ret_val": getattr(e, 'ret_val', 0),
                    "errno_val": getattr(e, 'errno_val', 0),
                    "is_wide": getattr(e, 'is_wide', False),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "MutexLock_EXIT", args)
                self._exit_count += 1
                
            elif etype == 'MutexUnlockEnterEvent':
                args = {
                    "type": "ENTER",
                    "thread": thread_name,
                    "cpu": e.cpu_id,
                    "sync_ptr": hex(getattr(e, 'sync_ptr', 0)),
                    "count": getattr(e, 'count', 0),
                    "owner": getattr(e, 'owner', 0),
                    "is_wide": getattr(e, 'is_wide', False),
                    "is_64bit": getattr(e, 'is_64bit', False),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "MutexUnlock", args)
                self._enter_count += 1
                
            elif etype == 'MutexUnlockExitEvent':
                args = {
                    "type": "EXIT",
                    "thread": thread_name,
                    "cpu": e.cpu_id,
                    "ret_val": getattr(e, 'ret_val', 0),
                    "errno_val": getattr(e, 'errno_val', 0),
                    "is_wide": getattr(e, 'is_wide', False),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "MutexUnlock_EXIT", args)
                self._exit_count += 1
    
    def _write_sem_events(self, events: List, running_map: Dict):
        """写入 Semaphore 事件 (全部为 Instant)"""
        for e in events:
            etype = type(e).__name__
            pid, tid = self._find_thread(running_map, e.cpu_id, e.timestamp_ns)
            if pid == 0 and tid == 0:
                pid, tid = self._get_fallback_thread(e.cpu_id)
            
            track_uuid = self._tracks.get_thread_sync_track(pid, tid)
            if not track_uuid:
                self._dropped_no_track += 1
                continue
            
            # 使用格式化的名称
            thread_name = self._tracks.format_thread_name(pid, tid)
            
            if etype == 'SemWaitEnterEvent':
                args = {
                    "type": "ENTER",
                    "thread": thread_name,
                    "cpu": e.cpu_id,
                    "sync_ptr": hex(getattr(e, 'sync_ptr', 0)),
                    "try_flag": getattr(e, 'try_flag', 0),
                    "count": getattr(e, 'count', 0),
                    "owner": getattr(e, 'owner', 0),
                    "is_wide": getattr(e, 'is_wide', False),
                    "is_64bit": getattr(e, 'is_64bit', False),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "SemWait", args)
                self._enter_count += 1
                
            elif etype == 'SemWaitExitEvent':
                args = {
                    "type": "EXIT",
                    "thread": thread_name,
                    "cpu": e.cpu_id,
                    "ret_val": getattr(e, 'ret_val', 0),
                    "errno_val": getattr(e, 'errno_val', 0),
                    "is_wide": getattr(e, 'is_wide', False),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "SemWait_EXIT", args)
                self._exit_count += 1
                
            elif etype == 'SemPostEnterEvent':
                args = {
                    "type": "ENTER",
                    "thread": thread_name,
                    "cpu": e.cpu_id,
                    "sync_ptr": hex(getattr(e, 'sync_ptr', 0)),
                    "count": getattr(e, 'count', 0),
                    "owner": getattr(e, 'owner', 0),
                    "is_wide": getattr(e, 'is_wide', False),
                    "is_64bit": getattr(e, 'is_64bit', False),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "SemPost", args)
                self._enter_count += 1
                
            elif etype == 'SemPostExitEvent':
                args = {
                    "type": "EXIT",
                    "thread": thread_name,
                    "cpu": e.cpu_id,
                    "ret_val": getattr(e, 'ret_val', 0),
                    "errno_val": getattr(e, 'errno_val', 0),
                    "is_wide": getattr(e, 'is_wide', False),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "SemPost_EXIT", args)
                self._exit_count += 1
    
    def _write_condvar_events(self, events: List, running_map: Dict):
        """写入 Condvar 事件 (全部为 Instant)"""
        for e in events:
            etype = type(e).__name__
            pid, tid = self._find_thread(running_map, e.cpu_id, e.timestamp_ns)
            if pid == 0 and tid == 0:
                pid, tid = self._get_fallback_thread(e.cpu_id)
            
            track_uuid = self._tracks.get_thread_sync_track(pid, tid)
            if not track_uuid:
                self._dropped_no_track += 1
                continue
            
            # 使用格式化的名称
            thread_name = self._tracks.format_thread_name(pid, tid)
            
            if etype == 'CondvarWaitEnterEvent':
                args = {
                    "type": "ENTER",
                    "thread": thread_name,
                    "cpu": e.cpu_id,
                    "sync_ptr": hex(getattr(e, 'sync_ptr', 0)),
                    "mutex_ptr": hex(getattr(e, 'mutex_ptr', 0)),
                    "sync_count": getattr(e, 'sync_count', 0),
                    "sync_owner": getattr(e, 'sync_owner', 0),
                    "mutex_count": getattr(e, 'mutex_count', 0),
                    "mutex_owner": getattr(e, 'mutex_owner', 0),
                    "is_wide": getattr(e, 'is_wide', False),
                    "is_64bit": getattr(e, 'is_64bit', False),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "CondvarWait", args)
                self._enter_count += 1
                
            elif etype == 'CondvarWaitExitEvent':
                args = {
                    "type": "EXIT",
                    "thread": thread_name,
                    "cpu": e.cpu_id,
                    "ret_val": getattr(e, 'ret_val', 0),
                    "errno_val": getattr(e, 'errno_val', 0),
                    "is_wide": getattr(e, 'is_wide', False),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "CondvarWait_EXIT", args)
                self._exit_count += 1
                
            elif etype == 'CondvarSignalEnterEvent':
                args = {
                    "type": "ENTER",
                    "thread": thread_name,
                    "cpu": e.cpu_id,
                    "sync_ptr": hex(getattr(e, 'sync_ptr', 0)),
                    "signal_all": getattr(e, 'signal_all', 0),
                    "sync_count": getattr(e, 'sync_count', 0),
                    "sync_owner": getattr(e, 'sync_owner', 0),
                    "is_wide": getattr(e, 'is_wide', False),
                    "is_64bit": getattr(e, 'is_64bit', False),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "CondvarSignal", args)
                self._enter_count += 1
                
            elif etype == 'CondvarSignalExitEvent':
                args = {
                    "type": "EXIT",
                    "thread": thread_name,
                    "cpu": e.cpu_id,
                    "ret_val": getattr(e, 'ret_val', 0),
                    "errno_val": getattr(e, 'errno_val', 0),
                    "is_wide": getattr(e, 'is_wide', False),
                }
                write_instant(self._builder, e.timestamp_ns, track_uuid, "CondvarSignal_EXIT", args)
                self._exit_count += 1
    
    def _get_fallback_thread(self, cpu_id: int) -> Tuple[int, int]:
        """
        当无法推断线程时，返回一个 fallback 的 (pid, tid)
        
        使用一个虚拟进程 (pid=0xFFFF) 来收纳无法归属的事件，
        tid 使用 cpu_id+1 来区分不同 CPU 上的未知线程。
        """
        # 使用 0xFFFF 作为虚拟进程 PID，避免与真实进程冲突
        fallback_pid = 0xFFFF
        fallback_tid = cpu_id + 1
        return fallback_pid, fallback_tid
    
    # =========================================================================
    # 兼容旧接口
    # =========================================================================
    
    def write_all_sync_events_as_instant(
        self,
        mutex_events: List,
        sem_events: List,
        condvar_events: List,
        running_map: Dict[int, List[Tuple[int, int, int]]]
    ) -> Tuple[int, int]:
        """兼容旧接口"""
        return self.write_sync_events(mutex_events, sem_events, condvar_events, running_map)
    
    def write_paired_sync_events(
        self, 
        paired_events: Dict,
        instant_events: Dict
    ) -> Tuple[int, int]:
        """[已废弃]"""
        return 0, 0
    
    @property
    def enter_count(self) -> int:
        return self._enter_count
    
    @property
    def exit_count(self) -> int:
        return self._exit_count
