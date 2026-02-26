"""
QST Parser - Condvar Wait 事件解析器

解析 __KER_SYNC_CONDVAR_WAIT 内核调用事件。

参考: kercall_table_Events.html

ENTER (32-bit):
    Fast: sync_p, mutex_p
    Wide: sync_p, mutex_p, sync->count, sync->owner, mutex->count, mutex->owner

ENTER (64-bit, _NTO_TRACE_KERCALL64):
    Fast: sync_p (64)
    Wide: sync_p (64), mutex_p (64), sync->count, sync->owner, mutex->count, mutex->owner
    
    64-bit Wide 模式数据布局:
        COMBINE_BEGIN: timestamp, sync_p_low, sync_p_high
        COMBINE_CONT:  timestamp, mutex_p_low, mutex_p_high
        COMBINE_CONT:  timestamp, sync->count, sync->owner
        COMBINE_END:   timestamp, mutex->count, mutex->owner

EXIT (32/64-bit):
    Fast: ret_val, empty
    Wide: ret_val, empty
"""

from typing import Optional, Dict, Any
from ....models.base import RawEvent
from ....constants import StructType
from ..models import CondvarWaitEnterEvent, CondvarWaitExitEvent
from ..constants import KernelCall


class CondvarWaitEventParser:
    """Condvar Wait 事件解析器"""
    
    KERCALL_NUM = KernelCall.SYNC_CONDVAR_WAIT
    KERCALL_NAME = "SYNC_CONDVAR_WAIT"
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._enter_buffers: Dict[str, Dict[str, Any]] = {}
        self._exit_buffers: Dict[str, Dict[str, Any]] = {}
    
    # ========================================================================
    # ENTER 事件解析
    # ========================================================================
    
    def parse_enter(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[CondvarWaitEnterEvent]:
        """
        解析 __KER_SYNC_CONDVAR_WAIT ENTER 事件
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            # Fast 模式
            if is_64bit:
                # 64-bit Fast: sync_p (64)
                sync_ptr = raw.data[1] | (raw.data[2] << 32)
                return CondvarWaitEnterEvent(
                    timestamp_cycles=raw.data[0],
                    timestamp_ns=timestamp_ns,
                    cpu_id=raw.cpu_id,
                    kercall_num=self.KERCALL_NUM,
                    kercall_name=self.KERCALL_NAME,
                    # is_64bit=True,
                    # is_wide=False,
                    # data1=raw.data[1],
                    # data2=raw.data[2],
                    sync_ptr=sync_ptr,
                    mutex_ptr=0,
                )
            else:
                # 32-bit Fast: sync_p, mutex_p
                return CondvarWaitEnterEvent(
                    timestamp_cycles=raw.data[0],
                    timestamp_ns=timestamp_ns,
                    cpu_id=raw.cpu_id,
                    kercall_num=self.KERCALL_NUM,
                    kercall_name=self.KERCALL_NAME,
                    # is_64bit=False,
                    # is_wide=False,
                    # data1=raw.data[1],
                    # data2=raw.data[2],
                    sync_ptr=raw.data[1],
                    mutex_ptr=raw.data[2],
                )
        
        # Wide 模式组合事件
        return self._parse_enter_wide(raw, timestamp_ns, is_64bit, struct_type)
    
    def _parse_enter_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool,
        struct_type: int
    ) -> Optional[CondvarWaitEnterEvent]:
        """解析 ENTER Wide mode 组合事件"""
        key = f"condvar_wait_enter_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            if is_64bit:
                # 64-bit Wide BEGIN: timestamp, sync_p_low, sync_p_high
                sync_ptr = raw.data[1] | (raw.data[2] << 32)
                self._enter_buffers[key] = {
                    'timestamp_cycles': raw.data[0],
                    'timestamp_ns': timestamp_ns,
                    'cpu_id': raw.cpu_id,
                    'is_64bit': True,
                    'sync_ptr': sync_ptr,
                    'wide_data': [],
                }
            else:
                # 32-bit Wide BEGIN: timestamp, sync_p, mutex_p
                self._enter_buffers[key] = {
                    'timestamp_cycles': raw.data[0],
                    'timestamp_ns': timestamp_ns,
                    'cpu_id': raw.cpu_id,
                    'is_64bit': False,
                    'sync_ptr': raw.data[1],
                    'mutex_ptr': raw.data[2],
                    'wide_data': [],
                }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, self._enter_buffers, "condvar_wait_enter_")
            if matching_key:
                self._enter_buffers[matching_key]['wide_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, self._enter_buffers, "condvar_wait_enter_")
            if matching_key:
                buf = self._enter_buffers[matching_key]
                buf['wide_data'].extend([raw.data[1], raw.data[2]])
                
                wide_data = buf['wide_data']
                
                if buf['is_64bit']:
                    # 64-bit Wide: sync_p (64), mutex_p (64), sync->count, sync->owner, mutex->count, mutex->owner
                    # wide_data: mutex_p_low, mutex_p_high, sync->count, sync->owner, mutex->count, mutex->owner
                    mutex_ptr = 0
                    if len(wide_data) >= 2:
                        mutex_ptr = wide_data[0] | (wide_data[1] << 32)
                    sync_count = wide_data[2] if len(wide_data) > 2 else 0
                    sync_owner = wide_data[3] if len(wide_data) > 3 else 0
                    mutex_count = wide_data[4] if len(wide_data) > 4 else 0
                    mutex_owner = wide_data[5] if len(wide_data) > 5 else 0
                    
                    event = CondvarWaitEnterEvent(
                        timestamp_cycles=buf['timestamp_cycles'],
                        timestamp_ns=buf['timestamp_ns'],
                        cpu_id=buf['cpu_id'],
                        kercall_num=self.KERCALL_NUM,
                        kercall_name=self.KERCALL_NAME,
                        # is_64bit=True,
                        # is_wide=True,
                        # data1=buf['sync_ptr'] & 0xFFFFFFFF,
                        # data2=(buf['sync_ptr'] >> 32) & 0xFFFFFFFF,
                        sync_ptr=buf['sync_ptr'],
                        mutex_ptr=mutex_ptr,
                        sync_count=sync_count,
                        sync_owner=sync_owner,
                        mutex_count=mutex_count,
                        mutex_owner=mutex_owner,
                    )
                else:
                    # 32-bit Wide: sync_p, mutex_p, sync->count, sync->owner, mutex->count, mutex->owner
                    # wide_data: sync->count, sync->owner, mutex->count, mutex->owner
                    sync_count = wide_data[0] if len(wide_data) > 0 else 0
                    sync_owner = wide_data[1] if len(wide_data) > 1 else 0
                    mutex_count = wide_data[2] if len(wide_data) > 2 else 0
                    mutex_owner = wide_data[3] if len(wide_data) > 3 else 0
                    
                    event = CondvarWaitEnterEvent(
                        timestamp_cycles=buf['timestamp_cycles'],
                        timestamp_ns=buf['timestamp_ns'],
                        cpu_id=buf['cpu_id'],
                        kercall_num=self.KERCALL_NUM,
                        kercall_name=self.KERCALL_NAME,
                        # is_64bit=False,
                        # is_wide=True,
                        # data1=buf['sync_ptr'],
                        # data2=buf.get('mutex_ptr', 0),
                        sync_ptr=buf['sync_ptr'],
                        mutex_ptr=buf.get('mutex_ptr', 0),
                        sync_count=sync_count,
                        sync_owner=sync_owner,
                        mutex_count=mutex_count,
                        mutex_owner=mutex_owner,
                    )
                
                del self._enter_buffers[matching_key]
                return event
        
        return None
    
    # ========================================================================
    # EXIT 事件解析
    # ========================================================================
    
    def parse_exit(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[CondvarWaitExitEvent]:
        """解析 __KER_SYNC_CONDVAR_WAIT EXIT 事件"""
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            ret_val = raw.data[1]
            errno_val = raw.data[2] if ret_val == 0xFFFFFFFF else 0
            
            return CondvarWaitExitEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=self.KERCALL_NUM,
                kercall_name=self.KERCALL_NAME,
                # is_64bit=is_64bit,
                # is_wide=False,
                ret_val=ret_val,
                errno_val=errno_val,
            )
        
        # Wide mode 组合事件
        return self._parse_exit_wide(raw, timestamp_ns, is_64bit, struct_type)
    
    def _parse_exit_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool,
        struct_type: int
    ) -> Optional[CondvarWaitExitEvent]:
        """解析 EXIT Wide mode 组合事件"""
        key = f"condvar_wait_exit_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            ret_val = raw.data[1]
            errno_val = raw.data[2] if ret_val == 0xFFFFFFFF else 0
            
            self._exit_buffers[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'is_64bit': is_64bit,
                'ret_val': ret_val,
                'errno_val': errno_val,
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, self._exit_buffers, "condvar_wait_exit_")
            if matching_key:
                buf = self._exit_buffers[matching_key]
                
                event = CondvarWaitExitEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    kercall_num=self.KERCALL_NUM,
                    kercall_name=self.KERCALL_NAME,
                    # is_64bit=buf['is_64bit'],
                    # is_wide=True,
                    ret_val=buf['ret_val'],
                    errno_val=buf['errno_val'],
                )
                
                del self._exit_buffers[matching_key]
                return event
        
        return None
    
    def _find_key(self, raw: RawEvent, buffers: Dict, prefix: str) -> Optional[str]:
        """查找匹配的组合事件 key"""
        search_prefix = f"{prefix}{raw.cpu_id}_"
        for k in buffers.keys():
            if k.startswith(search_prefix):
                buf = buffers[k]
                if buf.get('timestamp_cycles') == raw.data[0]:
                    return k
        return None
