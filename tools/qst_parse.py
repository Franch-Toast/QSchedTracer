#!/usr/bin/env python3
"""
qst_parse.py - QST 文件解析器 (v3)

功能:
1. 解析 .qst 二进制文件 (v3 格式)
2. 从事件 header 提取 CPU ID
3. 从 PROCDESTROY 事件提取进程/线程名称
4. 计算真实 UNIX 时间戳 (从最后一个事件向前推算)
5. 导出为 Chrome Trace JSON (Perfetto 兼容)

文件格式 (v3):
    ┌──────────────────────────────────────────────────────────────┐
    │ FileHeader (64 字节)                                         │
    │   - magic: 'QST3' (0x51535433)                               │
    │   - clock_freq, wallclock_sec, wallclock_nsec, sync_cycles   │
    ├──────────────────────────────────────────────────────────────┤
    │ ProcInfoHeader (16 字节)                                     │
    │   - magic: 'PINF', event_count                               │
    ├──────────────────────────────────────────────────────────────┤
    │ ProcInfo 数据 (仅提取 PROCDESTROY 获取名称)                  │
    ├──────────────────────────────────────────────────────────────┤
    │ MainDataHeader (16 字节)                                     │
    │   - magic: 'MAIN', event_count                               │
    ├──────────────────────────────────────────────────────────────┤
    │ 主调度数据 (按时间顺序)                                      │
    │   - 最后一个事件的 data[0] == sync_cycles                    │
    └──────────────────────────────────────────────────────────────┘

时间戳计算:
    - 最后一个事件时间 = wallclock
    - 从后向前推算: event_time = next_time - delta_cycles / clock_freq
    - 自动处理 32 位 cycles 回绕

用法:
    python3 qst_parse.py trace.qst              # 显示摘要
    python3 qst_parse.py trace.qst -o trace.json # 导出 Perfetto JSON
    python3 qst_parse.py trace.qst -v           # 详细输出
"""

import struct
import json
import sys
import argparse
from dataclasses import dataclass
from typing import Dict, List, Tuple, Optional, Any
from enum import IntEnum
from collections import defaultdict
from datetime import datetime, timezone

# ============================================================================
# 常量定义
# ============================================================================

# 文件格式
QST_MAGIC = 0x51535433      # 'QST3'
QST_VERSION = 3
QST_EVENT_SIZE = 16

# 文件头大小
FILE_HEADER_SIZE = 64
PROCINFO_HEADER_SIZE = 16
MAINDATA_HEADER_SIZE = 16

# 魔数
PROCINFO_MAGIC = 0x50494E46  # 'PINF'
MAINDATA_MAGIC = 0x4D41494E  # 'MAIN'

# 32 位最大值
MAX_UINT32 = 0xFFFFFFFF

# 内部事件类
class InternalClass(IntEnum):
    EMPTY = 0
    CONTROL = 1
    KER_CALL = 2
    INT = 3
    PR_TH = 4      # PROCESS/THREAD/VTHREAD
    SYSTEM = 5
    USER = 6
    COMM = 7

# 外部事件类
class ExternalClass(IntEnum):
    THREAD = 1
    VTHREAD = 2
    PROCESS = 3
    CONTROL = 4

# 线程状态
THREAD_STATES = {
    0: "DEAD", 1: "RUNNING", 2: "READY", 3: "STOPPED",
    4: "SEND", 5: "RECEIVE", 6: "REPLY", 7: "STACK",
    8: "WAITTHREAD", 9: "WAITPAGE", 10: "SIGSUSPEND",
    11: "SIGWAITINFO", 12: "NANOSLEEP", 13: "MUTEX",
    14: "CONDVAR", 15: "JOIN", 16: "INTR", 17: "SEM",
    18: "WAITCTX", 19: "NET_SEND", 20: "NET_REPLY",
}

# PROCESS 事件类型
PROCESS_EVENTS = {
    1: "PROCCREATE",
    2: "PROCCREATE_NAME",
    4: "PROCDESTROY",
    8: "PROCDESTROY_NAME",
    16: "PROCTHREAD_NAME",
}

# 结构类型
class StructType(IntEnum):
    SIMPLE = 0
    COMBINE_BEGIN = 1
    COMBINE_CONT = 2
    COMBINE_END = 3

