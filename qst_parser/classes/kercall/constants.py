"""
QST Parser - KerCall 类常量定义

内核调用编号和信号相关常量。
参考: sys/kercalls.h, kercall_table_Events.html
"""

from enum import IntEnum


class KernelCall(IntEnum):
    """
    内核调用编号
    
    来自 sys/kercalls.h
    """
    # 系统调用
    NOP = 0                     # __KER_NOP
    TRACE_EVENT = 1             # __KER_TRACE_EVENT
    RING0 = 2                   # __KER_RING0
    CACHE_FLUSH = 3             # __KER_CACHE_FLUSH
    SYS_SRANDOM = 4             # __KER_SYS_SRANDOM
    MSG_REGISTER_EVENT = 5      # __KER_MSG_REGISTER_EVENT
    MSG_SEND_PULSEPTR = 6       # __KER_MSG_SEND_PULSEPTR
    SYS_CPUPAGE_GET = 7         # __KER_SYS_CPUPAGE_GET
    SYS_CPUPAGE_SET = 8         # __KER_SYS_CPUPAGE_SET
    
    # 消息传递
    MSG_PAUSE = 9               # __KER_MSG_PAUSE
    MSG_CURRENT = 10            # __KER_MSG_CURRENT
    MSG_SENDV = 11              # __KER_MSG_SENDV
    MSG_SENDVNC = 12            # __KER_MSG_SENDVNC
    MSG_ERROR = 13              # __KER_MSG_ERROR
    MSG_RECEIVEV = 14           # __KER_MSG_RECEIVEV
    MSG_REPLYV = 15             # __KER_MSG_REPLYV
    MSG_READV = 16              # __KER_MSG_READV
    MSG_WRITEV = 17             # __KER_MSG_WRITEV
    MSG_READWRITEV = 18         # __KER_MSG_READWRITEV
    MSG_INFO = 19               # __KER_MSG_INFO
    MSG_SEND_PULSE = 20         # __KER_MSG_SEND_PULSE
    MSG_DELIVER_EVENT = 21      # __KER_MSG_DELIVER_EVENT
    MSG_KEYDATA = 22            # __KER_MSG_KEYDATA
    MSG_READIOV = 23            # __KER_MSG_READIOV
    MSG_RECEIVEPULSEV = 24      # __KER_MSG_RECEIVEPULSEV
    MSG_VERIFY_EVENT = 25       # __KER_MSG_VERIFY_EVENT
    
    # 信号
    SIGNAL_KILL = 26            # __KER_SIGNAL_KILL
    SIGNAL_RETURN = 27          # __KER_SIGNAL_RETURN
    SIGNAL_FAULT = 28           # __KER_SIGNAL_FAULT
    SIGNAL_ACTION = 29          # __KER_SIGNAL_ACTION
    SIGNAL_PROCMASK = 30        # __KER_SIGNAL_PROCMASK
    SIGNAL_SUSPEND = 31         # __KER_SIGNAL_SUSPEND
    SIGNAL_WAITINFO = 32        # __KER_SIGNAL_WAITINFO
    SIGNAL_KILL_SIGVAL = 33     # __KER_SIGNAL_KILL_SIGVAL
    SIGNAL_SPARE2 = 34          # __KER_SIGNAL_SPARE2
    
    # Channel/Connection
    CHANNEL_CREATE = 35         # __KER_CHANNEL_CREATE
    CHANNEL_DESTROY = 36        # __KER_CHANNEL_DESTROY
    CHANCON_ATTR = 37           # __KER_CHANCON_ATTR
    CHANNEL_SPARE1 = 38         # __KER_CHANNEL_SPARE1
    CONNECT_ATTACH = 39         # __KER_CONNECT_ATTACH
    CONNECT_DETACH = 40         # __KER_CONNECT_DETACH
    CONNECT_SERVER_INFO = 41    # __KER_CONNECT_SERVER_INFO
    CONNECT_CLIENT_INFO = 42    # __KER_CONNECT_CLIENT_INFO
    CONNECT_FLAGS = 43          # __KER_CONNECT_FLAGS
    CONNECT_SPARE1 = 44         # __KER_CONNECT_SPARE1
    CONNECT_SPARE2 = 45         # __KER_CONNECT_SPARE2
    
    # 线程
    THREAD_CREATE = 46          # __KER_THREAD_CREATE
    THREAD_DESTROY = 47         # __KER_THREAD_DESTROY
    THREAD_DESTROYALL = 48      # __KER_THREAD_DESTROYALL
    THREAD_DETACH = 49          # __KER_THREAD_DETACH
    THREAD_JOIN = 50            # __KER_THREAD_JOIN
    THREAD_CANCEL = 51          # __KER_THREAD_CANCEL
    THREAD_CTL = 52             # __KER_THREAD_CTL
    THREAD_CTLEXT = 53          # __KER_THREAD_CTLEXT
    THREAD_SPARE1 = 54          # __KER_THREAD_SPARE1
    
    # 中断
    INTERRUPT_ATTACH = 55       # __KER_INTERRUPT_ATTACH
    INTERRUPT_DETACH_FUNC = 56  # __KER_INTERRUPT_DETACH_FUNC
    INTERRUPT_DETACH = 57       # __KER_INTERRUPT_DETACH
    INTERRUPT_WAIT = 58         # __KER_INTERRUPT_WAIT
    INTERRUPT_MASK = 59         # __KER_INTERRUPT_MASK
    INTERRUPT_UNMASK = 60       # __KER_INTERRUPT_UNMASK
    INTERRUPT_CHARACTERISTIC = 61  # __KER_INTERRUPT_CHARACTERISTIC
    INTERRUPT_SPARE2 = 62       # __KER_INTERRUPT_SPARE2
    INTERRUPT_SPARE3 = 63       # __KER_INTERRUPT_SPARE3
    INTERRUPT_SPARE4 = 64       # __KER_INTERRUPT_SPARE4
    
    # 时钟
    CLOCK_TIME = 65             # __KER_CLOCK_TIME
    CLOCK_ADJUST = 66           # __KER_CLOCK_ADJUST
    CLOCK_PERIOD = 67           # __KER_CLOCK_PERIOD
    CLOCK_ID = 68               # __KER_CLOCK_ID
    CLOCK_SPARE2 = 69           # __KER_CLOCK_SPARE2
    
    # 定时器
    TIMER_CREATE = 70           # __KER_TIMER_CREATE
    TIMER_DESTROY = 71          # __KER_TIMER_DESTROY
    TIMER_SETTIME = 72          # __KER_TIMER_SETTIME
    TIMER_INFO = 73             # __KER_TIMER_INFO
    TIMER_ALARM = 74            # __KER_TIMER_ALARM
    TIMER_TIMEOUT = 75          # __KER_TIMER_TIMEOUT
    TIMER_SPARE1 = 76           # __KER_TIMER_SPARE1
    TIMER_SPARE2 = 77           # __KER_TIMER_SPARE2
    
    # 同步原语
    SYNC_CREATE = 78            # __KER_SYNC_CREATE
    SYNC_DESTROY = 79           # __KER_SYNC_DESTROY
    SYNC_MUTEX_LOCK = 80        # __KER_SYNC_MUTEX_LOCK
    SYNC_MUTEX_UNLOCK = 81      # __KER_SYNC_MUTEX_UNLOCK
    SYNC_CONDVAR_WAIT = 82      # __KER_SYNC_CONDVAR_WAIT
    SYNC_CONDVAR_SIGNAL = 83    # __KER_SYNC_CONDVAR_SIGNAL
    SYNC_SEM_POST = 84          # __KER_SYNC_SEM_POST
    SYNC_SEM_WAIT = 85          # __KER_SYNC_SEM_WAIT
    SYNC_CTL = 86               # __KER_SYNC_CTL
    SYNC_MUTEX_REVIVE = 87      # __KER_SYNC_MUTEX_REVIVE
    
    # 调度
    SCHED_GET = 88              # __KER_SCHED_GET
    SCHED_SET = 89              # __KER_SCHED_SET
    SCHED_YIELD = 90            # __KER_SCHED_YIELD
    SCHED_INFO = 91             # __KER_SCHED_INFO
    SCHED_CTL = 92              # __KER_SCHED_CTL
    
    # 网络
    NET_CRED = 93               # __KER_NET_CRED
    NET_VTID = 94               # __KER_NET_VTID
    NET_UNBLOCK = 95            # __KER_NET_UNBLOCK
    NET_INFOSCOID = 96          # __KER_NET_INFOSCOID
    NET_SIGNAL_KILL = 97        # __KER_NET_SIGNAL_KILL
    NET_SPARE1 = 98             # __KER_NET_SPARE1
    NET_SPARE2 = 99             # __KER_NET_SPARE2
    
    # 电源管理
    POWER_PARAMETER = 100       # __KER_POWER_PARAMETER
    POWER_ACTIVE = 101          # __KER_POWER_ACTIVE
    SCHED_WAYPOINT = 102        # __KER_SCHED_WAYPOINT
    POWER_SPARE2 = 103          # __KER_POWER_SPARE2
    POWER_SPARE3 = 104          # __KER_POWER_SPARE3
    POWER_SPARE4 = 105          # __KER_POWER_SPARE4
    
    # 其他
    SYS_CUSTOM = 106            # __KER_SYS_CUSTOM
    BAD = 107                   # __KER_BAD


