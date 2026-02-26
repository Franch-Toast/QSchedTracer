"""
QST Parser - Comm 类常量定义

通信事件类型常量。
参考: kercall_table_Events.html
"""

from enum import IntEnum


class CommEventType(IntEnum):
    """
    通信事件类型
    """
    COMM_SMSG = 1        # 发送消息
    COMM_RMSG = 2        # 接收消息
    COMM_REPLY = 3       # 回复
    COMM_ERROR = 4       # 错误
    COMM_SPULSE = 5      # 发送脉冲
    COMM_RPULSE = 6      # 接收脉冲
    COMM_SPULSE_EXE = 7  # SIGEV_PULSE delivered
    COMM_SPULSE_DIS = 8  # _PULSE_CODE_DISCONNECT
    COMM_SPULSE_DEA = 9  # _PULSE_CODE_COIDDEATH
    COMM_SPULSE_UN = 10  # _PULSE_CODE_UNBLOCK
    COMM_SPULSE_QUN = 11 # _PULSE_CODE_NET_UNBLOCK
    COMM_SIGNAL = 12     # 信号通信
