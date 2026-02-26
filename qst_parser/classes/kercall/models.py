"""
QST Parser - KerCall 类数据模型

内核调用事件的数据结构定义。

事件类型层次结构:
    BaseEvent
    └── KerCallBaseEvent (内核调用基类)
        ├── KerCallEnterEvent (进入事件基类)
        ├── KerCallExitEvent (退出事件基类)
        └── KerCallIntEvent (中断事件基类)

根据 kercall_table_Events.html，每个内核调用都有 ENTER 和 EXIT 两个版本:
- ENTER: 记录输入参数
- EXIT: 记录返回值和输出参数
"""

from dataclasses import dataclass, field
from typing import Any, Dict, Optional
from ...models.base import BaseEvent
from .constants import SIGNAL_NAMES, KernelCall


# ============================================================================
# 内核调用事件基类
# ============================================================================

@dataclass
class KerCallBaseEvent(BaseEvent):
    """
    内核调用事件基类
    
    所有内核调用事件的基类，包含公共字段。
    """
    kercall_num: int = 0        # 内核调用编号 (0-107)
    kercall_name: str = ""      # 内核调用名称
    is_64bit: bool = False      # 是否为 64 位版本
    is_wide: bool = False       # 是否为 Wide mode
    
    @property
    def kercall_type(self) -> str:
        """获取内核调用类型名称"""
        try:
            return KernelCall(self.kercall_num & 0x7F).name
        except ValueError:
            return f"KER_{self.kercall_num}"


@dataclass
class KerCallEnterEvent(KerCallBaseEvent):
    """
    内核调用进入事件基类
    
    记录进入内核调用时的输入参数。
    """
    event_type: str = "ENTER"
    
    # Fast mode 通用数据字段
    data1: int = 0              # 第一个参数
    data2: int = 0              # 第二个参数
    
    # Wide mode 额外数据
    extra_data: Dict[str, Any] = field(default_factory=dict)


@dataclass
class KerCallExitEvent(KerCallBaseEvent):
    """
    内核调用退出事件基类
    
    记录退出内核调用时的返回值和输出参数。
    
    注意: 根据 kercall_table_Events.html:
    "all _NTO_TRACE_KERCALLEXIT calls now have errno as the second
    parameter if the kercall failed (i.e., the retval is -1)"
    """
    event_type: str = "EXIT"
    
    # 返回值
    ret_val: int = 0            # 返回值
    errno_val: int = 0          # 错误码（当 ret_val == -1 时）
    
    # Wide mode 额外数据
    extra_data: Dict[str, Any] = field(default_factory=dict)


@dataclass
class KerCallIntEvent(KerCallBaseEvent):
    """
    内核调用被中断事件
    
    记录内核调用执行过程中被中断的情况。
    """
    event_type: str = "INT"
    
    # 通用数据字段
    data1: int = 0
    data2: int = 0
    extra_data: Dict[str, Any] = field(default_factory=dict)


# ============================================================================
# 兼容性: 保留旧的通用 KerCallEvent
# ============================================================================

@dataclass
class KerCallEvent(KerCallBaseEvent):
    """
    通用内核调用事件（兼容旧代码）
    
    存储所有内核调用的通用数据。
    建议使用更具体的 KerCallEnterEvent/KerCallExitEvent/KerCallIntEvent。
    """
    # Fast mode 数据
    data1: int = 0
    data2: int = 0
    
    # Wide mode 额外数据
    extra_data: Dict[str, Any] = field(default_factory=dict)


@dataclass
class SignalKillEvent(BaseEvent):
    """
    SignalKill 事件
    
    支持 Fast mode 和 Wide mode:
    
    Fast mode (SIMPLE):
        - data[0]: timestamp
        - data[1]: pid
        - data[2]: signo
    
    Wide mode (组合事件):
        COMBINE_BEGIN:
            - data[0]: timestamp
            - data[1]: nd
            - data[2]: pid
        COMBINE_CONT:
            - data[0]: timestamp
            - data[1]: tid
            - data[2]: signo
        COMBINE_END:
            - data[0]: timestamp
            - data[1]: code
            - data[2]: value
    """
    # 目标信息
    nd: int = 0                 # reserved (节点 ID)
    target_pid: int = 0         # 目标进程 ID
    target_tid: int = 0         # 目标线程 ID
    signo: int = 0              # 信号编号
    code: int = 0               # 信号代码
    value: int = 0              # 信号值
    
    # 推断的发送者 (从 RUNNING 事件)
    sender_pid: int = 0
    sender_tid: int = 0
    
    @property
    def signal_name(self) -> str:
        """获取信号名称"""
        return SIGNAL_NAMES.get(self.signo, f"SIG{self.signo}")


