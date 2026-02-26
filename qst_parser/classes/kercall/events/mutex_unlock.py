"""
QST Parser - Mutex Unlock 事件解析器

解析 __KER_SYNC_MUTEX_UNLOCK 内核调用事件。

参考: kercall_table_Events.html

ENTER (32-bit):
    Fast: sync_p, owner
    Wide: sync_p, count, owner

ENTER (64-bit, _NTO_TRACE_KERCALL64):
    Fast: sync_p (64), owner  <- 注意: 包含 owner!
    Wide: sync_p (64), count, owner
    
    64-bit Wide 模式数据布局:
        COMBINE_BEGIN: timestamp, sync_p_low, sync_p_high
        COMBINE_END:   timestamp, count, owner

EXIT (32/64-bit):
    Fast: ret_val, empty
    Wide: ret_val, empty
"""

from typing import Optional, Dict, Any
from ....models.base import RawEvent
from ....constants import StructType
from ..models import MutexUnlockEnterEvent, MutexUnlockExitEvent
from ..constants import KernelCall


class MutexUnlockEventParser:
    """Mutex Unlock 事件解析器"""
    
    KERCALL_NUM = KernelCall.SYNC_MUTEX_UNLOCK
    KERCALL_NAME = "SYNC_MUTEX_UNLOCK"
    
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
    ) -> Optional[MutexUnlockEnterEvent]:
        """
        解析 __KER_SYNC_MUTEX_UNLOCK ENTER 事件
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            # Fast 模式
            if is_64bit:
                # 64-bit Fast: sync_p (64), owner
                # 注意: MUTEX_UNLOCK 的 64-bit Fast 包含 owner!
                # data[1], data[2] = sync_p (64-bit)
                # 但 SIMPLE 只有 3 个数据槽位，owner 可能在 data[3] (如果有的话)
                sync_ptr = raw.data[1] | (raw.data[2] << 32)
                # 检查是否有更多数据 (owner 可能在扩展数据中)
                owner = 0
                return MutexUnlockEnterEvent(
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
                    owner=owner,
                )
            else:
                # 32-bit Fast: sync_p, owner
                return MutexUnlockEnterEvent(
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
                    owner=raw.data[2],
                )
        
        # Wide 模式组合事件
        return self._parse_enter_wide(raw, timestamp_ns, is_64bit, struct_type)
    
    def _parse_enter_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool,
        struct_type: int
    ) -> Optional[MutexUnlockEnterEvent]:
        """解析 ENTER Wide mode 组合事件"""
        key = f"mutex_unlock_enter_{raw.cpu_id}_{raw.data[0]}"
        
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
                # 32-bit Wide BEGIN: timestamp, sync_p, count
                self._enter_buffers[key] = {
                    'timestamp_cycles': raw.data[0],
                    'timestamp_ns': timestamp_ns,
                    'cpu_id': raw.cpu_id,
                    'is_64bit': False,
                    'sync_ptr': raw.data[1],
                    'count': raw.data[2],
                    'wide_data': [],
                }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, self._enter_buffers, "mutex_unlock_enter_")
            if matching_key:
                self._enter_buffers[matching_key]['wide_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, self._enter_buffers, "mutex_unlock_enter_")
            if matching_key:
                buf = self._enter_buffers[matching_key]
                buf['wide_data'].extend([raw.data[1], raw.data[2]])
                
                wide_data = buf['wide_data']
                
                if buf['is_64bit']:
                    # 64-bit Wide: sync_p (64), count, owner
                    count = wide_data[0] if len(wide_data) > 0 else 0
                    owner = wide_data[1] if len(wide_data) > 1 else 0
                    event = MutexUnlockEnterEvent(
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
                        count=count,
                        owner=owner,
                    )
                else:
                    # 32-bit Wide: sync_p, count, owner
                    owner = wide_data[0] if len(wide_data) > 0 else 0
                    event = MutexUnlockEnterEvent(
                        timestamp_cycles=buf['timestamp_cycles'],
                        timestamp_ns=buf['timestamp_ns'],
                        cpu_id=buf['cpu_id'],
                        kercall_num=self.KERCALL_NUM,
                        kercall_name=self.KERCALL_NAME,
                        # is_64bit=False,
                        # is_wide=True,
                        # data1=buf['sync_ptr'],
                        # data2=buf.get('count', 0),
                        sync_ptr=buf['sync_ptr'],
                        count=buf.get('count', 0),
                        owner=owner,
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
    ) -> Optional[MutexUnlockExitEvent]:
        """
        解析 __KER_SYNC_MUTEX_UNLOCK EXIT 事件
        
        EXIT 事件格式 (32/64位相同):
            Fast/Wide: ret_val, empty
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            ret_val = raw.data[1]
            errno_val = raw.data[2] if ret_val == 0xFFFFFFFF else 0
            
            return MutexUnlockExitEvent(
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
    ) -> Optional[MutexUnlockExitEvent]:
        """解析 EXIT Wide mode 组合事件"""
        key = f"mutex_unlock_exit_{raw.cpu_id}_{raw.data[0]}"
        
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
            matching_key = self._find_key(raw, self._exit_buffers, "mutex_unlock_exit_")
            if matching_key:
                buf = self._exit_buffers[matching_key]
                
                event = MutexUnlockExitEvent(
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