# Perfetto 颜色
STATE_COLORS = {
    "RUNNING": "good",
    "READY": "olive",
    "RECEIVE": "rail_load",
    "SEND": "rail_response",
    "REPLY": "rail_animation",
    "MUTEX": "bad",
    "CONDVAR": "terrible",
    "SEM": "bad",
    "NANOSLEEP": "thread_state_sleeping",
    "DEAD": "black",
    "STOPPED": "grey",
    "INTR": "yellow",
}

# ============================================================================
# 数据结构
# ============================================================================

@dataclass
class FileHeader:
    """QST 文件头 (v3, 64 字节)"""
    magic: int = 0
    version: int = 0
    clock_freq: int = 0
    wallclock_sec: int = 0
    wallclock_nsec: int = 0
    sync_cycles: int = 0
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'FileHeader':
        if len(data) < FILE_HEADER_SIZE:
            raise ValueError(f"文件头数据太小: {len(data)} < {FILE_HEADER_SIZE}")
        
        # 解析: magic(4) + version(4) + clock_freq(8) + wallclock_sec(8) + wallclock_nsec(8) + sync_cycles(4) + reserved(28)
        magic, version, clock_freq, wallclock_sec, wallclock_nsec, sync_cycles = \
            struct.unpack('<IIQqqI', data[:36])
        
        if magic != QST_MAGIC:
            raise ValueError(f"无效的文件魔数: 0x{magic:08X} (期望 0x{QST_MAGIC:08X})")
        
        if version != QST_VERSION:
            raise ValueError(f"不支持的文件版本: {version} (仅支持 v{QST_VERSION})")
        
        return cls(
            magic=magic,
            version=version,
            clock_freq=clock_freq,
            wallclock_sec=wallclock_sec,
            wallclock_nsec=wallclock_nsec,
            sync_cycles=sync_cycles,
        )
    
    def get_wallclock_ns(self) -> int:
        """获取 wallclock 纳秒时间戳"""
        return self.wallclock_sec * 1_000_000_000 + self.wallclock_nsec
    
    def get_wallclock_datetime(self) -> datetime:
        """获取 wallclock 日期时间"""
        return datetime.fromtimestamp(
            self.wallclock_sec + self.wallclock_nsec / 1e9,
            tz=timezone.utc
        )


@dataclass
class ProcInfoHeader:
    """进程/线程信息头 (16 字节)"""
    magic: int = 0
    event_count: int = 0
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'ProcInfoHeader':
        if len(data) < PROCINFO_HEADER_SIZE:
            raise ValueError(f"ProcInfoHeader 数据太小")
        
        magic, event_count, reserved = struct.unpack('<IIQ', data[:16])
        
        if magic != PROCINFO_MAGIC:
            raise ValueError(f"无效的 ProcInfoHeader 魔数: 0x{magic:08X}")
        
        return cls(magic=magic, event_count=event_count)


@dataclass
class MainDataHeader:
    """主调度数据头 (16 字节)"""
    magic: int = 0
    event_count: int = 0
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'MainDataHeader':
        if len(data) < MAINDATA_HEADER_SIZE:
            raise ValueError(f"MainDataHeader 数据太小")
        
        magic, event_count, reserved = struct.unpack('<IIQ', data[:16])
        
        if magic != MAINDATA_MAGIC:
            raise ValueError(f"无效的 MainDataHeader 魔数: 0x{magic:08X}")
        
        return cls(magic=magic, event_count=event_count)


@dataclass
class RawEvent:
    """原始事件 (16 字节)"""
    header: int = 0
    data: Tuple[int, int, int] = (0, 0, 0)
    cpu_id: int = 0
    struct_type: int = 0
    internal_class: int = 0
    internal_event: int = 0
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'RawEvent':
        header, d0, d1, d2 = struct.unpack('<IIII', data[:16])
        
        cpu_id = (header >> 24) & 0x3F
        struct_type = (header >> 30) & 0x3
        internal_class = (header >> 10) & 0x1F
        internal_event = header & 0x3FF
        
        return cls(
            header=header,
            data=(d0, d1, d2),
            cpu_id=cpu_id,
            struct_type=struct_type,
            internal_class=internal_class,
            internal_event=internal_event,
        )


@dataclass
class ThreadEvent:
    """解析后的线程状态事件"""
    timestamp_cycles: int       # 原始 32 位时间戳 (cycles)
    timestamp_ns: int = 0       # 计算后的 UNIX 纳秒时间戳
    cpu_id: int = 0
    pid: int = 0
    tid: int = 0
    state: str = ""
    is_vthread: bool = False


