"""
QST Parser - 调度相关事件解析器

解析 SCHED_SET, SCHED_YIELD 相关的内核调用事件。

参考: kercall_table_Events.html
"""

from typing import Optional, Dict, Any
from ....models.base import RawEvent
from ....constants import StructType
from ..models import (
    SchedSetEnterEvent, SchedSetExitEvent,
    SchedYieldEnterEvent, SchedYieldExitEvent,
    KerCallEnterEvent, KerCallExitEvent,
)
from ..constants import KernelCall


class SchedEventParser:
    """
    调度相关事件解析器
    
    支持:
    - SCHED_SET: 设置调度参数
    - SCHED_YIELD: 主动让出 CPU
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._combine_buffers: Dict[str, Dict[str, Any]] = {}
    
    # ========================================================================
    # SCHED_SET
    # ========================================================================
    
    def parse_sched_set_enter(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[SchedSetEnterEvent]:
        """
        解析 __KER_SCHED_SET ENTER 事件
        
        Fast mode: pid, sched_priority
        Wide mode: pid, tid, policy, sched_priority, sched_curpriority,
                   ss_low_priority, ss_max_repl, repl_period.tv_sec/nsec, 
                   init_budget.tv_sec/nsec
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            return SchedSetEnterEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=KernelCall.SCHED_SET,
                kercall_name="SCHED_SET",
                is_64bit=is_64bit,
                is_wide=False,
                data1=raw.data[1],
                data2=raw.data[2],
                target_pid=raw.data[1],
                sched_priority=raw.data[2],
            )
        
        return self._parse_sched_set_enter_wide(raw, timestamp_ns, is_64bit, struct_type)
    
    def _parse_sched_set_enter_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool,
        struct_type: int
    ) -> Optional[SchedSetEnterEvent]:
        """解析 SCHED_SET ENTER Wide mode"""
        key = f"sched_set_enter_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffers[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'is_64bit': is_64bit,
                'pid': raw.data[1],
                'tid': raw.data[2],
                'extra_data': [],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, "sched_set_enter_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, "sched_set_enter_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
                
                # Wide: pid, tid, policy, sched_priority, sched_curpriority,
                #       ss_low_priority, ss_max_repl, repl_period.tv_sec/nsec,
                #       init_budget.tv_sec/nsec
                extra = buf['extra_data']
                
                event = SchedSetEnterEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    kercall_num=KernelCall.SCHED_SET,
                    kercall_name="SCHED_SET",
                    is_64bit=buf['is_64bit'],
                    is_wide=True,
                    data1=buf['pid'],
                    data2=buf['tid'],
                    target_pid=buf['pid'],
                    target_tid=buf['tid'],
                    policy=extra[0] if len(extra) > 0 else 0,
                    sched_priority=extra[1] if len(extra) > 1 else 0,
                    sched_curpriority=extra[2] if len(extra) > 2 else 0,
                    ss_low_priority=extra[3] if len(extra) > 3 else 0,
                    ss_max_repl=extra[4] if len(extra) > 4 else 0,
                    repl_period_sec=extra[5] if len(extra) > 5 else 0,
                    repl_period_nsec=extra[6] if len(extra) > 6 else 0,
                    init_budget_sec=extra[7] if len(extra) > 7 else 0,
                    init_budget_nsec=extra[8] if len(extra) > 8 else 0,
                    extra_data={'wide_data': extra},
                )
                
                del self._combine_buffers[matching_key]
                return event
        
        return None
    
    def parse_sched_set_exit(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[SchedSetExitEvent]:
        """
        解析 __KER_SCHED_SET EXIT 事件
        
        Fast/Wide mode: ret_val, empty
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            ret_val = raw.data[1]
            errno_val = raw.data[2] if ret_val == 0xFFFFFFFF else 0
            
            return SchedSetExitEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=KernelCall.SCHED_SET,
                kercall_name="SCHED_SET",
                is_64bit=is_64bit,
                is_wide=False,
                ret_val=ret_val,
                errno_val=errno_val,
            )
        
        return self._parse_simple_exit_wide(
            raw, timestamp_ns, is_64bit, struct_type,
            KernelCall.SCHED_SET, "SCHED_SET", "sched_set_exit_",
            SchedSetExitEvent
        )
    
    # ========================================================================
    # SCHED_YIELD
    # ========================================================================
    
    def parse_sched_yield_enter(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[SchedYieldEnterEvent]:
        """
        解析 __KER_SCHED_YIELD ENTER 事件
        
        Fast/Wide mode: empty, empty
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            return SchedYieldEnterEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=KernelCall.SCHED_YIELD,
                kercall_name="SCHED_YIELD",
                is_64bit=is_64bit,
                is_wide=False,
                data1=raw.data[1],
                data2=raw.data[2],
            )
        
        return self._parse_simple_enter_wide(
            raw, timestamp_ns, is_64bit, struct_type,
            KernelCall.SCHED_YIELD, "SCHED_YIELD", "sched_yield_enter_",
            SchedYieldEnterEvent
        )
    
    def parse_sched_yield_exit(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[SchedYieldExitEvent]:
        """
        解析 __KER_SCHED_YIELD EXIT 事件
        
        Fast/Wide mode: ret_val, empty
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            ret_val = raw.data[1]
            errno_val = raw.data[2] if ret_val == 0xFFFFFFFF else 0
            
            return SchedYieldExitEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=KernelCall.SCHED_YIELD,
                kercall_name="SCHED_YIELD",
                is_64bit=is_64bit,
                is_wide=False,
                ret_val=ret_val,
                errno_val=errno_val,
            )
        
        return self._parse_simple_exit_wide(
            raw, timestamp_ns, is_64bit, struct_type,
            KernelCall.SCHED_YIELD, "SCHED_YIELD", "sched_yield_exit_",
            SchedYieldExitEvent
        )
    
    # ========================================================================
    # 辅助方法
    # ========================================================================
    
    def _parse_simple_enter_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool,
        struct_type: int,
        kercall_num: int,
        kercall_name: str,
        key_prefix: str,
        enter_class
    ) -> Optional[KerCallEnterEvent]:
        """解析简单的 ENTER Wide mode"""
        key = f"{key_prefix}{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffers[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'is_64bit': is_64bit,
                'data1': raw.data[1],
                'data2': raw.data[2],
                'extra_data': [],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, key_prefix)
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, key_prefix)
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
                
                event = enter_class(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    kercall_num=kercall_num,
                    kercall_name=kercall_name,
                    is_64bit=buf['is_64bit'],
                    is_wide=True,
                    data1=buf['data1'],
                    data2=buf['data2'],
                    extra_data={'wide_data': buf['extra_data']},
                )
                
                del self._combine_buffers[matching_key]
                return event
        
        return None
    
    def _parse_simple_exit_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool,
        struct_type: int,
        kercall_num: int,
        kercall_name: str,
        key_prefix: str,
        exit_class
    ) -> Optional[KerCallExitEvent]:
        """解析简单的 EXIT Wide mode"""
        key = f"{key_prefix}{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            ret_val = raw.data[1]
            self._combine_buffers[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'is_64bit': is_64bit,
                'ret_val': ret_val,
                'errno_val': raw.data[2] if ret_val == 0xFFFFFFFF else 0,
                'extra_data': [],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, key_prefix)
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, key_prefix)
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
                
                event = exit_class(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    kercall_num=kercall_num,
                    kercall_name=kercall_name,
                    is_64bit=buf['is_64bit'],
                    is_wide=True,
                    ret_val=buf['ret_val'],
                    errno_val=buf['errno_val'],
                    extra_data={'wide_data': buf['extra_data']},
                )
                
                del self._combine_buffers[matching_key]
                return event
        
        return None
    
    def _find_key(self, raw: RawEvent, prefix: str) -> Optional[str]:
        """查找匹配的组合事件 key"""
        search_prefix = f"{prefix}{raw.cpu_id}_"
        for k in self._combine_buffers.keys():
            if k.startswith(search_prefix):
                buf = self._combine_buffers[k]
                if buf.get('timestamp_cycles') == raw.data[0]:
                    return k
        return None
