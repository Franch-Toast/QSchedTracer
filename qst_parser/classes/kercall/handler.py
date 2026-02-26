"""
QST Parser - KerCall 类事件处理器

处理 _NTO_TRACE_KERCALL* 类的所有内核调用事件。

根据 samples_parser.html，内核调用事件通过事件号范围区分:
- 0-127: KERCALLENTER (进入内核调用)
- 128-255: KERCALLEXIT (退出内核调用)
- 256-383: KERCALLINT (内核调用被中断)
"""

from typing import List, Optional, Any, Dict, Tuple
from ..base import BaseClassHandler
from ...models.base import RawEvent
from ...constants import ExternalClass
from .constants import (
    KernelCall, KERCALL_64, KERCALL_SIGNAL, KERCALL_MSG,
    KERCALL_SYNC, KERCALL_TIMER, KERCALL_CHANNEL, KERCALL_THREAD,
)
from .models import (
    KerCallEvent, KerCallBaseEvent,
    KerCallEnterEvent, KerCallExitEvent, KerCallIntEvent,
    SignalKillEvent,
    # 同步原语
    MutexLockEnterEvent, MutexLockExitEvent,
    MutexUnlockEnterEvent, MutexUnlockExitEvent,
    SemWaitEnterEvent, SemWaitExitEvent,
    SemPostEnterEvent, SemPostExitEvent,
    CondvarWaitEnterEvent, CondvarWaitExitEvent,
    CondvarSignalEnterEvent, CondvarSignalExitEvent,
    # 消息传递
    MsgSendvEnterEvent, MsgSendvExitEvent,
    MsgReceivevEnterEvent, MsgReceivevExitEvent,
    MsgReplyvEnterEvent, MsgReplyvExitEvent,
    # 调度
    SchedSetEnterEvent, SchedSetExitEvent,
    SchedYieldEnterEvent, SchedYieldExitEvent,
    # 配对后的事件
    PairedSyncEvent, PairedIpcEvent,
)
from .events import (
    SignalKillEventParser, GenericKerCallParser,
    MutexLockEventParser, MutexUnlockEventParser,
    SemWaitEventParser, SemPostEventParser,
    CondvarWaitEventParser, CondvarSignalEventParser,
    MsgEventParser, SchedEventParser,
)
from ..thread.models import ThreadEvent
from collections import defaultdict