# ============================================================================
# 辅助函数
# ============================================================================

def internal_to_external(int_class: int, int_event: int) -> Tuple[int, int]:
    """将内部事件类/事件号转换为外部格式"""
    MAX_TH_STATE_NUM = 26
    
    if int_class == InternalClass.PR_TH:
        if int_event >= (2 * MAX_TH_STATE_NUM):
            ext_class = ExternalClass.PROCESS
            ext_event = 1 << ((int_event >> 6) - 1)
        elif int_event >= MAX_TH_STATE_NUM:
            ext_class = ExternalClass.VTHREAD
            ext_event = int_event - MAX_TH_STATE_NUM
        else:
            ext_class = ExternalClass.THREAD
            ext_event = int_event
        return ext_class, ext_event
    elif int_class == InternalClass.CONTROL:
        return ExternalClass.CONTROL, int_event
    
    return -1, -1


def calculate_timestamps_backward(events: List[ThreadEvent], 
                                   clock_freq: int, 
                                   wallclock_ns: int) -> None:
    """
    从后向前计算每个事件的真实 UNIX 时间戳
    
    Args:
        events: 事件列表 (按时间顺序，从早到晚)
        clock_freq: 时钟频率 (Hz)
        wallclock_ns: 采集结束时的 UNIX 纳秒时间戳
    """
    n = len(events)
    if n == 0:
        return
    
    # 最后一个事件的时间 = wallclock
    events[-1].timestamp_ns = wallclock_ns
    
    # 从倒数第二个事件开始，向前推算
    for i in range(n - 2, -1, -1):
        curr_cycles = events[i].timestamp_cycles
        next_cycles = events[i + 1].timestamp_cycles
        
        # 计算 cycles 差值
        delta_cycles = next_cycles - curr_cycles
        
        # 处理 32 位回绕
        if delta_cycles < 0:
            delta_cycles += MAX_UINT32 + 1
        
        # 计算时间差 (纳秒)
        delta_ns = (delta_cycles * 1_000_000_000) // clock_freq
        
        # 当前事件时间 = 下一个事件时间 - 时间差
        events[i].timestamp_ns = events[i + 1].timestamp_ns - delta_ns


# ============================================================================
# QST 文件解析器
# ============================================================================