# 信号名称映射
SIGNAL_NAMES = {
    1: "SIGHUP",
    2: "SIGINT",
    3: "SIGQUIT",
    4: "SIGILL",
    5: "SIGTRAP",
    6: "SIGABRT",
    7: "SIGBUS",
    8: "SIGFPE",
    9: "SIGKILL",
    10: "SIGUSR1",
    11: "SIGSEGV",
    12: "SIGUSR2",
    13: "SIGPIPE",
    14: "SIGALRM",
    15: "SIGTERM",
    16: "SIGSTKFLT",
    17: "SIGCHLD",
    18: "SIGCONT",
    19: "SIGSTOP",
    20: "SIGTSTP",
    21: "SIGTTIN",
    22: "SIGTTOU",
    23: "SIGURG",
    24: "SIGXCPU",
    25: "SIGXFSZ",
    26: "SIGVTALRM",
    27: "SIGPROF",
    28: "SIGWINCH",
    29: "SIGIO",
    30: "SIGPWR",
    31: "SIGSYS",
}


# 64位内核调用标志
KERCALL_64 = 0x200  # _NTO_TRACE_KERCALL64


# 内核调用分类
KERCALL_MSG = {
    KernelCall.MSG_PAUSE, KernelCall.MSG_CURRENT, KernelCall.MSG_SENDV,
    KernelCall.MSG_SENDVNC, KernelCall.MSG_ERROR, KernelCall.MSG_RECEIVEV,
    KernelCall.MSG_REPLYV, KernelCall.MSG_READV, KernelCall.MSG_WRITEV,
    KernelCall.MSG_READWRITEV, KernelCall.MSG_INFO, KernelCall.MSG_SEND_PULSE,
    KernelCall.MSG_DELIVER_EVENT, KernelCall.MSG_KEYDATA, KernelCall.MSG_READIOV,
    KernelCall.MSG_RECEIVEPULSEV, KernelCall.MSG_VERIFY_EVENT,
    KernelCall.MSG_SEND_PULSEPTR, KernelCall.MSG_REGISTER_EVENT,
}

