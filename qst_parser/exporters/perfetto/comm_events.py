"""
通信事件写入器

处理 COMM 类事件，写入到 Thread Track
- SMSG/RMSG: 消息发送/接收 (Instant)
- REPLY/ERROR: 回复/错误 (Instant)
- SPULSE/RPULSE: 脉冲 (Instant)
- SIGNAL: 信号 (Instant)

所有通信事件都是 Instant Events，表示某个时刻发生的事件。
"""

from typing import List, Dict, Tuple, TYPE_CHECKING

from .tracks import TrackManager
from .utils import write_instant

if TYPE_CHECKING:
    from perfetto.trace_builder.proto_builder import TraceProtoBuilder


class CommEventWriter:
    """通信事件写入器"""
    
    def __init__(self, builder: "TraceProtoBuilder", track_manager: TrackManager):
        self._builder = builder
        self._tracks = track_manager
        self._msg_count = 0
        self._reply_count = 0
        self._pulse_count = 0
        self._signal_count = 0
    
    def write_comm_events(
        self,
        comm_handler,
        running_map: Dict[int, List[Tuple[int, int, int]]]
    ) -> Dict[str, int]:
        """
        写入通信事件
        
        Args:
            comm_handler: CommClassHandler 实例
            running_map: CPU -> [(timestamp, pid, tid), ...] 映射
        
        Returns:
            各类事件的计数
        """
        self._msg_count = 0
        self._reply_count = 0
        self._pulse_count = 0
        self._signal_count = 0
        
        if not comm_handler:
            return self._get_counts()
        
        # 消息事件 (SMSG/RMSG)
        self._write_msg_events(comm_handler.msg_events, running_map)
        
        # 回复事件 (REPLY/ERROR)
        self._write_reply_events(comm_handler.reply_events, running_map)
        
        # 脉冲事件 (SPULSE/RPULSE/...)
        self._write_pulse_events(comm_handler.pulse_events, running_map)
        
        # 信号事件 (SIGNAL)
        self._write_signal_events(comm_handler.signal_events, running_map)
        
        return self._get_counts()
    
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
    
    def _write_msg_events(self, events: List, running_map: Dict):
        """写入消息事件"""
        for e in events:
            pid, tid = self._find_thread(running_map, e.cpu_id, e.timestamp_ns)
            if pid == 0 and tid == 0:
                # 使用事件自带的 pid
                pid = e.pid
                tid = 1  # 主线程
            
            track_uuid = self._tracks.get_thread_track(pid, tid)
            if not track_uuid:
                continue
            
            event_name = "SMSG" if e.event_type == "smsg" else "RMSG"
            args = {
                "event_type": e.event_type,
                "rcvid": e.rcvid,
                "pid": e.pid,
                "cpu": e.cpu_id,
                "is_wide": getattr(e, 'is_wide', False),
            }
            write_instant(self._builder, e.timestamp_ns, track_uuid, event_name, args)
            self._msg_count += 1
    
    def _write_reply_events(self, events: List, running_map: Dict):
        """写入回复/错误事件"""
        for e in events:
            # 使用事件中的 pid 和 tid（tid 已在解析时从复合值中提取）
            pid = e.pid
            tid = e.tid  # 这是真正的 tid（已从低 16 位提取）
            
            track_uuid = self._tracks.get_thread_track(pid, tid)
            if not track_uuid:
                # 尝试使用 running_map 找到当前 CPU 上运行的线程
                found_pid, found_tid = self._find_thread(running_map, e.cpu_id, e.timestamp_ns)
                if found_pid != 0 and found_tid != 0:
                    track_uuid = self._tracks.get_thread_track(found_pid, found_tid)
                    pid, tid = found_pid, found_tid
            
            if not track_uuid:
                continue
            
            event_name = "REPLY" if e.event_type == "reply" else "ERROR"
            
            # 使用格式化的名称
            thread_name = self._tracks.format_thread_name(pid, tid)
            
            args = {
                "event_type": e.event_type,
                "thread": thread_name,
                "pid": e.pid,
                "tid": e.tid,               # 真正的 tid
                "nd": e.nd,                 # 节点描述符
                "tid_raw": hex(e.tid_raw),  # 原始复合值
                "cpu": e.cpu_id,
                "is_wide": getattr(e, 'is_wide', False),
            }
            write_instant(self._builder, e.timestamp_ns, track_uuid, event_name, args)
            self._reply_count += 1
    
    def _write_pulse_events(self, events: List, running_map: Dict):
        """写入脉冲事件"""
        for e in events:
            pid, tid = self._find_thread(running_map, e.cpu_id, e.timestamp_ns)
            if pid == 0 and tid == 0:
                pid = e.pid
                tid = 1
            
            track_uuid = self._tracks.get_thread_track(pid, tid)
            if not track_uuid:
                continue
            
            # 脉冲类型名称映射
            pulse_names = {
                "spulse": "SPULSE",
                "rpulse": "RPULSE",
                "spulse_exe": "PULSE_EXE",
                "spulse_dis": "PULSE_DIS",
                "spulse_dea": "PULSE_DEA",
                "spulse_un": "PULSE_UN",
                "spulse_qun": "PULSE_QUN",
            }
            event_name = pulse_names.get(e.event_type, "PULSE")
            
            args = {
                "event_type": e.event_type,
                "scoid": e.scoid,
                "pid": e.pid,
                "cpu": e.cpu_id,
                "is_wide": getattr(e, 'is_wide', False),
            }
            write_instant(self._builder, e.timestamp_ns, track_uuid, event_name, args)
            self._pulse_count += 1
    
    def _write_signal_events(self, events: List, running_map: Dict):
        """写入信号事件"""
        # 信号名称映射
        signal_names = {
            1: "SIGHUP", 2: "SIGINT", 3: "SIGQUIT", 4: "SIGILL",
            5: "SIGTRAP", 6: "SIGABRT", 7: "SIGBUS", 8: "SIGFPE",
            9: "SIGKILL", 10: "SIGUSR1", 11: "SIGSEGV", 12: "SIGUSR2",
            13: "SIGPIPE", 14: "SIGALRM", 15: "SIGTERM", 16: "SIGSTKFLT",
            17: "SIGCHLD", 18: "SIGCONT", 19: "SIGSTOP", 20: "SIGTSTP",
        }
        
        for e in events:
            pid, tid = self._find_thread(running_map, e.cpu_id, e.timestamp_ns)
            if pid == 0 and tid == 0:
                continue  # 信号事件必须有线程上下文
            
            track_uuid = self._tracks.get_thread_track(pid, tid)
            if not track_uuid:
                continue
            
            sig_name = signal_names.get(e.si_signo, f"SIG{e.si_signo}")
            args = {
                "signo": e.si_signo,
                "si_code": e.si_code,
                "si_errno": getattr(e, 'si_errno', 0),
                "cpu": e.cpu_id,
                "is_wide": getattr(e, 'is_wide', False),
            }
            write_instant(self._builder, e.timestamp_ns, track_uuid, f"SIGNAL:{sig_name}", args)
            self._signal_count += 1
    
    def _get_counts(self) -> Dict[str, int]:
        return {
            "msg": self._msg_count,
            "reply": self._reply_count,
            "pulse": self._pulse_count,
            "signal": self._signal_count,
        }