class QstParser:
    """QST 文件解析器 (v3)"""
    
    def __init__(self, filepath: str, verbose: bool = False):
        self.filepath = filepath
        self.verbose = verbose
        
        self.file_header: Optional[FileHeader] = None
        self.procinfo_header: Optional[ProcInfoHeader] = None
        self.maindata_header: Optional[MainDataHeader] = None
        
        self.thread_events: List[ThreadEvent] = []
        self.process_info: Dict[int, str] = {}      # pid -> name
        self.thread_info: Dict[Tuple[int, int], str] = {}  # (pid, tid) -> name
        
        self._combine_buffer: Dict[str, dict] = {}
    
    def parse(self) -> None:
        """解析 QST 文件"""
        with open(self.filepath, 'rb') as f:
            # ================================================================
            # 1. 读取 FileHeader (64 字节)
            # ================================================================
            file_header_data = f.read(FILE_HEADER_SIZE)
            self.file_header = FileHeader.from_bytes(file_header_data)
            
            if self.verbose:
                print(f"[解析] 文件版本: v{self.file_header.version}")
                print(f"[解析] 时钟频率: {self.file_header.clock_freq:,} Hz")
                print(f"[解析] Wallclock: {self.file_header.get_wallclock_datetime().isoformat()}")
                print(f"[解析] Sync Cycles: {self.file_header.sync_cycles}")
            
            # ================================================================
            # 2. 读取 ProcInfoHeader (16 字节) + ProcInfo 数据
            # ================================================================
            procinfo_header_data = f.read(PROCINFO_HEADER_SIZE)
            self.procinfo_header = ProcInfoHeader.from_bytes(procinfo_header_data)
            
            procinfo_size = self.procinfo_header.event_count * QST_EVENT_SIZE
            procinfo_data = f.read(procinfo_size)
            
            if self.verbose:
                print(f"[解析] ProcInfo 事件数: {self.procinfo_header.event_count}")
            
            # 解析 ProcInfo 数据 (仅提取名称)
            self._parse_procinfo_events(procinfo_data)
            
            # ================================================================
            # 3. 读取 MainDataHeader (16 字节) + 主调度数据
            # ================================================================
            maindata_header_data = f.read(MAINDATA_HEADER_SIZE)
            self.maindata_header = MainDataHeader.from_bytes(maindata_header_data)
            
            main_data_size = self.maindata_header.event_count * QST_EVENT_SIZE
            main_data = f.read(main_data_size)
            
            if self.verbose:
                print(f"[解析] 主调度事件数: {self.maindata_header.event_count}")
            
            # 解析主调度数据
            self._parse_main_events(main_data)
        
        # ================================================================
        # 4. 计算时间戳 (从后向前)
        # ================================================================
        if self.thread_events and self.file_header:
            calculate_timestamps_backward(
                self.thread_events,
                self.file_header.clock_freq,
                self.file_header.get_wallclock_ns()
            )
        
        if self.verbose:
            print(f"[解析] 完成:")
            print(f"  线程事件: {len(self.thread_events)}")
            print(f"  进程数: {len(self.process_info)}")
            print(f"  线程名数: {len(self.thread_info)}")
    
    def _parse_procinfo_events(self, data: bytes) -> None:
        """解析 ProcInfo 数据 (仅提取名称)"""
        event_count = len(data) // QST_EVENT_SIZE
        
        for i in range(event_count):
            offset = i * QST_EVENT_SIZE
            event_data = data[offset:offset + QST_EVENT_SIZE]
            if len(event_data) == QST_EVENT_SIZE:
                raw = RawEvent.from_bytes(event_data)
                ext_class, ext_event = internal_to_external(
                    raw.internal_class, raw.internal_event
                )
                
                # 只处理 PROCESS 类事件 (包含名称)
                if ext_class == ExternalClass.PROCESS:
                    self._parse_process_event(raw, ext_event)
    
    def _parse_main_events(self, data: bytes) -> None:
        """解析主调度数据
        
        注意: 会检测时间跳跃来过滤污染数据。
        污染数据可能来自于采集器在结束时调用 _NTO_TRACE_START 获取进程信息，
        如果采集器版本较旧，这些数据会被错误地写入主缓冲区。
        """
        event_count = len(data) // QST_EVENT_SIZE
        
        # 时间跳跃阈值: 10ms (用于检测污染数据)
        jump_threshold_cycles = int(self.file_header.clock_freq * 0.01) if self.file_header else 192000
        
        prev_cycles = None
        filtered_count = 0
        
        for i in range(event_count):
            offset = i * QST_EVENT_SIZE
            event_data = data[offset:offset + QST_EVENT_SIZE]
            if len(event_data) == QST_EVENT_SIZE:
                raw = RawEvent.from_bytes(event_data)
                curr_cycles = raw.data[0]
                
                # 检测时间跳跃 (污染数据检测)
                if prev_cycles is not None:
                    cycle_diff = curr_cycles - prev_cycles
                    if cycle_diff > jump_threshold_cycles:
                        # 发现大的时间跳跃，停止解析
                        # 后续数据可能是 _NTO_TRACE_START 产生的污染数据
                        filtered_count = event_count - i
                        if self.verbose:
                            jump_ms = cycle_diff / self.file_header.clock_freq * 1000
                            print(f"[警告] 在事件 {i} 处检测到 {jump_ms:.2f}ms 的时间跳跃")
                            print(f"[警告] 过滤掉 {filtered_count} 个可能污染的事件")
                        break
                
                prev_cycles = curr_cycles
                
                ext_class, ext_event = internal_to_external(
                    raw.internal_class, raw.internal_event
                )
                
                if ext_class == ExternalClass.THREAD:
                    self._parse_thread_event(raw, ext_event, is_vthread=False)
                elif ext_class == ExternalClass.VTHREAD:
                    self._parse_thread_event(raw, ext_event, is_vthread=True)
                elif ext_class == ExternalClass.PROCESS:
                    # 主调度数据中也可能有 PROCESS 事件
                    self._parse_process_event(raw, ext_event)
    
    def _parse_thread_event(self, raw: RawEvent, state_code: int, 
                            is_vthread: bool) -> None:
        """解析 THREAD/VTHREAD 事件"""
        timestamp = raw.data[0]
        pid = raw.data[1]
        tid = raw.data[2]
        state = THREAD_STATES.get(state_code, f"STATE_{state_code}")
        
        event = ThreadEvent(
            timestamp_cycles=timestamp,
            cpu_id=raw.cpu_id,
            pid=pid,
            tid=tid,
            state=state,
            is_vthread=is_vthread,
        )
        self.thread_events.append(event)
    
    def _parse_process_event(self, raw: RawEvent, event_type: int) -> None:
        """解析 PROCESS 事件"""
        struct_type = raw.struct_type
        
        if event_type == 2:  # PROCCREATE_NAME
            self._handle_proccreate_name(raw, struct_type)
        elif event_type == 4:  # PROCDESTROY
            self._handle_procdestroy(raw, struct_type)
        elif event_type == 16:  # PROCTHREAD_NAME
            self._handle_procthread_name(raw, struct_type)
    
    def _handle_proccreate_name(self, raw: RawEvent, struct_type: int) -> None:
        """处理 PROCCREATE_NAME 组合事件"""
        key = f"proccreate_{raw.cpu_id}"
        
        if struct_type in (StructType.SIMPLE, StructType.COMBINE_BEGIN):
            self._combine_buffer[key] = {
                'ppid': raw.data[1],
                'pid': raw.data[2],
                'name_bytes': bytearray(),
            }
        else:
            if key in self._combine_buffer:
                buf = self._combine_buffer[key]
                buf['name_bytes'].extend(struct.pack('<II', raw.data[1], raw.data[2]))
                
                if struct_type == StructType.COMBINE_END:
                    name = buf['name_bytes'].rstrip(b'\x00').decode('utf-8', errors='replace')
                    self.process_info[buf['pid']] = name
                    del self._combine_buffer[key]
    
    def _handle_procdestroy(self, raw: RawEvent, struct_type: int) -> None:
        """处理 PROCDESTROY 组合事件"""
        key = f"procdestroy_{raw.cpu_id}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffer[key] = {
                'ppid': raw.data[1],
                'pid': raw.data[2],
                'name_bytes': bytearray(),
            }
        elif struct_type in (StructType.COMBINE_CONT, StructType.COMBINE_END):
            if key in self._combine_buffer:
                buf = self._combine_buffer[key]
                buf['name_bytes'].extend(struct.pack('<II', raw.data[1], raw.data[2]))
                
                if struct_type == StructType.COMBINE_END:
                    full_path = buf['name_bytes'].rstrip(b'\x00').decode('utf-8', errors='replace')
                    name = full_path.split('/')[-1] if '/' in full_path else full_path
                    name = name.split('\x00')[0].strip()
                    if buf['pid'] not in self.process_info:
                        self.process_info[buf['pid']] = name
                    del self._combine_buffer[key]
    
    def _handle_procthread_name(self, raw: RawEvent, struct_type: int) -> None:
        """处理 PROCTHREAD_NAME 组合事件"""
        key = f"thread_{raw.cpu_id}"
        
        if struct_type in (StructType.SIMPLE, StructType.COMBINE_BEGIN):
            self._combine_buffer[key] = {
                'pid': raw.data[1],
                'tid': raw.data[2],
                'name_bytes': bytearray(),
            }
        else:
            if key in self._combine_buffer:
                buf = self._combine_buffer[key]
                buf['name_bytes'].extend(struct.pack('<II', raw.data[1], raw.data[2]))
                
                if struct_type == StructType.COMBINE_END:
                    name = buf['name_bytes'].rstrip(b'\x00').decode('utf-8', errors='replace')
                    self.thread_info[(buf['pid'], buf['tid'])] = name
                    del self._combine_buffer[key]
    
    def get_thread_name(self, pid: int, tid: int) -> str:
        """获取线程名称"""
        if (pid, tid) in self.thread_info:
            return self.thread_info[(pid, tid)]
        if pid in self.process_info:
            name = self.process_info[pid]
            return name if tid == 1 else f"{name}:{tid}"
        return f"unknown:{pid}:{tid}"
    
    def get_process_name(self, pid: int) -> str:
        """获取进程名称"""
        return self.process_info.get(pid, f"unknown:{pid}")
    
    def print_summary(self) -> None:
        """打印摘要"""
        if not self.file_header:
            print("无数据")
            return
        
        print("\n" + "=" * 60)
        print("QST 文件摘要")
        print("=" * 60)
        print(f"文件:          {self.filepath}")
        print(f"版本:          v{self.file_header.version}")
        print(f"时钟频率:      {self.file_header.clock_freq:,} Hz")
        print(f"线程事件数:    {len(self.thread_events):,}")
        print(f"进程数:        {len(self.process_info)}")
        print(f"线程名数:      {len(self.thread_info)}")
        
        # 时间信息
        wallclock = self.file_header.get_wallclock_datetime()
        print(f"\n时间信息:")
        print(f"  采集结束时间: {wallclock.strftime('%Y-%m-%d %H:%M:%S.%f')} UTC")
        print(f"  Sync Cycles:  {self.file_header.sync_cycles}")
        
        if self.thread_events:
            first_ts = self.thread_events[0].timestamp_ns
            last_ts = self.thread_events[-1].timestamp_ns
            duration_ns = last_ts - first_ts
            duration_sec = duration_ns / 1e9
            
            first_dt = datetime.fromtimestamp(first_ts / 1e9, tz=timezone.utc)
            last_dt = datetime.fromtimestamp(last_ts / 1e9, tz=timezone.utc)
            
            print(f"  第一个事件:   {first_dt.strftime('%Y-%m-%d %H:%M:%S.%f')} UTC")
            print(f"  最后一个事件: {last_dt.strftime('%Y-%m-%d %H:%M:%S.%f')} UTC")
            print(f"  持续时间:     {duration_sec:.3f} 秒")
        
        # CPU 分布统计
        cpu_counts: Dict[int, int] = defaultdict(int)
        for event in self.thread_events:
            cpu_counts[event.cpu_id] += 1
        
        if cpu_counts:
            print("\nCPU 分布:")
            for cpu_id in sorted(cpu_counts.keys()):
                print(f"  CPU {cpu_id}: {cpu_counts[cpu_id]:,} 事件")
        
        # 状态统计
        state_counts: Dict[str, int] = {}
        for event in self.thread_events:
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
        
        print("=" * 60 + "\n")
    
    def to_perfetto_json(self, output_path: str) -> None:
        """导出为 Perfetto JSON (Chrome Trace Event Format)"""
        if not self.file_header or not self.thread_events:
            print("无事件可导出")
            return
        
        trace_events: List[Dict[str, Any]] = []
        
        # === 1. 元数据事件 ===
        seen_processes = set()
        seen_threads = set()
        
        for event in self.thread_events:
            pid, tid = event.pid, event.tid
            
            if pid not in seen_processes:
                seen_processes.add(pid)
                trace_events.append({
                    "name": "process_name",
                    "ph": "M",
                    "pid": pid,
                    "args": {"name": self.get_process_name(pid)}
                })
            
            if (pid, tid) not in seen_threads:
                seen_threads.add((pid, tid))
                trace_events.append({
                    "name": "thread_name",
                    "ph": "M",
                    "pid": pid,
                    "tid": tid,
                    "args": {"name": self.get_thread_name(pid, tid)}
                })
        
        # === 2. 线程状态事件 ===
        thread_events_map: Dict[Tuple[int, int], List[ThreadEvent]] = defaultdict(list)
        for event in self.thread_events:
            thread_events_map[(event.pid, event.tid)].append(event)
        
        for (pid, tid), events in thread_events_map.items():
            # 按时间戳排序
            events.sort(key=lambda e: e.timestamp_ns)
            
            last_valid_ts = -1
            for i, event in enumerate(events):
                ts_us = event.timestamp_ns // 1000  # 转换为微秒
                
                if ts_us <= last_valid_ts:
                    continue
                
                # 计算持续时间
                duration_us = 1
                for j in range(i + 1, len(events)):
                    next_ts_us = events[j].timestamp_ns // 1000
                    if next_ts_us > ts_us:
                        duration_us = next_ts_us - ts_us
                        break
                
                # 限制持续时间
                if duration_us <= 0:
                    duration_us = 1
                elif duration_us > 10_000_000:  # 10 秒
                    duration_us = 1
                
                color = STATE_COLORS.get(event.state, "generic_work")
                
                trace_events.append({
                    "name": event.state,
                    "cat": "thread_state",
                    "ph": "X",
                    "ts": ts_us,
                    "dur": duration_us,
                    "pid": pid,
                    "tid": tid,
                    "args": {
                        "state": event.state,
                        "cpu": event.cpu_id,
                        "vthread": event.is_vthread,
                    },
                    "cname": color,
                })
                
                last_valid_ts = ts_us
        
        # === 3. CPU 运行轨道 ===
        cpu_tracks: Dict[int, List[ThreadEvent]] = defaultdict(list)
        for event in self.thread_events:
            if event.state == "RUNNING":
                cpu_tracks[event.cpu_id].append(event)
        
        for cpu_id, events in cpu_tracks.items():
            events.sort(key=lambda e: e.timestamp_ns)
            
            last_valid_ts = -1
            for i, event in enumerate(events):
                ts_us = event.timestamp_ns // 1000
                
                if ts_us <= last_valid_ts:
                    continue
                
                duration_us = 1
                for j in range(i + 1, len(events)):
                    next_ts_us = events[j].timestamp_ns // 1000
                    if next_ts_us > ts_us:
                        duration_us = next_ts_us - ts_us
                        break
                
                if duration_us <= 0:
                    duration_us = 1
                elif duration_us > 10_000_000:
                    duration_us = 1
                
                trace_events.append({
                    "name": self.get_thread_name(event.pid, event.tid),
                    "cat": "cpu",
                    "ph": "X",
                    "ts": ts_us,
                    "dur": duration_us,
                    "pid": 0,
                    "tid": cpu_id,
                    "args": {
                        "pid": event.pid,
                        "tid": event.tid,
                    },
                    "cname": "good",
                })
                
                last_valid_ts = ts_us
        
        # CPU 轨道元数据
        for cpu_id in cpu_tracks.keys():
            trace_events.append({
                "name": "thread_name",
                "ph": "M",
                "pid": 0,
                "tid": cpu_id,
                "args": {"name": f"CPU {cpu_id}"}
            })
        
        if cpu_tracks:
            trace_events.append({
                "name": "process_name",
                "ph": "M",
                "pid": 0,
                "args": {"name": "CPUs"}
            })
            trace_events.append({
                "name": "process_sort_index",
                "ph": "M",
                "pid": 0,
                "args": {"sort_index": -1000}
            })
        
        # === 4. 构建输出 ===
        
        # 计算时间范围
        first_ts = self.thread_events[0].timestamp_ns
        last_ts = self.thread_events[-1].timestamp_ns
        first_dt = datetime.fromtimestamp(first_ts / 1e9, tz=timezone.utc)
        last_dt = datetime.fromtimestamp(last_ts / 1e9, tz=timezone.utc)
        
        output = {
            "traceEvents": trace_events,
            "displayTimeUnit": "ms",
            "otherData": {
                "source": "QSchedTracer (qst_parse.py v3)",
                "file": self.filepath,
                "trace_start_time": first_dt.isoformat(),
                "trace_end_time": last_dt.isoformat(),
                "duration_sec": (last_ts - first_ts) / 1e9,
                "total_events": len(self.thread_events),
                "processes": len(self.process_info),
                "threads_with_names": len(self.thread_info),
                "clock_freq_hz": self.file_header.clock_freq,
            }
        }
        
        # 写入文件
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(output, f, indent=2, ensure_ascii=False)
        
        print(f"导出完成: {output_path}")
        print(f"  事件数: {len(trace_events)}")
        print(f"  进程数: {len(seen_processes)}")
        print(f"  线程数: {len(seen_threads)}")
        print(f"  CPU 数: {len(cpu_tracks)}")
        print(f"  时间范围: {first_dt.isoformat()} ~ {last_dt.isoformat()}")
        print(f"\n在 https://ui.perfetto.dev/ 中打开查看")


# ============================================================================
# 主函数
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description='QST 文件解析器 - QNX 调度追踪数据解析工具 (v3)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s trace.qst                  显示摘要
  %(prog)s trace.qst -o trace.json    导出 Perfetto JSON
  %(prog)s trace.qst -v               详细输出

时间戳:
  使用 UNIX 时间戳 (微秒)，Perfetto 会正确显示绝对时间。
""")
    
    parser.add_argument('input', help='输入 .qst 文件')
    parser.add_argument('-o', '--output', help='输出 JSON 文件')
    parser.add_argument('-v', '--verbose', action='store_true', help='详细输出')
    
    args = parser.parse_args()
    
    try:
        qst = QstParser(args.input, verbose=args.verbose)
        qst.parse()
        qst.print_summary()
        
        if args.output:
            qst.to_perfetto_json(args.output)
    
    except FileNotFoundError:
        print(f"错误: 文件不存在: {args.input}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"错误: {e}", file=sys.stderr)
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
