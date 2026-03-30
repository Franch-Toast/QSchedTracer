"""
QST Parser - Thread 类常量定义

线程状态相关的常量和枚举。
参考: sys/states.h, kercall_table_Events.html
"""

from enum import IntEnum


class ThreadState(IntEnum):
    """
    线程状态枚举
    
    来自 sys/states.h，对应 _NTO_TRACE_THREAD 事件的各个状态。
    """
    DEAD = 0           # 线程已死亡
    RUNNING = 1        # 正在运行
    READY = 2          # 就绪等待调度
    STOPPED = 3        # 被停止（调试）
    SEND = 4           # 阻塞在消息发送
    RECEIVE = 5        # 阻塞在消息接收
    REPLY = 6          # 阻塞等待回复
    STACK = 7          # 等待栈分配
    WAITTHREAD = 8     # 等待其他线程
    WAITPAGE = 9       # 等待页面
    SIGSUSPEND = 10    # 信号挂起
    SIGWAITINFO = 11   # 等待信号信息
    NANOSLEEP = 12     # 睡眠
    MUTEX = 13         # 阻塞在互斥锁
    CONDVAR = 14       # 阻塞在条件变量
    JOIN = 15          # 等待线程 join
    INTR = 16          # 等待中断
    SEM = 17           # 阻塞在信号量
    WAITCTX = 18       # 等待上下文
    NET_SEND = 19      # 网络发送
    NET_REPLY = 20     # 网络回复


# 线程状态码到名称的映射
THREAD_STATES = {
    ThreadState.DEAD: "DEAD",
    ThreadState.RUNNING: "RUNNING",
    ThreadState.READY: "READY",
    ThreadState.STOPPED: "STOPPED",
    ThreadState.SEND: "SEND",
    ThreadState.RECEIVE: "RECEIVE",
    ThreadState.REPLY: "REPLY",
    ThreadState.STACK: "STACK",
    ThreadState.WAITTHREAD: "WAITTHREAD",
    ThreadState.WAITPAGE: "WAITPAGE",
    ThreadState.SIGSUSPEND: "SIGSUSPEND",
    ThreadState.SIGWAITINFO: "SIGWAITINFO",
    ThreadState.NANOSLEEP: "NANOSLEEP",
    ThreadState.MUTEX: "MUTEX",
    ThreadState.CONDVAR: "CONDVAR",
    ThreadState.JOIN: "JOIN",
    ThreadState.INTR: "INTR",
    ThreadState.SEM: "SEM",
    ThreadState.WAITCTX: "WAITCTX",
    ThreadState.NET_SEND: "NET_SEND",
    ThreadState.NET_REPLY: "NET_REPLY",
    24: "CREATE",    # _TRACE_THREAD_CREATE = STATE_MAX
    25: "DESTROY",   # _TRACE_THREAD_DESTROY
}


# 线程状态的最大数量 (用于内部/外部事件号转换)
# 参考 trace.h: _TRACE_MAX_TH_STATE_NUM = 26 (STATE_MAX(24) + CREATE + DESTROY)
MAX_TH_STATE_NUM = 26


# 参考 trace.h: _TRACE_THREAD_CREATE = STATE_MAX(24), _TRACE_THREAD_DESTROY = 25
THREAD_STATE_CREATE = 24
THREAD_STATE_DESTROY = 25
