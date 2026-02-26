"""
系统事件写入器

处理 SYSTEM 类事件
- SYS_IPI: 处理器间中断 (Instant on CPU Track)
- SYS_PAGEWAIT: 页面等待 (Instant on Thread Track)

IPI 事件表示跨核调度触发，显示在 CPU Track 上。
PageWait 事件表示内存 I/O 等待，显示在 Thread Track 上。
"""

from typing import List, Dict, Tuple, TYPE_CHECKING

from .tracks import TrackManager
from .utils import write_instant

if TYPE_CHECKING:
    from perfetto.trace_builder.proto_builder import TraceProtoBuilder


class SystemEventWriter:
    """系统事件写入器"""
    
    def __init__(self, builder: "TraceProtoBuilder", track_manager: TrackManager):
        self._builder = builder
        self._tracks = track_manager
        self._ipi_count = 0
        self._pagewait_count = 0
    
    def write_system_events(self, system_handler) -> Dict[str, int]:
        """
        写入系统事件
        
        Args:
            system_handler: SystemClassHandler 实例
        
        Returns:
            各类事件的计数
        """
        self._ipi_count = 0
        self._pagewait_count = 0
        
        if not system_handler:
            return self._get_counts()
        
        # IPI 事件
        self._write_ipi_events(system_handler.ipi_events)
        
        # PageWait 事件
        self._write_pagewait_events(system_handler.pagewait_events)
        
        return self._get_counts()
    
    def _write_ipi_events(self, events: List):
        """
        写入 IPI (处理器间中断) 事件
        
        IPI 表示跨核调度触发，显示在被中断的 CPU Track 上。
        """
        # IPI 命令名称映射 (常见的 IPI 类型)
        ipi_names = {
            0: "IPI_NOP",
            1: "IPI_RESCHED",      # 重新调度
            2: "IPI_TLB_FLUSH",    # TLB 刷新
            3: "IPI_CLOCK",        # 时钟同步
            4: "IPI_DEBUG",        # 调试
        }
        
        for e in events:
            # IPI 显示在 CPU Track 上
            track_uuid = self._tracks.get_cpu_track(e.cpu_id)
            if not track_uuid:
                continue
            
            ipi_name = ipi_names.get(e.ipicmd, f"IPI_{e.ipicmd}")
            write_instant(
                self._builder,
                e.timestamp_ns,
                track_uuid,
                ipi_name,
                {
                    "ipicmd": e.ipicmd,
                    "ip": hex(e.ip) if e.ip else "0x0",
                    "pid": e.pid,
                    "tid": e.tid,
                }
            )
            self._ipi_count += 1
    
    def _write_pagewait_events(self, events: List):
        """
        写入 PageWait (页面等待) 事件
        
        PageWait 表示内存 I/O 等待，显示在对应的 Thread Track 上。
        """
        for e in events:
            track_uuid = self._tracks.get_thread_track(e.pid, e.tid)
            if not track_uuid:
                continue
            
            args = {
                "vaddr": hex(e.vaddr) if e.vaddr else "0x0",
                "ip": hex(e.ip) if e.ip else "0x0",
                "cpu": e.cpu_id,
            }
            
            # Wide mode 有更多信息
            if e.is_wide:
                args["fault_type"] = e.fault_type
                if e.object_name:
                    args["object"] = e.object_name
            
            write_instant(
                self._builder,
                e.timestamp_ns,
                track_uuid,
                "PageWait",
                args
            )
            self._pagewait_count += 1
    
    def _get_counts(self) -> Dict[str, int]:
        return {
            "ipi": self._ipi_count,
            "pagewait": self._pagewait_count,
        }