@dataclass
class MsgSendEvent(BaseEvent):
    """
    消息发送事件 (__KER_MSG_SENDV / __KER_MSG_SENDVNC)
    
    Fast mode:
        - coid, msg[0]
    
    Wide mode:
        - coid, sparts, rparts, msg[0], msg[1], msg[2]
    """
    coid: int = 0           # 连接 ID
    sparts: int = 0         # 发送部分数
    rparts: int = 0         # 接收部分数
    msg: tuple = (0, 0, 0)  # 消息数据


@dataclass
class MsgReceiveEvent(BaseEvent):
    """
    消息接收事件 (__KER_MSG_RECEIVEV / __KER_MSG_RECEIVEPULSEV)
    
    Fast mode:
        - chid, rparts
    
    Wide mode:
        - chid, rparts
    """
    chid: int = 0           # 频道 ID
    rparts: int = 0         # 接收部分数


@dataclass
class SyncEvent(BaseEvent):
    """
    同步原语事件 (Mutex, Condvar, Semaphore)
    
    Fast mode:
        - sync_p, count/owner
    
    Wide mode:
        - sync_p, count, owner, ...
    """
    sync_type: str = ""     # mutex/condvar/sem/create/destroy
    sync_ptr: int = 0       # 同步对象指针
    count: int = 0          # 计数/值
    owner: int = 0          # 所有者 (tid)


@dataclass
class TimerKerCallEvent(BaseEvent):
    """
    定时器内核调用事件
    """
    timer_id: int = 0
    flags: int = 0


@dataclass
class ChannelEvent(BaseEvent):
    """
    频道/连接事件
    """
    event_type: str = ""    # create/destroy/attach/detach
    chid: int = 0
    coid: int = 0
    flags: int = 0


@dataclass
class ThreadKerCallEvent(BaseEvent):
    """
    线程内核调用事件
    """
    event_type: str = ""    # create/destroy/join/cancel/etc
    tid: int = 0
    pid: int = 0
    func_ptr: int = 0
    arg_ptr: int = 0


# ============================================================================
# 同步原语事件 - 专用类
# ============================================================================

@dataclass
class MutexLockEnterEvent(KerCallEnterEvent):
    """
    Mutex Lock 进入事件
    
    Fast mode: sync_p, owner
    Wide mode: sync_p, count, owner
    """
    sync_ptr: int = 0       # 同步对象指针
    count: int = 0          # 锁计数（Wide mode）
    owner: int = 0          # 当前所有者 tid


@dataclass
class MutexLockExitEvent(KerCallExitEvent):
    """
    Mutex Lock 退出事件
    
    Fast/Wide mode: ret_val, empty
    """
    pass  # 使用基类的 ret_val, errno_val


@dataclass
class MutexUnlockEnterEvent(KerCallEnterEvent):
    """
    Mutex Unlock 进入事件
    
    Fast mode: sync_p, owner
    Wide mode: sync_p, count, owner
    """
    sync_ptr: int = 0       # 同步对象指针
    count: int = 0          # 锁计数（Wide mode）
    owner: int = 0          # 当前所有者 tid


@dataclass
class MutexUnlockExitEvent(KerCallExitEvent):
    """
    Mutex Unlock 退出事件
    """
    pass


@dataclass
class SemWaitEnterEvent(KerCallEnterEvent):
    """
    Semaphore Wait 进入事件
    
    Fast mode: sync_p, count
    Wide mode: sync_p, try, count, owner
    """
    sync_ptr: int = 0       # 同步对象指针
    try_flag: int = 0       # 是否为 try wait（Wide mode）
    count: int = 0          # 信号量计数
    owner: int = 0          # 所有者（Wide mode）


@dataclass
class SemWaitExitEvent(KerCallExitEvent):
    """
    Semaphore Wait 退出事件
    """
    pass


