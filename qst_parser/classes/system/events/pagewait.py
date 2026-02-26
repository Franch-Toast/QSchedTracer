"""
QST Parser - PageWait (页面等待) 事件解析器

参考文档: kercall_table_Events.html

_NTO_TRACE_SYS_PAGEWAIT:
    Fast: pid (32), tid (32), ip (32), vaddr (32)
    Wide: pid (32), tid (32), ip (32), vaddr (32), fault_type (32), mmap_flags (32),
          object_offset (64), object_name (string)

Wide 模式下 object_name 是字符串，通过组合事件传递：
    COMBINE_BEGIN: timestamp, pid, tid
    COMBINE_CONT:  timestamp, ip, vaddr / fault_type, mmap_flags / offset_low, offset_high
    COMBINE_END:   timestamp, name_part1, name_part2 (可能多个 CONT 传递名称)
"""

from typing import Optional, Dict, Any
from ....models.base import RawEvent
from ....constants import StructType
from ..models import PageWaitEvent


class PageWaitEventParser:
    """PageWait 事件解析器"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._combine_buffers: Dict[str, Dict[str, Any]] = {}
    
    def parse(
        self,
        raw: RawEvent,
        timestamp_ns: int
    ) -> Optional[PageWaitEvent]:
        """
        解析 PageWait 事件
        
        Args:
            raw: 原始事件数据
            timestamp_ns: 已计算的时间戳
        
        Returns:
            PageWaitEvent 或 None（组合事件未完成时）
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            # Fast mode: pid, tid, ip, vaddr
            # 但 SIMPLE 只有 3 个数据槽位
            return PageWaitEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                pid=raw.data[1],
                tid=raw.data[2],
                ip=0,
                vaddr=0,
                is_wide=False,
            )
        
        # Wide mode 组合事件
        return self._parse_wide(raw, timestamp_ns, struct_type)
    
    def _parse_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        struct_type: int
    ) -> Optional[PageWaitEvent]:
        """解析 Wide mode 组合事件"""
        key = f"pagewait_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            # COMBINE_BEGIN: timestamp, pid, tid
            self._combine_buffers[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'pid': raw.data[1],
                'tid': raw.data[2],
                'data': [],
                'name_bytes': bytearray(),
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, "pagewait_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                data_len = len(buf['data'])
                
                if data_len < 6:
                    # 还在收集数值数据: ip, vaddr, fault_type, mmap_flags, offset_low, offset_high
                    buf['data'].extend([raw.data[1], raw.data[2]])
                else:
                    # 开始收集字符串名称
                    self._append_name_data(buf, raw.data[1], raw.data[2])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, "pagewait_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                
                # 添加最后的数据
                data_len = len(buf['data'])
                if data_len < 6:
                    buf['data'].extend([raw.data[1], raw.data[2]])
                else:
                    self._append_name_data(buf, raw.data[1], raw.data[2])
                
                data = buf['data']
                ip = data[0] if len(data) > 0 else 0
                vaddr = data[1] if len(data) > 1 else 0
                fault_type = data[2] if len(data) > 2 else 0
                mmap_flags = data[3] if len(data) > 3 else 0
                offset_low = data[4] if len(data) > 4 else 0
                offset_high = data[5] if len(data) > 5 else 0
                object_offset = offset_low | (offset_high << 32)
                
                # 解码对象名称
                name_bytes = buf['name_bytes']
                object_name = ""
                if name_bytes:
                    # 去除 null 终止符
                    null_pos = name_bytes.find(0)
                    if null_pos >= 0:
                        name_bytes = name_bytes[:null_pos]
                    try:
                        object_name = name_bytes.decode('utf-8', errors='replace')
                    except:
                        object_name = ""
                
                event = PageWaitEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    pid=buf['pid'],
                    tid=buf['tid'],
                    ip=ip,
                    vaddr=vaddr,
                    fault_type=fault_type,
                    mmap_flags=mmap_flags,
                    object_offset=object_offset,
                    object_name=object_name,
                    is_wide=True,
                )
                
                del self._combine_buffers[matching_key]
                return event
        
        return None
    
    def _append_name_data(self, buf: Dict[str, Any], data1: int, data2: int) -> None:
        """将数据追加到名称字节数组"""
        # 每个 32 位整数包含 4 个字节的名称数据
        buf['name_bytes'].extend(data1.to_bytes(4, 'little'))
        buf['name_bytes'].extend(data2.to_bytes(4, 'little'))
    
    def _find_key(self, raw: RawEvent, prefix: str) -> Optional[str]:
        """查找匹配的组合事件 key"""
        search_prefix = f"{prefix}{raw.cpu_id}_"
        for k in self._combine_buffers.keys():
            if k.startswith(search_prefix):
                buf = self._combine_buffers[k]
                if buf.get('timestamp_cycles') == raw.data[0]:
                    return k
        return None
