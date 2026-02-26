"""
QST Parser - 通用内核调用事件解析器

解析所有内核调用事件的通用数据。
支持 ENTER/EXIT/INT 三种事件类型。
"""

from typing import Optional
from ....models.base import RawEvent
from ....constants import StructType
from ..models import (
    KerCallEvent, 
    KerCallEnterEvent, 
    KerCallExitEvent, 
    KerCallIntEvent,
)
from ..constants import KernelCall, KERCALL_64


class GenericKerCallParser:
    """
    通用内核调用解析器
    
    解析所有内核调用的基本数据，支持:
    - ENTER: 进入内核调用
    - EXIT: 退出内核调用
    - INT: 内核调用被中断
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._combine_buffer_enter = {}
        self._combine_buffer_exit = {}
        self._combine_buffer_int = {}
    
    def _get_kercall_name(self, kercall_num: int) -> str:
        """获取内核调用名称"""
        try:
            return KernelCall(kercall_num & 0x7F).name
        except ValueError:
            return f"KER_{kercall_num & 0x7F}"
    
    # ========================================================================
    # ENTER 事件解析
    # ========================================================================
    
    def parse_enter(
        self,
        raw: RawEvent,
        kercall_num: int,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[KerCallEnterEvent]:
        """
        解析 KERCALLENTER 事件
        
        Args:
            raw: 原始事件数据
            kercall_num: 内核调用编号 (0-107)
            timestamp_ns: 已计算的时间戳
            is_64bit: 是否为 64 位版本
        
        Returns:
            KerCallEnterEvent 或 None
        """
        struct_type = raw.struct_type
        kercall_name = self._get_kercall_name(kercall_num)
        
        if struct_type == StructType.SIMPLE:
            # Fast mode
            return KerCallEnterEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=kercall_num,
                kercall_name=kercall_name,
                is_64bit=is_64bit,
                is_wide=False,
                data1=raw.data[1],
                data2=raw.data[2],
            )
        
        # Wide mode
        return self._parse_enter_wide(
            raw, kercall_num, kercall_name, is_64bit, timestamp_ns, struct_type
        )
    
    def _parse_enter_wide(
        self,
        raw: RawEvent,
        kercall_num: int,
        kercall_name: str,
        is_64bit: bool,
        timestamp_ns: int,
        struct_type: int
    ) -> Optional[KerCallEnterEvent]:
        """解析 ENTER Wide mode 组合事件"""
        key = f"enter_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffer_enter[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'kercall_num': kercall_num,
                'kercall_name': kercall_name,
                'is_64bit': is_64bit,
                'data1': raw.data[1],
                'data2': raw.data[2],
                'extra_data': [],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, self._combine_buffer_enter, "enter_")
            if matching_key:
                buf = self._combine_buffer_enter[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, self._combine_buffer_enter, "enter_")
            if matching_key:
                buf = self._combine_buffer_enter[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
                
                event = KerCallEnterEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    kercall_num=buf['kercall_num'],
                    kercall_name=buf['kercall_name'],
                    is_64bit=buf['is_64bit'],
                    is_wide=True,
                    data1=buf['data1'],
                    data2=buf['data2'],
                    extra_data={'wide_data': buf['extra_data']},
                )
                
                del self._combine_buffer_enter[matching_key]
                return event
        
        return None
    
    # ========================================================================
    # EXIT 事件解析
    # ========================================================================
    
    def parse_exit(
        self,
        raw: RawEvent,
        kercall_num: int,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[KerCallExitEvent]:
        """
        解析 KERCALLEXIT 事件
        
        Args:
            raw: 原始事件数据
            kercall_num: 内核调用编号 (0-107)
            timestamp_ns: 已计算的时间戳
            is_64bit: 是否为 64 位版本
        
        Returns:
            KerCallExitEvent 或 None
        """
        struct_type = raw.struct_type
        kercall_name = self._get_kercall_name(kercall_num)
        
        if struct_type == StructType.SIMPLE:
            # Fast mode
            # 注意: EXIT 事件第一个数据是 ret_val
            ret_val = raw.data[1]
            # 如果 ret_val == -1，第二个数据是 errno
            errno_val = raw.data[2] if ret_val == 0xFFFFFFFF else 0
            
            return KerCallExitEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=kercall_num,
                kercall_name=kercall_name,
                is_64bit=is_64bit,
                is_wide=False,
                ret_val=ret_val,
                errno_val=errno_val,
            )
        
        # Wide mode
        return self._parse_exit_wide(
            raw, kercall_num, kercall_name, is_64bit, timestamp_ns, struct_type
        )
    
    def _parse_exit_wide(
        self,
        raw: RawEvent,
        kercall_num: int,
        kercall_name: str,
        is_64bit: bool,
        timestamp_ns: int,
        struct_type: int
    ) -> Optional[KerCallExitEvent]:
        """解析 EXIT Wide mode 组合事件"""
        key = f"exit_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            ret_val = raw.data[1]
            errno_val = raw.data[2] if ret_val == 0xFFFFFFFF else 0
            
            self._combine_buffer_exit[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'kercall_num': kercall_num,
                'kercall_name': kercall_name,
                'is_64bit': is_64bit,
                'ret_val': ret_val,
                'errno_val': errno_val,
                'extra_data': [],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, self._combine_buffer_exit, "exit_")
            if matching_key:
                buf = self._combine_buffer_exit[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, self._combine_buffer_exit, "exit_")
            if matching_key:
                buf = self._combine_buffer_exit[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
                
                event = KerCallExitEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    kercall_num=buf['kercall_num'],
                    kercall_name=buf['kercall_name'],
                    is_64bit=buf['is_64bit'],
                    is_wide=True,
                    ret_val=buf['ret_val'],
                    errno_val=buf['errno_val'],
                    extra_data={'wide_data': buf['extra_data']},
                )
                
                del self._combine_buffer_exit[matching_key]
                return event
        
        return None
    
    # ========================================================================
    # INT 事件解析
    # ========================================================================
    
    def parse_int(
        self,
        raw: RawEvent,
        kercall_num: int,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[KerCallIntEvent]:
        """
        解析 KERCALLINT 事件（内核调用被中断）
        
        Args:
            raw: 原始事件数据
            kercall_num: 内核调用编号 (0-107)
            timestamp_ns: 已计算的时间戳
            is_64bit: 是否为 64 位版本
        
        Returns:
            KerCallIntEvent 或 None
        """
        struct_type = raw.struct_type
        kercall_name = self._get_kercall_name(kercall_num)
        
        if struct_type == StructType.SIMPLE:
            # Fast mode
            return KerCallIntEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=kercall_num,
                kercall_name=kercall_name,
                is_64bit=is_64bit,
                is_wide=False,
                data1=raw.data[1],
                data2=raw.data[2],
            )
        
        # Wide mode
        return self._parse_int_wide(
            raw, kercall_num, kercall_name, is_64bit, timestamp_ns, struct_type
        )
    
    def _parse_int_wide(
        self,
        raw: RawEvent,
        kercall_num: int,
        kercall_name: str,
        is_64bit: bool,
        timestamp_ns: int,
        struct_type: int
    ) -> Optional[KerCallIntEvent]:
        """解析 INT Wide mode 组合事件"""
        key = f"int_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffer_int[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'kercall_num': kercall_num,
                'kercall_name': kercall_name,
                'is_64bit': is_64bit,
                'data1': raw.data[1],
                'data2': raw.data[2],
                'extra_data': [],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, self._combine_buffer_int, "int_")
            if matching_key:
                buf = self._combine_buffer_int[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, self._combine_buffer_int, "int_")
            if matching_key:
                buf = self._combine_buffer_int[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
                
                event = KerCallIntEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    kercall_num=buf['kercall_num'],
                    kercall_name=buf['kercall_name'],
                    is_64bit=buf['is_64bit'],
                    is_wide=True,
                    data1=buf['data1'],
                    data2=buf['data2'],
                    extra_data={'wide_data': buf['extra_data']},
                )
                
                del self._combine_buffer_int[matching_key]
                return event
        
        return None
    
    # ========================================================================
    # 兼容旧接口
    # ========================================================================
    
    def parse(
        self, 
        raw: RawEvent, 
        kercall_num: int,
        timestamp_ns: int = 0
    ) -> Optional[KerCallEvent]:
        """
        解析内核调用事件（兼容旧接口）
        
        Args:
            raw: 原始事件数据
            kercall_num: 内核调用编号
            timestamp_ns: 已计算的时间戳
        """
        struct_type = raw.struct_type
        
        # 检查是否为 64 位调用
        is_64bit = (kercall_num & KERCALL_64) != 0
        base_kercall = kercall_num & 0x7F
        kercall_name = self._get_kercall_name(base_kercall)
        
        if struct_type == StructType.SIMPLE:
            return KerCallEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=kercall_num,
                kercall_name=kercall_name,
                is_64bit=is_64bit,
                data1=raw.data[1],
                data2=raw.data[2],
            )
        
        # Wide mode - 使用 enter 的缓冲区（兼容）
        return self._parse_legacy_wide(
            raw, kercall_num, kercall_name, is_64bit, 
            timestamp_ns, struct_type
        )
    
    def _parse_legacy_wide(
        self,
        raw: RawEvent,
        kercall_num: int,
        kercall_name: str,
        is_64bit: bool,
        timestamp_ns: int,
        struct_type: int
    ) -> Optional[KerCallEvent]:
        """解析 Wide mode 组合事件（兼容旧接口）"""
        key = f"legacy_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffer_enter[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'kercall_num': kercall_num,
                'kercall_name': kercall_name,
                'is_64bit': is_64bit,
                'data1': raw.data[1],
                'data2': raw.data[2],
                'extra_data': [],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, self._combine_buffer_enter, "legacy_")
            if matching_key:
                buf = self._combine_buffer_enter[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, self._combine_buffer_enter, "legacy_")
            if matching_key:
                buf = self._combine_buffer_enter[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
                
                event = KerCallEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    kercall_num=buf['kercall_num'],
                    kercall_name=buf['kercall_name'],
                    is_64bit=buf['is_64bit'],
                    data1=buf['data1'],
                    data2=buf['data2'],
                    extra_data={'wide_data': buf['extra_data']},
                )
                
                del self._combine_buffer_enter[matching_key]
                return event
        
        return None
    
    # ========================================================================
    # 辅助方法
    # ========================================================================
    
    def _find_key(self, raw: RawEvent, buffer: dict, prefix: str) -> Optional[str]:
        """查找匹配时间戳的组合事件 key"""
        search_prefix = f"{prefix}{raw.cpu_id}_"
        for k in buffer.keys():
            if k.startswith(search_prefix):
                buf = buffer[k]
                if buf.get('timestamp_cycles') == raw.data[0]:
                    return k
        return None