@dataclass
class SemPostEnterEvent(KerCallEnterEvent):
    """
    Semaphore Post 进入事件
    
    Fast mode: sync_p, count
    Wide mode: sync_p, count, owner
    """
    sync_ptr: int = 0       # 同步对象指针
    count: int = 0          # 信号量计数
    owner: int = 0          # 所有者（Wide mode）


@dataclass
class SemPostExitEvent(KerCallExitEvent):
    """
    Semaphore Post 退出事件
    """
    pass


@dataclass
class CondvarWaitEnterEvent(KerCallEnterEvent):
    """
    Condvar Wait 进入事件
    
    Fast mode: sync_p, mutex_p
    Wide mode: sync_p, mutex_p, sync->count, sync->owner, mutex->count, mutex->owner
    """
    sync_ptr: int = 0           # 条件变量指针
    mutex_ptr: int = 0          # 关联的 Mutex 指针
    sync_count: int = 0         # 条件变量等待计数（Wide mode）
    sync_owner: int = 0         # 条件变量所有者（Wide mode）
    mutex_count: int = 0        # Mutex 计数（Wide mode）
    mutex_owner: int = 0        # Mutex 所有者（Wide mode）


@dataclass
class CondvarWaitExitEvent(KerCallExitEvent):
    """
    Condvar Wait 退出事件
    """
    pass


@dataclass
class CondvarSignalEnterEvent(KerCallEnterEvent):
    """
    Condvar Signal 进入事件
    
    Fast mode: sync_p, all
    Wide mode: sync_p, all, sync->count, sync->owner
    """
    sync_ptr: int = 0           # 条件变量指针
    signal_all: int = 0         # 是否 broadcast（0=signal, 1=broadcast）
    sync_count: int = 0         # 条件变量等待计数（Wide mode）
    sync_owner: int = 0         # 条件变量所有者（Wide mode）


@dataclass
class CondvarSignalExitEvent(KerCallExitEvent):
    """
    Condvar Signal 退出事件
    """
    pass


# ============================================================================
# 消息传递事件 - 专用类
# ============================================================================

@dataclass
class MsgSendvEnterEvent(KerCallEnterEvent):
    """
    MsgSendv 进入事件
    
    Fast mode: coid, msg[0]
    Wide mode: coid, sparts, rparts, msg[0], msg[1], msg[2]
    """
    coid: int = 0               # 连接 ID
    sparts: int = 0             # 发送部分数（Wide mode）
    rparts: int = 0             # 接收部分数（Wide mode）
    msg: tuple = (0, 0, 0)      # 消息数据


@dataclass
class MsgSendvExitEvent(KerCallExitEvent):
    """
    MsgSendv 退出事件
    
    Fast mode: status, rmsg[0]
    Wide mode: status, rmsg[0], rmsg[1], rmsg[2]
    """
    rmsg: tuple = (0, 0, 0)     # 回复消息数据


@dataclass
class MsgReceivevEnterEvent(KerCallEnterEvent):
    """
    MsgReceivev 进入事件
    
    Fast mode: chid, rparts
    Wide mode: chid, rparts
    """
    chid: int = 0               # 频道 ID
    rparts: int = 0             # 接收部分数


@dataclass
class MsgReceivevExitEvent(KerCallExitEvent):
    """
    MsgReceivev 退出事件
    
    Fast mode: rcvid, rmsg[0]
    Wide mode: rcvid, rmsg[0-2], info->*
    """
    rcvid: int = 0              # 接收 ID
    rmsg: tuple = (0, 0, 0)     # 消息数据
    # Wide mode 的 msg_info
    info_nd: int = 0
    info_srcnd: int = 0
    info_pid: int = 0
    info_tid: int = 0
    info_chid: int = 0
    info_scoid: int = 0
    info_coid: int = 0
    info_msglen: int = 0
    info_srcmsglen: int = 0
    info_dstmsglen: int = 0
    info_priority: int = 0
    info_flags: int = 0


@dataclass
class MsgReplyvEnterEvent(KerCallEnterEvent):
    """
    MsgReplyv 进入事件
    
    Fast mode: rcvid, status
    Wide mode: rcvid, sparts, status, smsg[0], smsg[1], smsg[2]
    """
    rcvid: int = 0              # 接收 ID
    sparts: int = 0             # 发送部分数（Wide mode）
    status: int = 0             # 状态码
    smsg: tuple = (0, 0, 0)     # 发送消息数据


