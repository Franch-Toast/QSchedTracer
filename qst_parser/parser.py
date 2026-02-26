"""
QST Parser - 主解析器类

QST 文件的核心解析逻辑。只负责高层调度，具体解析由各 class 处理器完成。
"""

from typing import Dict, List, Optional, Tuple
from collections import defaultdict
from datetime import datetime, timezone

from .constants import (
    QST_EVENT_SIZE,
    FILE_HEADER_SIZE,
    PROCINFO_HEADER_SIZE,
    MAINDATA_HEADER_SIZE,
    MAX_UINT32,
    InternalClass,
    ExternalClass,
)
from .models import FileHeader, ProcInfoHeader, MainDataHeader, RawEvent
from .utils import internal_to_external
from .classes import (
    ThreadClassHandler,
    VThreadClassHandler,
    ProcessClassHandler,
    KerCallClassHandler,
    ControlClassHandler,
    InterruptClassHandler,
    CommClassHandler,
    SystemClassHandler,
    QuipClassHandler,
    SecClassHandler,
    QvmClassHandler,
)
from .classes.thread import ThreadEvent
from .classes.kercall import SignalKillEvent


class QstParser:
    """
    QST 文件解析器 (v3)
    
    职责：
        1. 读取和解析文件头
        2. 反向推算事件时间戳（从最后一个事件向前计算，避免多次回环歧义）
        3. 分发事件到对应的类处理器
        4. 协调各处理器之间的数据交互
        5. 提供统一的对外接口
    
    具体的事件解析逻辑由各 class 处理器负责。
    """
    
    def __init__(self, filepath: str, verbose: bool = False):
        """
        初始化解析器
        
        Args:
            filepath: QST 文件路径
            verbose: 是否输出详细日志
        """
        self.filepath = filepath
        self.verbose = verbose
        
        # 文件头
        self.file_header: Optional[FileHeader] = None
        self.procinfo_header: Optional[ProcInfoHeader] = None
        self.maindata_header: Optional[MainDataHeader] = None
        
        # 时间戳统计（由反向时间戳计算填充）
        self._first_timestamp_ns: Optional[int] = None
        self._last_timestamp_ns: Optional[int] = None
        
        # 初始化所有类处理器
        self._thread_handler = ThreadClassHandler(verbose)
        self._vthread_handler = VThreadClassHandler(verbose)
        self._process_handler = ProcessClassHandler(verbose)
        self._kercall_handler = KerCallClassHandler(verbose)
        self._control_handler = ControlClassHandler(verbose)
        self._interrupt_handler = InterruptClassHandler(verbose)
        self._comm_handler = CommClassHandler(verbose)
        self._system_handler = SystemClassHandler(verbose)
        self._quip_handler = QuipClassHandler(verbose)
        self._sec_handler = SecClassHandler(verbose)
        self._qvm_handler = QvmClassHandler(verbose)
    
    # ========================================================================
    # 对外接口 - 数据访问
    # ========================================================================
    
    @property
    def thread_events(self) -> List[ThreadEvent]:
        """获取所有线程事件（包括 thread 和 vthread）"""
        events = []
        events.extend(self._thread_handler.thread_events)
        events.extend(self._vthread_handler.thread_events)
        return events
    
    @property
    def signal_events(self) -> List[SignalKillEvent]:
        """获取所有信号事件"""
        return self._kercall_handler.signal_events
    
    @property
    def process_info(self) -> Dict[int, str]:
        """获取进程信息字典 (pid -> name)"""
        return self._process_handler.process_info
    
    @property
    def thread_info(self) -> Dict[Tuple[int, int], str]:
        """获取线程信息字典 ((pid, tid) -> name)"""
        return self._process_handler.thread_info
    
    def get_thread_name(self, pid: int, tid: int) -> str:
        """获取线程名称"""
        return self._process_handler.get_thread_name(pid, tid)
    
    def get_process_name(self, pid: int) -> str:
        """获取进程名称"""
        return self._process_handler.get_process_name(pid)
    
    # ========================================================================
    # 解析流程
    # ========================================================================
    
    def parse(self) -> None:
        """解析 QST 文件"""
        with open(self.filepath, 'rb') as f:
            # 1. 读取文件头
            self._read_file_header(f)
            
            # 2. 读取并解析 ProcInfo 数据（仅提取进程/线程名称）
            self._read_and_parse_procinfo(f)
            
            # 3. 读取并解析主调度数据（使用反向时间戳计算）
            self._read_and_parse_main_data(f)
        
        # 4. 后处理（信号发送者推断等）
        self._post_process()
        
        if self.verbose:
            self._print_parse_summary()
    
    def _read_file_header(self, f) -> None:
        """读取文件头"""
        file_header_data = f.read(FILE_HEADER_SIZE)
        self.file_header = FileHeader.from_bytes(file_header_data)
        
        if self.verbose:
            print(f"[解析] 文件版本: v{self.file_header.version}")
            print(f"[解析] 时钟频率: {self.file_header.clock_freq:,} Hz")
            print(f"[解析] Wallclock: {self.file_header.get_wallclock_datetime().isoformat()}")
            print(f"[解析] Sync Cycles: {self.file_header.sync_cycles}")
    
    def _read_and_parse_procinfo(self, f) -> None:
        """读取并解析 ProcInfo 数据（仅提取进程/线程名称，不需要精确时间戳）"""
        procinfo_header_data = f.read(PROCINFO_HEADER_SIZE)
        self.procinfo_header = ProcInfoHeader.from_bytes(procinfo_header_data)
        
        procinfo_size = self.procinfo_header.event_count * QST_EVENT_SIZE
        procinfo_data = f.read(procinfo_size)
        
        if self.verbose:
            print(f"[解析] ProcInfo 事件数: {self.procinfo_header.event_count}")
        
        # ProcInfo 仅用于提取进程/线程名称，时间戳设为 0
        event_count = len(procinfo_data) // QST_EVENT_SIZE
        for i in range(event_count):
            offset = i * QST_EVENT_SIZE
            event_data = procinfo_data[offset:offset + QST_EVENT_SIZE]
            if len(event_data) != QST_EVENT_SIZE:
                continue
            raw = RawEvent.from_bytes(event_data)
            self._dispatch_event(raw, is_procinfo=True, timestamp_ns=0)
    
    def _read_and_parse_main_data(self, f) -> None:
        """读取并解析主调度数据（使用反向时间戳计算）"""
        maindata_header_data = f.read(MAINDATA_HEADER_SIZE)
        self.maindata_header = MainDataHeader.from_bytes(maindata_header_data)
        
        main_data_size = self.maindata_header.event_count * QST_EVENT_SIZE
        main_data = f.read(main_data_size)
        
        if self.verbose:
            print(f"[解析] 主调度事件数: {self.maindata_header.event_count}")
        
        self._parse_main_data_backward(main_data)
    
    def _parse_main_data_backward(self, data: bytes) -> None:
        """
        使用反向时间戳计算解析主调度数据
        
        算法:
            1. 预扫描所有事件，提取 cycles，检测时间跳跃确定有效范围
            2. 从最后一个事件（anchor: sync_cycles -> wallclock_ns）向前推算时间戳
               利用相邻事件的 cycles 差值计算，避免多次 32 位回环的歧义
            3. 按原始顺序分发事件到各处理器
        
        优势:
            相邻事件的 cycles 差值很小（通常微秒级），
            即使 32 位 cycles 在整个采集期间发生多次回环，
            相邻差值也不会跨越回环边界，因此计算是无歧义的。
        """
        event_count = len(data) // QST_EVENT_SIZE
        if event_count == 0:
            return
        
        # ====== Phase 1: 预扫描，提取原始事件和 cycles ======
        raw_events = []
        cycles_list = []
        
        # 时间跳跃检测阈值（10ms 的 cycles 数）
        jump_threshold_cycles = None
        if self.file_header and self.file_header.clock_freq > 0:
            jump_threshold_cycles = int(self.file_header.clock_freq * 0.01)
        
        prev_cycles = None
        
        for i in range(event_count):
            offset = i * QST_EVENT_SIZE
            event_data = data[offset:offset + QST_EVENT_SIZE]
            
            if len(event_data) != QST_EVENT_SIZE:
                break
            
            raw = RawEvent.from_bytes(event_data)
            curr_cycles = raw.data[0]
            
            # 时间跳跃检测（使用无符号 32 位差值）
            if jump_threshold_cycles and prev_cycles is not None:
                delta = (curr_cycles - prev_cycles) & MAX_UINT32
                if delta > jump_threshold_cycles:
                    if self.verbose and self.file_header:
                        jump_ms = delta / self.file_header.clock_freq * 1000
                        print(f"[警告] 在事件 {i} 处检测到 {jump_ms:.2f}ms 的时间跳跃")
                        print(f"[警告] 过滤掉 {event_count - i} 个可能污染的事件")
                    break
            
            prev_cycles = curr_cycles
            raw_events.append(raw)
            cycles_list.append(curr_cycles)
        
        n = len(raw_events)
        if n == 0:
            return
        
        # ====== Phase 2: 从最后一个事件向前计算时间戳 ======
        timestamps = [0] * n
        
        wallclock_ns = self.file_header.get_wallclock_ns() if self.file_header else 0
        clock_freq = self.file_header.clock_freq if self.file_header else 0
        
        # 最后一个事件的时间戳 = wallclock_ns（anchor point）
        timestamps[n - 1] = wallclock_ns
        
        if clock_freq > 0:
            for i in range(n - 2, -1, -1):
                curr_c = cycles_list[i]
                next_c = cycles_list[i + 1]
                
                # 计算相邻事件的 forward delta（无符号 32 位差值）
                # 正常情况下 next_c >= curr_c，delta 是小正数
                # 发生回环时 next_c < curr_c，& MAX_UINT32 得到正确的 forward delta
                delta_cycles = (next_c - curr_c) & MAX_UINT32
                
                # 如果 delta 超过半个周期（约 2^31），说明是小幅度反向乱序
                # （wide 事件的多个 event 可能时间戳略有乱序）
                if delta_cycles > MAX_UINT32 // 2:
                    # 转换为有符号负值：事件 i 实际上稍晚于事件 i+1
                    delta_cycles = delta_cycles - (MAX_UINT32 + 1)
                
                # 计算时间差（整数运算避免浮点误差）
                delta_ns = (delta_cycles * 1_000_000_000) // clock_freq
                
                # 向前推算：当前事件时间 = 下一个事件时间 - delta
                timestamps[i] = timestamps[i + 1] - delta_ns
        else:
            # 时钟频率无效，所有事件使用 wallclock
            for i in range(n):
                timestamps[i] = wallclock_ns
        
        # 更新时间戳统计
        self._first_timestamp_ns = timestamps[0]
        self._last_timestamp_ns = timestamps[n - 1]
        
        if self.verbose:
            first_dt = datetime.fromtimestamp(timestamps[0] / 1e9, tz=timezone.utc)
            last_dt = datetime.fromtimestamp(timestamps[n - 1] / 1e9, tz=timezone.utc)
            duration = (timestamps[n - 1] - timestamps[0]) / 1e9
            print(f"[解析] 时间戳计算完成（反向推算）:")
            print(f"  第一个事件: {first_dt.isoformat()}")
            print(f"  最后一个事件: {last_dt.isoformat()}")
            print(f"  持续时间: {duration:.3f} 秒")
        
        # ====== Phase 3: 按原始顺序分发事件 ======
        for i in range(n):
            self._dispatch_event(raw_events[i], is_procinfo=False, timestamp_ns=timestamps[i])
    
    def _dispatch_event(
        self, 
        raw: RawEvent, 
        is_procinfo: bool, 
        timestamp_ns: int
    ) -> None:
        """
        将事件分发到对应的类处理器
        
        所有 handler 的 handle_event 都接收 timestamp_ns 作为必须参数，
        并在内部完成时间戳赋值。
        
        Args:
            raw: 原始事件
            is_procinfo: 是否是 ProcInfo 数据
            timestamp_ns: 已计算的 UTC 时间戳
        """
        int_class = raw.internal_class
        int_event = raw.internal_event
        
        # 转换为外部事件类
        ext_class, ext_event = internal_to_external(int_class, int_event)
        
        # ProcInfo 数据只处理 PROCESS 事件
        if is_procinfo and ext_class != ExternalClass.PROCESS:
            return
        
        # 根据内部类分发
        if int_class == InternalClass.PR_TH:
            if ext_class == ExternalClass.THREAD:
                self._thread_handler.handle_event(raw, ext_event, timestamp_ns)
            elif ext_class == ExternalClass.VTHREAD:
                self._vthread_handler.handle_event(raw, ext_event, timestamp_ns)
            elif ext_class == ExternalClass.PROCESS:
                self._process_handler.handle_event(raw, ext_event, timestamp_ns)
        
        elif int_class == InternalClass.CONTROL:
            self._control_handler.handle_event(raw, ext_event, timestamp_ns)
        
        elif int_class == InternalClass.KER_CALL:
            # KerCall 事件根据事件号范围区分 ENTER/EXIT/INT
            # ext_class 已经是正确的外部类 (KERCALLENTER/KERCALLEXIT/KERCALLINT)
            self._kercall_handler.handle_event(
                raw, ext_event, timestamp_ns, ext_class=ext_class
            )
        
        elif int_class == InternalClass.INT:
            # INT 事件
            # ext_event 实际上是 int_event (1,2,3,4,5,6)，用于判断 64 位模式
            # 中断号存储在 raw.data[1] 中
            self._interrupt_handler.handle_event(
                raw, ext_event, timestamp_ns, ext_class=ext_class
            )
        
        elif int_class == InternalClass.COMM:
            self._comm_handler.handle_event(raw, ext_event, timestamp_ns)
        
        elif int_class == InternalClass.SYSTEM:
            self._system_handler.handle_event(raw, ext_event, timestamp_ns)
        
        elif int_class == InternalClass.USER:
            # USER 类事件暂未实现
            pass
        
        elif int_class == InternalClass.QUIP:
            self._quip_handler.handle_event(raw, ext_event, timestamp_ns)
        
        elif int_class == InternalClass.SEC:
            self._sec_handler.handle_event(raw, ext_event, timestamp_ns)
        
        elif int_class == InternalClass.QVM:
            self._qvm_handler.handle_event(raw, ext_event, timestamp_ns)
    
    def _post_process(self) -> None:
        """后处理：推断信号发送者等"""
        if not self.file_header:
            return
        
        # 推断信号发送者（不再需要计算时间戳，因为已经实时计算了）
        thread_events = self.thread_events
        if thread_events:
            self._kercall_handler.infer_signal_senders(thread_events)
    
    def _print_parse_summary(self) -> None:
        """打印解析摘要"""
        print(f"[解析] 完成:")
        print(f"  线程事件: {len(self.thread_events)}")
        print(f"  信号事件: {len(self.signal_events)}")
        print(f"  进程数: {len(self.process_info)}")
        print(f"  线程名数: {len(self.thread_info)}")
        
        if self._first_timestamp_ns is not None and self._last_timestamp_ns is not None:
            first_dt = datetime.fromtimestamp(
                self._first_timestamp_ns / 1e9,
                tz=timezone.utc
            )
            last_dt = datetime.fromtimestamp(
                self._last_timestamp_ns / 1e9,
                tz=timezone.utc
            )
            duration = (self._last_timestamp_ns - self._first_timestamp_ns) / 1e9
            print(f"  时间范围: {first_dt.isoformat()} ~ {last_dt.isoformat()}")
            print(f"  持续时间: {duration:.3f} 秒")
    
    # ========================================================================
    # 输出和导出
    # ========================================================================
    
    def print_summary(self) -> None:
        """打印摘要"""
        if not self.file_header:
            print("无数据")
            return
        
        thread_events = self.thread_events
        signal_events = self.signal_events
        
        print("\n" + "=" * 60)
        print("QST 文件摘要")
        print("=" * 60)
        print(f"文件:          {self.filepath}")
        print(f"版本:          v{self.file_header.version}")
        print(f"时钟频率:      {self.file_header.clock_freq:,} Hz")
        print(f"线程事件数:    {len(thread_events):,}")
        print(f"信号事件数:    {len(signal_events):,}")
        print(f"进程数:        {len(self.process_info)}")
        print(f"线程名数:      {len(self.thread_info)}")
        
        # 时间信息
        wallclock = self.file_header.get_wallclock_datetime()
        print(f"\n时间信息:")
        print(f"  采集结束时间: {wallclock.strftime('%Y-%m-%d %H:%M:%S.%f')} UTC")
        print(f"  Sync Cycles:  {self.file_header.sync_cycles}")
        
        if thread_events:
            first_ts = thread_events[0].timestamp_ns
            last_ts = thread_events[-1].timestamp_ns
            duration_ns = last_ts - first_ts
            duration_sec = duration_ns / 1e9
            
            first_dt = datetime.fromtimestamp(first_ts / 1e9, tz=timezone.utc)
            last_dt = datetime.fromtimestamp(last_ts / 1e9, tz=timezone.utc)
            
            print(f"  第一个事件:   {first_dt.strftime('%Y-%m-%d %H:%M:%S.%f')} UTC")
            print(f"  最后一个事件: {last_dt.strftime('%Y-%m-%d %H:%M:%S.%f')} UTC")
            print(f"  持续时间:     {duration_sec:.3f} 秒")
        
        # CPU 分布统计
        cpu_counts: Dict[int, int] = defaultdict(int)
        for event in thread_events:
            cpu_counts[event.cpu_id] += 1
        
        if cpu_counts:
            print("\nCPU 分布:")
            for cpu_id in sorted(cpu_counts.keys()):
                print(f"  CPU {cpu_id}: {cpu_counts[cpu_id]:,} 事件")
        
        # 状态统计
        state_counts: Dict[str, int] = {}
        for event in thread_events:
            state_counts[event.state] = state_counts.get(event.state, 0) + 1
        
        if state_counts:
            print("\n线程状态分布:")
            for state, count in sorted(state_counts.items(), key=lambda x: -x[1])[:10]:
                print(f"  {state:15s}: {count:8,}")
        
        # 进程列表
        if self.process_info:
            print("\n进程列表:")
            for pid, name in sorted(self.process_info.items())[:20]:
                print(f"  {pid:6d}: {name}")
            if len(self.process_info) > 20:
                print(f"  ... 共 {len(self.process_info)} 个进程")
        
        # 信号事件列表
        if signal_events:
            print("\n信号事件:")
            for i, sig in enumerate(signal_events[:10]):
                sender = f"PID {sig.sender_pid}" if sig.sender_pid else "未知"
                ts_str = ""
                if sig.timestamp_ns > 0:
                    dt = datetime.fromtimestamp(sig.timestamp_ns / 1e9, tz=timezone.utc)
                    ts_str = dt.strftime('%H:%M:%S.%f')
                print(f"  [{i+1}] {ts_str} {sig.signal_name} ({sig.signo}): "
                      f"{sender} → PID {sig.target_pid} TID {sig.target_tid}")
            if len(signal_events) > 10:
                print(f"  ... 共 {len(signal_events)} 个信号事件")
        
        print("=" * 60 + "\n")
    
