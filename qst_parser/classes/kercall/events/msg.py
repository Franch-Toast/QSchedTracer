"""
QST Parser - 消息传递事件解析器

解析 MSG_SENDV, MSG_RECEIVEV, MSG_REPLYV 相关的内核调用事件。

参考: kercall_table_Events.html
"""

from typing import Optional, Dict, Any
from ....models.base import RawEvent
from ....constants import StructType
from ..models import (
    MsgSendvEnterEvent, MsgSendvExitEvent,
    MsgReceivevEnterEvent, MsgReceivevExitEvent,
    MsgReplyvEnterEvent, MsgReplyvExitEvent,
    KerCallEnterEvent, KerCallExitEvent,
)
from ..constants import KernelCall


class MsgEventParser:
    """
    消息传递事件解析器
    
    支持:
    - MSG_SENDV / MSG_SENDVNC
    - MSG_RECEIVEV / MSG_RECEIVEPULSEV
    - MSG_REPLYV
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self._combine_buffers: Dict[str, Dict[str, Any]] = {}
    
    # ========================================================================
    # MSG_SENDV
    # ========================================================================
    
    def parse_msg_sendv_enter(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[MsgSendvEnterEvent]:
        """
        解析 __KER_MSG_SENDV ENTER 事件
        
        Fast mode: coid, msg[0]
        Wide mode: coid, sparts, rparts, msg[0], msg[1], msg[2]
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            return MsgSendvEnterEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=KernelCall.MSG_SENDV,
                kercall_name="MSG_SENDV",
                # is_64bit=is_64bit,
                # is_wide=False,
                # data1=raw.data[1],
                # data2=raw.data[2],
                coid=raw.data[1],
                msg=(raw.data[2], 0, 0),
            )
        
        return self._parse_sendv_enter_wide(raw, timestamp_ns, is_64bit, struct_type)
    
    def _parse_sendv_enter_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool,
        struct_type: int
    ) -> Optional[MsgSendvEnterEvent]:
        """解析 MSG_SENDV ENTER Wide mode"""
        key = f"sendv_enter_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffers[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'is_64bit': is_64bit,
                'coid': raw.data[1],
                'sparts': raw.data[2],
                'extra_data': [],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, "sendv_enter_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, "sendv_enter_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
                
                # Wide: coid, sparts, rparts, msg[0], msg[1], msg[2]
                extra = buf['extra_data']
                rparts = extra[0] if len(extra) > 0 else 0
                msg0 = extra[1] if len(extra) > 1 else 0
                msg1 = extra[2] if len(extra) > 2 else 0
                msg2 = extra[3] if len(extra) > 3 else 0
                
                event = MsgSendvEnterEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    kercall_num=KernelCall.MSG_SENDV,
                    kercall_name="MSG_SENDV",
                    # is_64bit=buf['is_64bit'],
                    # is_wide=True,
                    # data1=buf['coid'],
                    # data2=buf['sparts'],
                    coid=buf['coid'],
                    sparts=buf['sparts'],
                    rparts=rparts,
                    msg=(msg0, msg1, msg2),
                    extra_data={'wide_data': extra},
                )
                
                del self._combine_buffers[matching_key]
                return event
        
        return None
    
    def parse_msg_sendv_exit(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[MsgSendvExitEvent]:
        """
        解析 __KER_MSG_SENDV EXIT 事件
        
        Fast mode: status, rmsg[0]
        Wide mode: status, rmsg[0], rmsg[1], rmsg[2]
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            ret_val = raw.data[1]
            errno_val = raw.data[2] if ret_val == 0xFFFFFFFF else 0
            
            return MsgSendvExitEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=KernelCall.MSG_SENDV,
                kercall_name="MSG_SENDV",
                # is_64bit=is_64bit,
                # is_wide=False,
                ret_val=ret_val,
                errno_val=errno_val,
                rmsg=(raw.data[2], 0, 0) if ret_val != 0xFFFFFFFF else (0, 0, 0),
            )
        
        return self._parse_sendv_exit_wide(raw, timestamp_ns, is_64bit, struct_type)
    
    def _parse_sendv_exit_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool,
        struct_type: int
    ) -> Optional[MsgSendvExitEvent]:
        """解析 MSG_SENDV EXIT Wide mode"""
        key = f"sendv_exit_{raw.cpu_id}_{raw.data[0]}"
        
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
            matching_key = self._find_key(raw, "sendv_exit_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, "sendv_exit_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
                
                extra = buf['extra_data']
                rmsg0 = extra[0] if len(extra) > 0 else 0
                rmsg1 = extra[1] if len(extra) > 1 else 0
                rmsg2 = extra[2] if len(extra) > 2 else 0
                
                event = MsgSendvExitEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    kercall_num=KernelCall.MSG_SENDV,
                    kercall_name="MSG_SENDV",
                    # is_64bit=buf['is_64bit'],
                    # is_wide=True,
                    ret_val=buf['ret_val'],
                    errno_val=buf['errno_val'],
                    rmsg=(rmsg0, rmsg1, rmsg2),
                    extra_data={'wide_data': extra},
                )
                
                del self._combine_buffers[matching_key]
                return event
        
        return None
    
    # ========================================================================
    # MSG_RECEIVEV
    # ========================================================================
    
    def parse_msg_receivev_enter(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[MsgReceivevEnterEvent]:
        """
        解析 __KER_MSG_RECEIVEV ENTER 事件
        
        Fast mode: chid, rparts
        Wide mode: chid, rparts
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            return MsgReceivevEnterEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=KernelCall.MSG_RECEIVEV,
                kercall_name="MSG_RECEIVEV",
                # is_64bit=is_64bit,
                # is_wide=False,
                # data1=raw.data[1],
                # data2=raw.data[2],
                chid=raw.data[1],
                rparts=raw.data[2],
            )
        
        # Wide mode 也只有 chid, rparts
        return self._parse_simple_enter_wide(
            raw, timestamp_ns, is_64bit, struct_type,
            KernelCall.MSG_RECEIVEV, "MSG_RECEIVEV", "receivev_enter_",
            MsgReceivevEnterEvent,
            lambda buf, extra: MsgReceivevEnterEvent(
                timestamp_cycles=buf['timestamp_cycles'],
                timestamp_ns=buf['timestamp_ns'],
                cpu_id=buf['cpu_id'],
                kercall_num=KernelCall.MSG_RECEIVEV,
                kercall_name="MSG_RECEIVEV",
                # is_64bit=buf['is_64bit'],
                # is_wide=True,
                # data1=buf['data1'],
                # data2=buf['data2'],
                chid=buf['data1'],
                rparts=buf['data2'],
                extra_data={'wide_data': extra},
            )
        )
    
    def parse_msg_receivev_exit(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[MsgReceivevExitEvent]:
        """
        解析 __KER_MSG_RECEIVEV EXIT 事件
        
        Fast mode: rcvid, rmsg[0]
        Wide mode: rcvid, rmsg[0-2], info->*
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            return MsgReceivevExitEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=KernelCall.MSG_RECEIVEV,
                kercall_name="MSG_RECEIVEV",
                # is_64bit=is_64bit,
                # is_wide=False,
                ret_val=raw.data[1],
                errno_val=raw.data[2] if raw.data[1] == 0xFFFFFFFF else 0,
                rcvid=raw.data[1],
                rmsg=(raw.data[2], 0, 0),
            )
        
        return self._parse_receivev_exit_wide(raw, timestamp_ns, is_64bit, struct_type)
    
    def _parse_receivev_exit_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool,
        struct_type: int
    ) -> Optional[MsgReceivevExitEvent]:
        """解析 MSG_RECEIVEV EXIT Wide mode"""
        key = f"receivev_exit_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffers[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'is_64bit': is_64bit,
                'rcvid': raw.data[1],
                'rmsg0': raw.data[2],
                'extra_data': [],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, "receivev_exit_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, "receivev_exit_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
                
                # Wide: rcvid, rmsg[0-2], info->nd, srcnd, pid, tid, chid, scoid,
                #       coid, msglen, srcmsglen, dstmsglen, priority, flags, reserved
                extra = buf['extra_data']
                
                event = MsgReceivevExitEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    kercall_num=KernelCall.MSG_RECEIVEV,
                    kercall_name="MSG_RECEIVEV",
                    # is_64bit=buf['is_64bit'],
                    # is_wide=True,
                    ret_val=buf['rcvid'],
                    rcvid=buf['rcvid'],
                    rmsg=(buf['rmsg0'],
                          extra[0] if len(extra) > 0 else 0,
                          extra[1] if len(extra) > 1 else 0),
                    info_nd=extra[2] if len(extra) > 2 else 0,
                    info_srcnd=extra[3] if len(extra) > 3 else 0,
                    info_pid=extra[4] if len(extra) > 4 else 0,
                    info_tid=extra[5] if len(extra) > 5 else 0,
                    info_chid=extra[6] if len(extra) > 6 else 0,
                    info_scoid=extra[7] if len(extra) > 7 else 0,
                    info_coid=extra[8] if len(extra) > 8 else 0,
                    info_msglen=extra[9] if len(extra) > 9 else 0,
                    info_srcmsglen=extra[10] if len(extra) > 10 else 0,
                    info_dstmsglen=extra[11] if len(extra) > 11 else 0,
                    info_priority=extra[12] if len(extra) > 12 else 0,
                    info_flags=extra[13] if len(extra) > 13 else 0,
                    extra_data={'wide_data': extra},
                )
                
                del self._combine_buffers[matching_key]
                return event
        
        return None
    
    # ========================================================================
    # MSG_REPLYV
    # ========================================================================
    
    def parse_msg_replyv_enter(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[MsgReplyvEnterEvent]:
        """
        解析 __KER_MSG_REPLYV ENTER 事件
        
        Fast mode: rcvid, status
        Wide mode: rcvid, sparts, status, smsg[0], smsg[1], smsg[2]
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            return MsgReplyvEnterEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=KernelCall.MSG_REPLYV,
                kercall_name="MSG_REPLYV",
                # is_64bit=is_64bit,
                # is_wide=False,
                # data1=raw.data[1],
                # data2=raw.data[2],
                rcvid=raw.data[1],
                status=raw.data[2],
            )
        
        return self._parse_replyv_enter_wide(raw, timestamp_ns, is_64bit, struct_type)
    
    def _parse_replyv_enter_wide(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool,
        struct_type: int
    ) -> Optional[MsgReplyvEnterEvent]:
        """解析 MSG_REPLYV ENTER Wide mode"""
        key = f"replyv_enter_{raw.cpu_id}_{raw.data[0]}"
        
        if struct_type == StructType.COMBINE_BEGIN:
            self._combine_buffers[key] = {
                'timestamp_cycles': raw.data[0],
                'timestamp_ns': timestamp_ns,
                'cpu_id': raw.cpu_id,
                'is_64bit': is_64bit,
                'rcvid': raw.data[1],
                'sparts': raw.data[2],
                'extra_data': [],
            }
            return None
        
        elif struct_type == StructType.COMBINE_CONT:
            matching_key = self._find_key(raw, "replyv_enter_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
            return None
        
        elif struct_type == StructType.COMBINE_END:
            matching_key = self._find_key(raw, "replyv_enter_")
            if matching_key:
                buf = self._combine_buffers[matching_key]
                buf['extra_data'].extend([raw.data[1], raw.data[2]])
                
                # Wide: rcvid, sparts, status, smsg[0], smsg[1], smsg[2]
                extra = buf['extra_data']
                status = extra[0] if len(extra) > 0 else 0
                smsg0 = extra[1] if len(extra) > 1 else 0
                smsg1 = extra[2] if len(extra) > 2 else 0
                smsg2 = extra[3] if len(extra) > 3 else 0
                
                event = MsgReplyvEnterEvent(
                    timestamp_cycles=buf['timestamp_cycles'],
                    timestamp_ns=buf['timestamp_ns'],
                    cpu_id=buf['cpu_id'],
                    kercall_num=KernelCall.MSG_REPLYV,
                    kercall_name="MSG_REPLYV",
                    # is_64bit=buf['is_64bit'],
                    # is_wide=True,
                    # data1=buf['rcvid'],
                    # data2=buf['sparts'],
                    rcvid=buf['rcvid'],
                    sparts=buf['sparts'],
                    status=status,
                    smsg=(smsg0, smsg1, smsg2),
                    extra_data={'wide_data': extra},
                )
                
                del self._combine_buffers[matching_key]
                return event
        
        return None
    
    def parse_msg_replyv_exit(
        self,
        raw: RawEvent,
        timestamp_ns: int,
        is_64bit: bool = False
    ) -> Optional[MsgReplyvExitEvent]:
        """
        解析 __KER_MSG_REPLYV EXIT 事件
        
        Fast/Wide mode: ret_val, empty
        """
        struct_type = raw.struct_type
        
        if struct_type == StructType.SIMPLE:
            ret_val = raw.data[1]
            errno_val = raw.data[2] if ret_val == 0xFFFFFFFF else 0
            
            return MsgReplyvExitEvent(
                timestamp_cycles=raw.data[0],
                timestamp_ns=timestamp_ns,
                cpu_id=raw.cpu_id,
                kercall_num=KernelCall.MSG_REPLYV,
                kercall_name="MSG_REPLYV",
                # is_64bit=is_64bit,
                # is_wide=False,
                ret_val=ret_val,
                errno_val=errno_val,
            )
        
        return self._parse_simple_exit_wide(
            raw, timestamp_ns, is_64bit, struct_type,
            KernelCall.MSG_REPLYV, "MSG_REPLYV", "replyv_exit_",
            MsgReplyvExitEvent
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
        event_class,
        create_func
    ) -> Optional[KerCallEnterEvent]:
        """解析简单的 ENTER Wide mode（只有 data1, data2）"""
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
                event = create_func(buf, buf['extra_data'])
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
        """解析简单的 EXIT Wide mode（只有 ret_val）"""
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
                    # is_64bit=buf['is_64bit'],
                    # is_wide=True,
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