class KerCallClassHandler(BaseClassHandler):
    """
    KerCall 类事件处理器
    
    处理所有内核调用事件，根据外部类区分:
    - KERCALLENTER: 进入内核调用，记录输入参数
    - KERCALLEXIT: 退出内核调用，记录返回值
    - KERCALLINT: 内核调用被中断
    
    内核调用类型包括:
        - 信号相关: SIGNAL_KILL, SIGNAL_KILL_SIGVAL, NET_SIGNAL_KILL, ...
        - 消息传递: MSG_SENDV, MSG_RECEIVEV, MSG_REPLYV, ...
        - 同步原语: MUTEX_LOCK, CONDVAR_WAIT, SEM_WAIT, ...
        - 定时器: TIMER_CREATE, TIMER_SETTIME, ...
        - 线程: THREAD_CREATE, THREAD_DESTROY, ...
        - 频道/连接: CHANNEL_CREATE, CONNECT_ATTACH, ...
        - 其他: CACHE_FLUSH, CLOCK_*, SCHED_*, ...
    """
    
    def __init__(self, verbose: bool = False):
        super().__init__(verbose)
        
        # 专用解析器 - 信号
        self._signal_kill_parser = SignalKillEventParser(verbose)
        
        # 专用解析器 - Mutex
        self._mutex_lock_parser = MutexLockEventParser(verbose)
        self._mutex_unlock_parser = MutexUnlockEventParser(verbose)
        
        # 专用解析器 - Semaphore
        self._sem_wait_parser = SemWaitEventParser(verbose)
        self._sem_post_parser = SemPostEventParser(verbose)
        
        # 专用解析器 - Condvar
        self._condvar_wait_parser = CondvarWaitEventParser(verbose)
        self._condvar_signal_parser = CondvarSignalEventParser(verbose)
        
        # 专用解析器 - 消息传递
        self._msg_parser = MsgEventParser(verbose)
        
        # 专用解析器 - 调度
        self._sched_parser = SchedEventParser(verbose)
        
        # 通用解析器（处理未实现专用解析器的内核调用）
        self._generic_parser = GenericKerCallParser(verbose)
        
        # 存储所有内核调用事件（按类型分类）
        self.enter_events: List[KerCallEnterEvent] = []
        self.exit_events: List[KerCallExitEvent] = []
        self.int_events: List[KerCallIntEvent] = []
        
        # 专用事件列表
        self.mutex_events: List[KerCallBaseEvent] = []
        self.sem_events: List[KerCallBaseEvent] = []
        self.condvar_events: List[KerCallBaseEvent] = []
        self.msg_events: List[KerCallBaseEvent] = []
        self.sched_events: List[KerCallBaseEvent] = []
        
        # 兼容旧代码
        self.kercall_events: List[KerCallEvent] = []
    
    @property
    def class_name(self) -> str:
        return "KERCALL"
    
    @property
    def signal_events(self) -> List[SignalKillEvent]:
        """获取信号事件列表"""
        return self._signal_kill_parser.signal_events
    
    def handle_event(
        self, 
        raw: RawEvent, 
        ext_event: int,
        timestamp_ns: int,
        ext_class: int = ExternalClass.KERCALLENTER,
        **kwargs
    ) -> Optional[KerCallBaseEvent]:
        """
        处理内核调用事件
        
        这是统一入口，根据 ext_class 分发到对应的处理方法。
        
        Args:
            raw: 原始事件数据
            ext_event: 外部事件号（内核调用编号，可能包含 64 位标志）
            timestamp_ns: 已计算的 UTC 时间戳
            ext_class: 外部事件类 (KERCALLENTER/KERCALLEXIT/KERCALLINT)
        
        Returns:
            解析后的事件对象
        """
        if ext_class == ExternalClass.KERCALLENTER:
            return self.handle_enter_event(raw, ext_event, timestamp_ns)
        elif ext_class == ExternalClass.KERCALLEXIT:
            return self.handle_exit_event(raw, ext_event, timestamp_ns)
        elif ext_class == ExternalClass.KERCALLINT:
            return self.handle_int_event(raw, ext_event, timestamp_ns)
        else:
            # 未知类型，使用通用处理
            return self._handle_generic(raw, ext_event, timestamp_ns)
    
    def handle_enter_event(
        self,
        raw: RawEvent,
        ext_event: int,
        timestamp_ns: int
    ) -> Optional[KerCallEnterEvent]:
        """
        处理 KERCALLENTER 事件（进入内核调用）
        
        Args:
            raw: 原始事件数据
            ext_event: 外部事件号（内核调用编号，可能包含 64 位标志）
            timestamp_ns: 已计算的 UTC 时间戳
        
        Returns:
            KerCallEnterEvent 或其子类
        """
        # 提取内核调用编号和 64 位标志
        is_64bit = (ext_event & KERCALL_64) != 0
        kercall_num = ext_event & 0x7F
        
        event = None
        
        # 信号相关使用专用解析器
        if kercall_num in (KernelCall.SIGNAL_KILL, 
                          KernelCall.SIGNAL_KILL_SIGVAL,
                          KernelCall.NET_SIGNAL_KILL):
            signal_event = self._signal_kill_parser.parse(raw, timestamp_ns)
            if signal_event:
                return signal_event
        
        # 同步原语 - Mutex
        elif kercall_num == KernelCall.SYNC_MUTEX_LOCK:
            event = self._mutex_lock_parser.parse_enter(raw, timestamp_ns, is_64bit)
            if event:
                self.mutex_events.append(event)
        
        elif kercall_num == KernelCall.SYNC_MUTEX_UNLOCK:
            event = self._mutex_unlock_parser.parse_enter(raw, timestamp_ns, is_64bit)
            if event:
                self.mutex_events.append(event)
        
        # 同步原语 - Semaphore
        elif kercall_num == KernelCall.SYNC_SEM_WAIT:
            event = self._sem_wait_parser.parse_enter(raw, timestamp_ns, is_64bit)
            if event:
                self.sem_events.append(event)
        
        elif kercall_num == KernelCall.SYNC_SEM_POST:
            event = self._sem_post_parser.parse_enter(raw, timestamp_ns, is_64bit)
            if event:
                self.sem_events.append(event)
        
        # 同步原语 - Condvar
        elif kercall_num == KernelCall.SYNC_CONDVAR_WAIT:
            event = self._condvar_wait_parser.parse_enter(raw, timestamp_ns, is_64bit)
            if event:
                self.condvar_events.append(event)
        
        elif kercall_num == KernelCall.SYNC_CONDVAR_SIGNAL:
            event = self._condvar_signal_parser.parse_enter(raw, timestamp_ns, is_64bit)
            if event:
                self.condvar_events.append(event)
        
        # 消息传递
        elif kercall_num == KernelCall.MSG_SENDV:
            event = self._msg_parser.parse_msg_sendv_enter(raw, timestamp_ns, is_64bit)
            if event:
                self.msg_events.append(event)
        
        elif kercall_num == KernelCall.MSG_RECEIVEV:
            event = self._msg_parser.parse_msg_receivev_enter(raw, timestamp_ns, is_64bit)
            if event:
                self.msg_events.append(event)
        
        elif kercall_num == KernelCall.MSG_REPLYV:
            event = self._msg_parser.parse_msg_replyv_enter(raw, timestamp_ns, is_64bit)
            if event:
                self.msg_events.append(event)
        
        # 调度
        elif kercall_num == KernelCall.SCHED_SET:
            event = self._sched_parser.parse_sched_set_enter(raw, timestamp_ns, is_64bit)
            if event:
                self.sched_events.append(event)
        
        elif kercall_num == KernelCall.SCHED_YIELD:
            event = self._sched_parser.parse_sched_yield_enter(raw, timestamp_ns, is_64bit)
            if event:
                self.sched_events.append(event)
        
        # 其他内核调用使用通用解析器
        else:
            event = self._generic_parser.parse_enter(raw, kercall_num, timestamp_ns, is_64bit)
        
        if event:
            self.enter_events.append(event)
        return event
    
    def handle_exit_event(
        self,
        raw: RawEvent,
        ext_event: int,
        timestamp_ns: int
    ) -> Optional[KerCallExitEvent]:
        """
        处理 KERCALLEXIT 事件（退出内核调用）
        
        Args:
            raw: 原始事件数据
            ext_event: 外部事件号（内核调用编号，可能包含 64 位标志）
            timestamp_ns: 已计算的 UTC 时间戳
        
        Returns:
            KerCallExitEvent 或其子类
        """
        is_64bit = (ext_event & KERCALL_64) != 0
        kercall_num = ext_event & 0x7F
        
        event = None
        
        # 同步原语 - Mutex
        if kercall_num == KernelCall.SYNC_MUTEX_LOCK:
            event = self._mutex_lock_parser.parse_exit(raw, timestamp_ns, is_64bit)
            if event:
                self.mutex_events.append(event)
        
        elif kercall_num == KernelCall.SYNC_MUTEX_UNLOCK:
            event = self._mutex_unlock_parser.parse_exit(raw, timestamp_ns, is_64bit)
            if event:
                self.mutex_events.append(event)
        
        # 同步原语 - Semaphore
        elif kercall_num == KernelCall.SYNC_SEM_WAIT:
            event = self._sem_wait_parser.parse_exit(raw, timestamp_ns, is_64bit)
            if event:
                self.sem_events.append(event)
        
        elif kercall_num == KernelCall.SYNC_SEM_POST:
            event = self._sem_post_parser.parse_exit(raw, timestamp_ns, is_64bit)
            if event:
                self.sem_events.append(event)
        
        # 同步原语 - Condvar
        elif kercall_num == KernelCall.SYNC_CONDVAR_WAIT:
            event = self._condvar_wait_parser.parse_exit(raw, timestamp_ns, is_64bit)
            if event:
                self.condvar_events.append(event)
        
        elif kercall_num == KernelCall.SYNC_CONDVAR_SIGNAL:
            event = self._condvar_signal_parser.parse_exit(raw, timestamp_ns, is_64bit)
            if event:
                self.condvar_events.append(event)
        
        # 消息传递
        elif kercall_num == KernelCall.MSG_SENDV:
            event = self._msg_parser.parse_msg_sendv_exit(raw, timestamp_ns, is_64bit)
            if event:
                self.msg_events.append(event)
        
        elif kercall_num == KernelCall.MSG_RECEIVEV:
            event = self._msg_parser.parse_msg_receivev_exit(raw, timestamp_ns, is_64bit)
            if event:
                self.msg_events.append(event)
        
        elif kercall_num == KernelCall.MSG_REPLYV:
            event = self._msg_parser.parse_msg_replyv_exit(raw, timestamp_ns, is_64bit)
            if event:
                self.msg_events.append(event)
        
        # 调度
        elif kercall_num == KernelCall.SCHED_SET:
            event = self._sched_parser.parse_sched_set_exit(raw, timestamp_ns, is_64bit)
            if event:
                self.sched_events.append(event)
        
        elif kercall_num == KernelCall.SCHED_YIELD:
            event = self._sched_parser.parse_sched_yield_exit(raw, timestamp_ns, is_64bit)
            if event:
                self.sched_events.append(event)
        
        # 其他内核调用使用通用解析器
        else:
            event = self._generic_parser.parse_exit(raw, kercall_num, timestamp_ns, is_64bit)
        
        if event:
            self.exit_events.append(event)
        return event
    
    def handle_int_event(
        self,
        raw: RawEvent,
        ext_event: int,
        timestamp_ns: int
    ) -> Optional[KerCallIntEvent]:
        """
        处理 KERCALLINT 事件（内核调用被中断）
        
        Args:
            raw: 原始事件数据
            ext_event: 外部事件号（内核调用编号，可能包含 64 位标志）
            timestamp_ns: 已计算的 UTC 时间戳
        
        Returns:
            KerCallIntEvent
        """
        is_64bit = (ext_event & KERCALL_64) != 0
        kercall_num = ext_event & 0x7F
        
        event = self._generic_parser.parse_int(raw, kercall_num, timestamp_ns, is_64bit)
        if event:
            self.int_events.append(event)
        return event
    
    def _handle_generic(
        self,
        raw: RawEvent,
        ext_event: int,
        timestamp_ns: int
    ) -> Optional[KerCallEvent]:
        """通用处理（兼容旧代码）"""
        is_64bit = (ext_event & KERCALL_64) != 0
        kercall_num = ext_event & 0x7F
        
        event = self._generic_parser.parse(raw, kercall_num, timestamp_ns)
        if event:
            self.kercall_events.append(event)
        return event
    
    def infer_signal_senders(self, thread_events: List[ThreadEvent]) -> None:
        """
        从 RUNNING 事件推断信号发送者
        
        策略: 找到与 SignalKill 事件同一 CPU、时间最近且之前的 RUNNING 事件。
        使用 timestamp_ns（UNIX 纳秒时间戳）进行排序和比较，
        避免 32 位 cycles 回环导致的排序错误。
        
        Args:
            thread_events: 已解析的线程事件列表
        """
        signal_events = self.signal_events
        if not thread_events or not signal_events:
            return
        
        # 按 CPU 分组 RUNNING 事件
        running_by_cpu = defaultdict(list)
        for event in thread_events:
            if event.state == "RUNNING":
                running_by_cpu[event.cpu_id].append(event)
        
        # 按 UNIX 纳秒时间戳排序（无回环歧义）
        for cpu_events in running_by_cpu.values():
            cpu_events.sort(key=lambda e: e.timestamp_ns)
        
        # 为每个 SignalKill 事件推断发送者
        for sig_event in signal_events:
            cpu_id = sig_event.cpu_id
            sig_ts = sig_event.timestamp_ns
            
            if cpu_id not in running_by_cpu:
                continue
            
            # 二分查找时间戳 <= sig_ts 的最近 RUNNING 事件
            candidates = running_by_cpu[cpu_id]
            left, right = 0, len(candidates) - 1
            best_match = None
            
            while left <= right:
                mid = (left + right) // 2
                if candidates[mid].timestamp_ns <= sig_ts:
                    best_match = candidates[mid]
                    left = mid + 1
                else:
                    right = mid - 1
            
            if best_match:
                sig_event.sender_pid = best_match.pid
                sig_event.sender_tid = best_match.tid
                if self.verbose:
                    print(f"[SIGNAL_KILL] 推断发送者: PID={best_match.pid}, "
                          f"TID={best_match.tid}")
    
    # ========================================================================
    # 事件查询接口
    # ========================================================================
    
    def get_events_by_type(self, kercall_type: int) -> List[KerCallBaseEvent]:
        """获取指定类型的内核调用事件（所有 ENTER/EXIT/INT）"""
        result = []
        for e in self.enter_events:
            if e.kercall_num == kercall_type:
                result.append(e)
        for e in self.exit_events:
            if e.kercall_num == kercall_type:
                result.append(e)
        for e in self.int_events:
            if e.kercall_num == kercall_type:
                result.append(e)
        return result
    
    def get_message_events(self) -> List[KerCallBaseEvent]:
        """获取所有消息传递相关的内核调用事件"""
        return [e for e in self.enter_events + self.exit_events 
                if e.kercall_num in KERCALL_MSG]
    
    def get_sync_events(self) -> List[KerCallBaseEvent]:
        """获取所有同步原语相关的内核调用事件"""
        return [e for e in self.enter_events + self.exit_events 
                if e.kercall_num in KERCALL_SYNC]
    
    def get_timer_events(self) -> List[KerCallBaseEvent]:
        """获取所有定时器相关的内核调用事件"""
        return [e for e in self.enter_events + self.exit_events 
                if e.kercall_num in KERCALL_TIMER]
    
    # ========================================================================
    # 事件配对接口
    # ========================================================================
    
    def _build_running_map(self, thread_events: List[ThreadEvent]) -> Dict[int, List[Tuple[int, int, int]]]:
        """
        构建 CPU -> [(timestamp, pid, tid), ...] 的映射
        
        用于推断 KerCall 事件发生时在该 CPU 上运行的线程。
        
        Args:
            thread_events: 线程事件列表
        
        Returns:
            {cpu_id: [(timestamp_ns, pid, tid), ...], ...}，按时间戳排序
        """
        running_map: Dict[int, List[Tuple[int, int, int]]] = defaultdict(list)
        
        for e in thread_events:
            if e.state == "RUNNING":
                running_map[e.cpu_id].append((e.timestamp_ns, e.pid, e.tid))
        
        # 按时间戳排序
        for cpu_id in running_map:
            running_map[cpu_id].sort(key=lambda x: x[0])
        
        return running_map
    
    def _find_thread_for_event(
        self, 
        running_map: Dict[int, List[Tuple[int, int, int]]],
        cpu_id: int, 
        timestamp_ns: int
    ) -> Tuple[int, int]:
        """
        根据 cpu_id 和时间戳找到当时正在运行的线程
        
        使用二分查找找到时间戳 <= timestamp_ns 的最近 RUNNING 事件。
        
        Args:
            running_map: CPU -> RUNNING 事件映射
            cpu_id: CPU ID
            timestamp_ns: 时间戳
        
        Returns:
            (pid, tid) 或 (0, 0) 如果找不到
        """
        running_list = running_map.get(cpu_id, [])
        if not running_list:
            return 0, 0
        
        # 二分查找最近的 RUNNING 事件（时间戳 <= timestamp_ns）
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
            return result[1], result[2]  # pid, tid
        return 0, 0
    
    def pair_sync_events(
        self, 
        thread_events: List[ThreadEvent]
    ) -> Dict[str, List[PairedSyncEvent]]:
        """
        配对同步原语事件（只配对 Duration Slice 类型）
        
        按线程 (pid, tid) 分组配对 ENTER/EXIT 事件，而不是按 CPU。
        
        只配对需要 Duration Slice 的事件：
        - mutex_lock: 阻塞型操作，需要显示持续时间
        - sem_wait: 阻塞型操作
        - condvar_wait: 阻塞型操作
        
        不配对的事件（作为 Instant Event 处理）：
        - mutex_unlock: 快速释放操作
        - sem_post: 快速释放操作
        - condvar_signal: 快速通知操作
        
        Args:
            thread_events: 线程事件列表（用于推断 pid/tid）
        
        Returns:
            {
                'mutex_lock': [PairedSyncEvent, ...],
                'sem_wait': [...],
                'condvar_wait': [...],
            }
        """
        running_map = self._build_running_map(thread_events)
        
        result: Dict[str, List[PairedSyncEvent]] = {
            'mutex_lock': [],
            'sem_wait': [],
            'condvar_wait': [],
        }
        
        # 只配对 Duration Slice 类型的事件
        # mutex_unlock, sem_post, condvar_signal 作为 Instant Event 处理
        sync_type_map = {
            'MutexLockEnterEvent': ('mutex_lock', 'MutexLockExitEvent', self.mutex_events),
            'SemWaitEnterEvent': ('sem_wait', 'SemWaitExitEvent', self.sem_events),
            'CondvarWaitEnterEvent': ('condvar_wait', 'CondvarWaitExitEvent', self.condvar_events),
        }
        
        # 按类型和线程分组
        for enter_type, (result_key, exit_type, events) in sync_type_map.items():
            enters_by_thread: Dict[Tuple[int, int], List] = defaultdict(list)
            exits_by_thread: Dict[Tuple[int, int], List] = defaultdict(list)
            
            for e in events:
                etype = type(e).__name__
                pid, tid = self._find_thread_for_event(running_map, e.cpu_id, e.timestamp_ns)
                
                if pid == 0 and tid == 0:
                    continue  # 无法确定线程，跳过
                
                if etype == enter_type:
                    enters_by_thread[(pid, tid)].append(e)
                elif etype == exit_type:
                    exits_by_thread[(pid, tid)].append(e)
            
            # 在每个线程组内配对
            for (pid, tid), enters in enters_by_thread.items():
                exits = exits_by_thread.get((pid, tid), [])
                
                # 按时间戳排序
                enters.sort(key=lambda e: e.timestamp_ns)
                exits.sort(key=lambda e: e.timestamp_ns)
                
                # 顺序配对
                for enter, exit in zip(enters, exits):
                    if exit.timestamp_ns <= enter.timestamp_ns:
                        continue  # 异常情况，跳过
                    
                    paired = PairedSyncEvent(
                        pid=pid,
                        tid=tid,
                        enter_ts=enter.timestamp_ns,
                        exit_ts=exit.timestamp_ns,
                        sync_type=result_key,
                        sync_ptr=getattr(enter, 'sync_ptr', 0),
                        enter_cpu=enter.cpu_id,
                        exit_cpu=exit.cpu_id,
                        ret_val=getattr(exit, 'ret_val', 0),
                        errno_val=getattr(exit, 'errno_val', 0),
                    )
                    result[result_key].append(paired)
        
        return result
    
    def pair_ipc_events(
        self, 
        thread_events: List[ThreadEvent]
    ) -> Dict[str, List[PairedIpcEvent]]:
        """
        配对 IPC 事件
        
        按线程 (pid, tid) 分组配对 ENTER/EXIT 事件。
        
        Args:
            thread_events: 线程事件列表（用于推断 pid/tid）
        
        Returns:
            {
                'msg_send': [PairedIpcEvent, ...],
                'msg_reply': [...],
            }
        """
        running_map = self._build_running_map(thread_events)
        
        result: Dict[str, List[PairedIpcEvent]] = {
            'msg_send': [],
            'msg_reply': [],
        }
        
        # 定义事件类型映射
        ipc_type_map = {
            'MsgSendvEnterEvent': ('msg_send', 'MsgSendvExitEvent'),
            'MsgReplyvEnterEvent': ('msg_reply', 'MsgReplyvExitEvent'),
        }
        
        for ipc_type, (result_key, exit_type) in ipc_type_map.items():
            enters_by_thread: Dict[Tuple[int, int], List] = defaultdict(list)
            exits_by_thread: Dict[Tuple[int, int], List] = defaultdict(list)
            
            for e in self.msg_events:
                etype = type(e).__name__
                pid, tid = self._find_thread_for_event(running_map, e.cpu_id, e.timestamp_ns)
                
                if pid == 0 and tid == 0:
                    continue
                
                if etype == ipc_type:
                    enters_by_thread[(pid, tid)].append(e)
                elif etype == exit_type:
                    exits_by_thread[(pid, tid)].append(e)
            
            # 在每个线程组内配对
            for (pid, tid), enters in enters_by_thread.items():
                exits = exits_by_thread.get((pid, tid), [])
                
                enters.sort(key=lambda e: e.timestamp_ns)
                exits.sort(key=lambda e: e.timestamp_ns)
                
                for enter, exit in zip(enters, exits):
                    if exit.timestamp_ns <= enter.timestamp_ns:
                        continue
                    
                    paired = PairedIpcEvent(
                        pid=pid,
                        tid=tid,
                        enter_ts=enter.timestamp_ns,
                        exit_ts=exit.timestamp_ns,
                        ipc_type=result_key,
                        coid=getattr(enter, 'coid', 0),
                        rcvid=getattr(enter, 'rcvid', 0),
                        chid=getattr(enter, 'chid', 0),
                        enter_cpu=enter.cpu_id,
                        exit_cpu=exit.cpu_id,
                        ret_val=getattr(exit, 'ret_val', 0),
                        errno_val=getattr(exit, 'errno_val', 0),
                    )
                    result[result_key].append(paired)
        
        return result
    
    def get_instant_sync_events(
        self, 
        thread_events: List[ThreadEvent]
    ) -> Dict[str, List[Tuple[int, int, int, int]]]:
        """
        获取即时同步事件（只有 EXIT，没有配对的 ENTER）
        
        如 mutex_unlock, sem_post, condvar_signal 的 EXIT 事件。
        
        Args:
            thread_events: 线程事件列表
        
        Returns:
            {
                'mutex_unlock': [(pid, tid, timestamp_ns, cpu_id), ...],
                'sem_post': [...],
                'condvar_signal': [...],
            }
        """
        running_map = self._build_running_map(thread_events)
        
        result: Dict[str, List[Tuple[int, int, int, int]]] = {
            'mutex_unlock': [],
            'sem_post': [],
            'condvar_signal': [],
        }
        
        # Mutex Unlock EXIT
        for e in self.mutex_events:
            if type(e).__name__ == 'MutexUnlockExitEvent':
                pid, tid = self._find_thread_for_event(running_map, e.cpu_id, e.timestamp_ns)
                if pid != 0 or tid != 0:
                    result['mutex_unlock'].append((pid, tid, e.timestamp_ns, e.cpu_id))
        
        # Sem Post EXIT
        for e in self.sem_events:
            if type(e).__name__ == 'SemPostExitEvent':
                pid, tid = self._find_thread_for_event(running_map, e.cpu_id, e.timestamp_ns)
                if pid != 0 or tid != 0:
                    result['sem_post'].append((pid, tid, e.timestamp_ns, e.cpu_id))
        
        # Condvar Signal EXIT
        for e in self.condvar_events:
            if type(e).__name__ == 'CondvarSignalExitEvent':
                pid, tid = self._find_thread_for_event(running_map, e.cpu_id, e.timestamp_ns)
                if pid != 0 or tid != 0:
                    result['condvar_signal'].append((pid, tid, e.timestamp_ns, e.cpu_id))
        
        return result
    
    def get_instant_ipc_events(
        self, 
        thread_events: List[ThreadEvent]
    ) -> List[Tuple[int, int, int, int, int]]:
        """
        获取即时 IPC 事件（MsgReceive ENTER）
        
        Args:
            thread_events: 线程事件列表
        
        Returns:
            [(pid, tid, timestamp_ns, cpu_id, chid), ...]
        """
        running_map = self._build_running_map(thread_events)
        result = []
        
        for e in self.msg_events:
            if type(e).__name__ == 'MsgReceivevEnterEvent':
                pid, tid = self._find_thread_for_event(running_map, e.cpu_id, e.timestamp_ns)
                if pid != 0 or tid != 0:
                    result.append((pid, tid, e.timestamp_ns, e.cpu_id, getattr(e, 'chid', 0)))
        
        return result
