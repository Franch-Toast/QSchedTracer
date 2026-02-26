"""
QST Parser - Comm 类事件处理器

处理所有通信相关事件:
- COMM_SMSG / COMM_RMSG: 消息发送/接收
- COMM_REPLY / COMM_ERROR: 回复/错误
- COMM_SPULSE / COMM_RPULSE / COMM_SPULSE_*: 脉冲
- COMM_SIGNAL: 信号

参考: kercall_table_Events.html
"""

from typing import List, Optional, Any, Union
from ..base import BaseClassHandler
from ...models.base import RawEvent
from .models import (
    CommBaseEvent,
    CommMsgEvent, CommReplyEvent, CommPulseEvent, CommSignalEvent,
    CommEvent,  # 兼容旧代码
)
from .constants import CommEventType
from .events import (
    CommMsgEventParser, CommReplyEventParser, 
    CommPulseEventParser, CommSignalEventParser,
)


class CommClassHandler(BaseClassHandler):
    """
    Comm 类事件处理器
    
    处理所有通信相关事件。
    
    事件格式:
        - SMSG/RMSG: rcvid, pid
        - REPLY/ERROR: tid, pid  
        - SPULSE/RPULSE/...: scoid, pid
        - SIGNAL: si_signo, si_code (Fast) / si_signo, si_code, si_errno, __data.__pad[0-5] (Wide)
    """
    
    def __init__(self, verbose: bool = False):
        super().__init__(verbose)
        
        self._msg_parser = CommMsgEventParser(verbose)
        self._reply_parser = CommReplyEventParser(verbose)
        self._pulse_parser = CommPulseEventParser(verbose)
        self._signal_parser = CommSignalEventParser(verbose)
        
        # 分类存储事件
        self.msg_events: List[CommMsgEvent] = []
        self.reply_events: List[CommReplyEvent] = []
        self.pulse_events: List[CommPulseEvent] = []
        self.signal_events: List[CommSignalEvent] = []
        
        # 兼容旧代码
        self.comm_events: List[CommEvent] = []
    
    @property
    def class_name(self) -> str:
        return "COMM"
    
    def handle_event(
        self, 
        raw: RawEvent, 
        ext_event: int,
        timestamp_ns: int,
        **kwargs
    ) -> Optional[CommBaseEvent]:
        """
        处理通信事件
        
        Args:
            raw: 原始事件数据
            ext_event: 通信事件类型
            timestamp_ns: 已计算的 UTC 时间戳
        
        Returns:
            CommBaseEvent 的子类
        """
        event = None
        
        # 消息事件 (SMSG / RMSG)
        if ext_event == CommEventType.COMM_SMSG:
            event = self._msg_parser.parse(raw, timestamp_ns, "smsg")
            if event:
                self.msg_events.append(event)
        
        elif ext_event == CommEventType.COMM_RMSG:
            event = self._msg_parser.parse(raw, timestamp_ns, "rmsg")
            if event:
                self.msg_events.append(event)
        
        # 回复/错误事件 (REPLY / ERROR)
        elif ext_event == CommEventType.COMM_REPLY:
            event = self._reply_parser.parse(raw, timestamp_ns, "reply")
            if event:
                self.reply_events.append(event)
        
        elif ext_event == CommEventType.COMM_ERROR:
            event = self._reply_parser.parse(raw, timestamp_ns, "error")
            if event:
                self.reply_events.append(event)
        
        # 脉冲事件 (SPULSE / RPULSE / ...)
        elif ext_event == CommEventType.COMM_SPULSE:
            event = self._pulse_parser.parse(raw, timestamp_ns, "spulse")
            if event:
                self.pulse_events.append(event)
        
        elif ext_event == CommEventType.COMM_RPULSE:
            event = self._pulse_parser.parse(raw, timestamp_ns, "rpulse")
            if event:
                self.pulse_events.append(event)
        
        elif ext_event == CommEventType.COMM_SPULSE_EXE:
            event = self._pulse_parser.parse(raw, timestamp_ns, "spulse_exe")
            if event:
                self.pulse_events.append(event)
        
        elif ext_event == CommEventType.COMM_SPULSE_DIS:
            event = self._pulse_parser.parse(raw, timestamp_ns, "spulse_dis")
            if event:
                self.pulse_events.append(event)
        
        elif ext_event == CommEventType.COMM_SPULSE_DEA:
            event = self._pulse_parser.parse(raw, timestamp_ns, "spulse_dea")
            if event:
                self.pulse_events.append(event)
        
        elif ext_event == CommEventType.COMM_SPULSE_UN:
            event = self._pulse_parser.parse(raw, timestamp_ns, "spulse_un")
            if event:
                self.pulse_events.append(event)
        
        elif ext_event == CommEventType.COMM_SPULSE_QUN:
            event = self._pulse_parser.parse(raw, timestamp_ns, "spulse_qun")
            if event:
                self.pulse_events.append(event)
        
        # 信号事件 (SIGNAL)
        elif ext_event == CommEventType.COMM_SIGNAL:
            event = self._signal_parser.parse(raw, timestamp_ns)
            if event:
                self.signal_events.append(event)
        
        return event
    
    # ========================================================================
    # 事件查询接口
    # ========================================================================
    
    def get_all_events(self) -> List[CommBaseEvent]:
        """获取所有通信事件"""
        return (self.msg_events + self.reply_events + 
                self.pulse_events + self.signal_events)
