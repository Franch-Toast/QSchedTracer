"""
QST Parser - Comm 类数据模型

通信事件的数据结构定义。

参考: kercall_table_Events.html

事件类型:
- COMM_SMSG / COMM_RMSG: 消息发送/接收
- COMM_REPLY / COMM_ERROR: 回复/错误
- COMM_SPULSE / COMM_RPULSE: 脉冲发送/接收
- COMM_SPULSE_*: 特殊脉冲类型
- COMM_SIGNAL: 信号
"""

from dataclasses import dataclass, field
from typing import List
from ...models.base import BaseEvent


@dataclass
class CommBaseEvent(BaseEvent):
    """通信事件基类"""
    is_wide: bool = False


@dataclass
class CommMsgEvent(CommBaseEvent):
    """
    消息事件 (COMM_SMSG / COMM_RMSG)
    
    Fast/Wide: rcvid, pid
    
    用途:
    - SMSG: 消息已发送，rcvid 标识接收者，pid 是发送者进程
    - RMSG: 消息已接收，rcvid 标识发送者，pid 是接收者进程
    """
    event_type: str = ""     # smsg / rmsg
    rcvid: int = 0           # 接收 ID
    pid: int = 0             # 进程 ID


@dataclass
class CommReplyEvent(CommBaseEvent):
    """
    回复/错误事件 (COMM_REPLY / COMM_ERROR)
    
    Fast/Wide: tid, pid
    
    用途:
    - REPLY: 消息回复完成
    - ERROR: 消息错误
    
    注意：QNX 的 "tid" 字段是复合值，格式为 0xNNNNTTTT：
    - 高 16 位 (nd): 节点描述符 (node descriptor)
    - 低 16 位 (tid): 真正的线程 ID
    
    真正的 tid = tid_raw & 0xFFFF
    """
    event_type: str = ""         # reply / error
    tid_raw: int = 0             # 原始复合值 (高16位=nd, 低16位=tid)
    tid: int = 0                 # 真正的线程 ID (低 16 位)
    nd: int = 0                  # 节点描述符 (高 16 位)
    pid: int = 0                 # 进程 ID


@dataclass
class CommPulseEvent(CommBaseEvent):
    """
    脉冲事件 (COMM_SPULSE / COMM_RPULSE / COMM_SPULSE_*)
    
    Fast/Wide: scoid, pid
    
    事件类型:
    - SPULSE: 发送脉冲
    - RPULSE: 接收脉冲
    - SPULSE_EXE: SIGEV_PULSE delivered
    - SPULSE_DIS: _PULSE_CODE_DISCONNECT pulse delivered
    - SPULSE_DEA: _PULSE_CODE_COIDDEATH pulse delivered
    - SPULSE_UN: _PULSE_CODE_UNBLOCK delivered
    - SPULSE_QUN: _PULSE_CODE_NET_UNBLOCK delivered
    """
    event_type: str = ""     # spulse / rpulse / spulse_exe / ...
    scoid: int = 0           # 服务器连接 ID
    pid: int = 0             # 进程 ID


@dataclass
class CommSignalEvent(CommBaseEvent):
    """
    信号事件 (COMM_SIGNAL)
    
    Fast: si_signo, si_code
    Wide: si_signo, si_code, si_errno, __data.__pad[0], __data.__pad[1], 
          __data.__pad[2], __data.__pad[3], __data.__pad[4], __data.__pad[5]
    
    用途:
    - 记录信号的传递和相关信息
    """
    si_signo: int = 0        # 信号编号
    si_code: int = 0         # 信号代码
    si_errno: int = 0        # 错误码 (Wide mode)
    si_data: List[int] = field(default_factory=list)  # 附加数据 (Wide mode)


# 兼容旧代码
@dataclass
class CommEvent(BaseEvent):
    """
    通用通信事件（兼容旧代码）
    
    建议使用更具体的事件类型。
    """
    event_type: str = ""
    rcvid: int = 0
    scoid: int = 0
    pid: int = 0
    tid: int = 0              # 真正的 tid（REPLY/ERROR 已从复合值中提取）
    tid_raw: int = 0          # 原始复合值
    nd: int = 0               # 节点描述符
    si_signo: int = 0
    si_code: int = 0
    si_errno: int = 0