KERCALL_SIGNAL = {
    KernelCall.SIGNAL_KILL, KernelCall.SIGNAL_RETURN, KernelCall.SIGNAL_FAULT,
    KernelCall.SIGNAL_ACTION, KernelCall.SIGNAL_PROCMASK, KernelCall.SIGNAL_SUSPEND,
    KernelCall.SIGNAL_WAITINFO, KernelCall.SIGNAL_KILL_SIGVAL,
    KernelCall.NET_SIGNAL_KILL,
}

KERCALL_CHANNEL = {
    KernelCall.CHANNEL_CREATE, KernelCall.CHANNEL_DESTROY, KernelCall.CHANCON_ATTR,
    KernelCall.CONNECT_ATTACH, KernelCall.CONNECT_DETACH, KernelCall.CONNECT_SERVER_INFO,
    KernelCall.CONNECT_CLIENT_INFO, KernelCall.CONNECT_FLAGS,
}

KERCALL_THREAD = {
    KernelCall.THREAD_CREATE, KernelCall.THREAD_DESTROY, KernelCall.THREAD_DESTROYALL,
    KernelCall.THREAD_DETACH, KernelCall.THREAD_JOIN, KernelCall.THREAD_CANCEL,
    KernelCall.THREAD_CTL, KernelCall.THREAD_CTLEXT,
}

KERCALL_INTERRUPT = {
    KernelCall.INTERRUPT_ATTACH, KernelCall.INTERRUPT_DETACH_FUNC,
    KernelCall.INTERRUPT_DETACH, KernelCall.INTERRUPT_WAIT,
    KernelCall.INTERRUPT_MASK, KernelCall.INTERRUPT_UNMASK,
    KernelCall.INTERRUPT_CHARACTERISTIC,
}

KERCALL_CLOCK = {
    KernelCall.CLOCK_TIME, KernelCall.CLOCK_ADJUST, KernelCall.CLOCK_PERIOD,
    KernelCall.CLOCK_ID,
}

KERCALL_TIMER = {
    KernelCall.TIMER_CREATE, KernelCall.TIMER_DESTROY, KernelCall.TIMER_SETTIME,
    KernelCall.TIMER_INFO, KernelCall.TIMER_ALARM, KernelCall.TIMER_TIMEOUT,
}

KERCALL_SYNC = {
    KernelCall.SYNC_CREATE, KernelCall.SYNC_DESTROY, KernelCall.SYNC_MUTEX_LOCK,
    KernelCall.SYNC_MUTEX_UNLOCK, KernelCall.SYNC_CONDVAR_WAIT,
    KernelCall.SYNC_CONDVAR_SIGNAL, KernelCall.SYNC_SEM_POST, KernelCall.SYNC_SEM_WAIT,
    KernelCall.SYNC_CTL, KernelCall.SYNC_MUTEX_REVIVE,
}

KERCALL_SCHED = {
    KernelCall.SCHED_GET, KernelCall.SCHED_SET, KernelCall.SCHED_YIELD,
    KernelCall.SCHED_INFO, KernelCall.SCHED_CTL, KernelCall.SCHED_WAYPOINT,
}