@dataclass
class MsgReplyvExitEvent(KerCallExitEvent):
    """
    MsgReplyv 退出事件
    """
    pass


# ============================================================================
# 调度事件 - 专用类
# ============================================================================

@dataclass
class SchedSetEnterEvent(KerCallEnterEvent):
    """
    SchedSet 进入事件
    
    Fast mode: pid, sched_priority
    Wide mode: pid, tid, policy, sched_priority, sched_curpriority,
               ss_low_priority, ss_max_repl, repl_period.tv_sec/nsec, init_budget.tv_sec/nsec
    """
    target_pid: int = 0
    target_tid: int = 0         # Wide mode
    policy: int = 0             # 调度策略（Wide mode）
    sched_priority: int = 0     # 新优先级
    sched_curpriority: int = 0  # 当前优先级（Wide mode）
    ss_low_priority: int = 0    # Sporadic: 低优先级（Wide mode）
    ss_max_repl: int = 0        # Sporadic: 最大补充次数（Wide mode）
    repl_period_sec: int = 0    # Sporadic: 补充周期秒（Wide mode）
    repl_period_nsec: int = 0   # Sporadic: 补充周期纳秒（Wide mode）
    init_budget_sec: int = 0    # Sporadic: 初始预算秒（Wide mode）
    init_budget_nsec: int = 0   # Sporadic: 初始预算纳秒（Wide mode）


@dataclass
class SchedSetExitEvent(KerCallExitEvent):
    """
    SchedSet 退出事件
    """
    pass


@dataclass
class SchedYieldEnterEvent(KerCallEnterEvent):
    """
    SchedYield 进入事件
    
    Fast/Wide mode: empty, empty
    """
    pass


@dataclass
class SchedYieldExitEvent(KerCallExitEvent):
    """
    SchedYield 退出事件
    """
    pass


# ============================================================================
# 配对后的事件 - 包含完整的 ENTER/EXIT 信息
# ============================================================================

@dataclass
class PairedSyncEvent:
    """
    配对后的同步原语事件
    
    将 ENTER 和 EXIT 事件配对，包含线程信息和持续时间。
    """
    # 线程信息（从 RUNNING 事件推断）
    pid: int = 0
    tid: int = 0
    
    # 时间信息
    enter_ts: int = 0           # ENTER 时间戳（纳秒）
    exit_ts: int = 0            # EXIT 时间戳（纳秒）
    
    # 事件类型
    sync_type: str = ""         # mutex_lock, sem_wait, condvar_wait, etc.
    
    # 同步对象信息（来自 ENTER 事件）
    sync_ptr: int = 0           # 同步对象指针
    
    # CPU 信息
    enter_cpu: int = 0          # ENTER 时的 CPU
    exit_cpu: int = 0           # EXIT 时的 CPU
    
    # 返回值（来自 EXIT 事件）
    ret_val: int = 0
    errno_val: int = 0
    
    @property
    def duration_ns(self) -> int:
        """持续时间（纳秒）"""
        return self.exit_ts - self.enter_ts


@dataclass
class PairedIpcEvent:
    """
    配对后的 IPC 事件
    
    将 ENTER 和 EXIT 事件配对，包含线程信息和持续时间。
    """
    # 线程信息（从 RUNNING 事件推断）
    pid: int = 0
    tid: int = 0
    
    # 时间信息
    enter_ts: int = 0           # ENTER 时间戳（纳秒）
    exit_ts: int = 0            # EXIT 时间戳（纳秒）
    
    # 事件类型
    ipc_type: str = ""          # msg_send, msg_reply
    
    # IPC 信息（来自 ENTER 事件）
    coid: int = 0               # MsgSend: 连接 ID
    rcvid: int = 0              # MsgReply: 接收 ID
    chid: int = 0               # MsgReceive: 频道 ID
    
    # CPU 信息
    enter_cpu: int = 0
    exit_cpu: int = 0
    
    # 返回值
    ret_val: int = 0
    errno_val: int = 0
    
    @property
    def duration_ns(self) -> int:
        """持续时间（纳秒）"""
        return self.exit_ts - self.enter_ts
