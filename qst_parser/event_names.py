"""
Kernel call and event name tables for QNX 7.1 and 8.0.

Derived from:
  - sys/kercalls.h (QNX 7.1 and 8.0)
  - sys/trace.h (_NTO_TRACE_COMM_*, _NTO_TRACE_SYS_*)
  - SAT kercall_table_Events.html

QNX 7.1 and 8.0 use DIFFERENT __KER_* numbering.
The trace event code == __KER_* index (0..127).
"""

# ============================================================================
# QNX 7.1 __KER_* -> human-readable name  (from kercalls.h)
# ============================================================================
KERCALL_NAMES_71 = {
    0: 'Nop',
    1: 'TraceEvent',
    2: 'Ring0',
    3: 'CacheFlush',
    4: 'SysSrandom',
    5: 'MsgRegisterEvent',
    6: 'MsgSendPulsePtr',
    7: 'SysCpupageGet',
    8: 'SysCpupageSet',
    9: 'MsgPause',
    10: 'MsgCurrent',
    11: 'MsgSendv',
    12: 'MsgSendvnc',
    13: 'MsgError',
    14: 'MsgReceivev',
    15: 'MsgReplyv',
    16: 'MsgReadv',
    17: 'MsgWritev',
    18: 'MsgReadwritev',
    19: 'MsgInfo',
    20: 'MsgSendPulse',
    21: 'MsgDeliverEvent',
    22: 'MsgKeyData',
    23: 'MsgReadiov',
    24: 'MsgReceivePulsev',
    25: 'MsgVerifyEvent',
    26: 'SignalKill',
    27: 'SignalReturn',
    28: 'SignalFault',
    29: 'SignalAction',
    30: 'SignalProcmask',
    31: 'SignalSuspend',
    32: 'SignalWaitinfo',
    33: 'SignalKillSigval',
    35: 'ChannelCreate',
    36: 'ChannelDestroy',
    37: 'ChanconAttr',
    39: 'ConnectAttach',
    40: 'ConnectDetach',
    41: 'ConnectServerInfo',
    42: 'ConnectClientInfo',
    43: 'ConnectFlags',
    46: 'ThreadCreate',
    47: 'ThreadDestroy',
    48: 'ThreadDestroyAll',
    49: 'ThreadDetach',
    50: 'ThreadJoin',
    51: 'ThreadCancel',
    52: 'ThreadCtl',
    53: 'ThreadCtlExt',
    55: 'InterruptAttach',
    56: 'InterruptDetachFunc',
    57: 'InterruptDetach',
    58: 'InterruptWait',
    59: 'InterruptMask',
    60: 'InterruptUnmask',
    61: 'InterruptCharacteristic',
    65: 'ClockTime',
    66: 'ClockAdjust',
    67: 'ClockPeriod',
    68: 'ClockId',
    70: 'TimerCreate',
    71: 'TimerDestroy',
    72: 'TimerSettime',
    73: 'TimerInfo',
    74: 'TimerAlarm',
    75: 'TimerTimeout',
    78: 'SyncCreate',
    79: 'SyncDestroy',
    80: 'SyncMutexLock',
    81: 'SyncMutexUnlock',
    82: 'SyncCondvarWait',
    83: 'SyncCondvarSignal',
    84: 'SyncSemPost',
    85: 'SyncSemWait',
    86: 'SyncCtl',
    87: 'SyncMutexRevive',
    88: 'SchedGet',
    89: 'SchedSet',
    90: 'SchedYield',
    91: 'SchedInfo',
    92: 'SchedCtl',
    93: 'NetCred',
    94: 'NetVtid',
    95: 'NetUnblock',
    96: 'NetInfoscoid',
    97: 'NetSignalKill',
    100: 'PowerParameter',
    101: 'PowerActive',
    102: 'SchedWaypoint',
    106: 'SysCustom',
    107: 'Bad',
}

# ============================================================================
# QNX 8.0 __KER_* -> human-readable name  (from kercalls.h)
# ============================================================================
KERCALL_NAMES_80 = {
    0: 'Nop',
    1: 'TraceEvent',
    2: 'Ring0',
    3: 'CacheControl',
    4: 'SysSrandom',
    5: 'MsgRegisterEvent',
    6: 'MsgSendPulsePtr',
    7: 'SysCpupageGet',
    8: 'SysCpupageSet',
    9: 'MsgPause',
    10: 'MsgCurrent',
    11: 'MsgSendv',
    12: 'MsgSendvnc',
    13: 'MsgError',
    14: 'MsgReceivev',
    15: 'MsgReplyv',
    16: 'MsgReadv',
    17: 'MsgWritev',
    19: 'MsgInfo',
    20: 'MsgSendPulse',
    21: 'MsgDeliverEvent',
    24: 'MsgReceivePulsev',
    25: 'MsgVerifyEvent',
    26: 'SignalKill',
    27: 'SignalReturn',
    29: 'SignalAction',
    30: 'SignalProcmask',
    31: 'SignalSuspend',
    32: 'SignalWaitinfo',
    33: 'SignalKillSigval',
    35: 'ChannelCreate',
    36: 'ChannelDestroy',
    37: 'ChannelCtl',
    39: 'ConnectAttach',
    40: 'ConnectDetach',
    41: 'ConnectServerInfo',
    42: 'ConnectClientInfo',
    43: 'ConnectFlags',
    44: 'ConnectDupfd',
    46: 'ThreadCreate',
    47: 'ThreadDestroy',
    49: 'ThreadDetach',
    50: 'ThreadJoin',
    51: 'ThreadCancel',
    52: 'ThreadCtl',
    53: 'ThreadCtlExt',
    55: 'InterruptAttach',
    56: 'InterruptQuery',
    57: 'InterruptDetach',
    58: 'InterruptWait',
    59: 'InterruptMask',
    60: 'InterruptUnmask',
    61: 'InterruptUnblock',
    62: 'InterruptCtl',
    65: 'ClockTime',
    66: 'ClockAdjust',
    67: 'ClockPeriod',
    68: 'ClockId',
    70: 'TimerCreate',
    71: 'TimerDestroy',
    72: 'TimerSettime',
    73: 'TimerInfo',
    74: 'TimerAlarm',
    75: 'TimerTimeout',
    76: 'TimerDelegate',
    77: 'SyncRwlock',
    78: 'SyncCreate',
    79: 'SyncDestroy',
    80: 'SyncMutexLock',
    81: 'SyncMutexUnlock',
    82: 'SyncCondvarWait',
    83: 'SyncCondvarSignal',
    84: 'SyncSemPost',
    85: 'SyncSemWait',
    86: 'SyncCtl',
    87: 'SyncBarrierWait',
    88: 'SchedGet',
    89: 'SchedSet',
    90: 'SchedYield',
    91: 'SchedInfo',
    92: 'SchedCtl',
    93: 'MsgQueueOpen',
    94: 'MsgQueueSend',
    95: 'MsgQueueReceive',
    96: 'MsgQueueClose',
    97: 'MsgQueueCtl',
    98: 'PipeOpen',
    99: 'PipeCtl',
    100: 'PipeClose',
    104: 'SyncTypeDestroy',
    105: 'Test',
    106: 'SysCustom',
    107: 'Bad',
}


def get_kercall_names(os_version: int) -> dict:
    """Return the kercall name table for the given OS version."""
    if os_version >= 800:
        return KERCALL_NAMES_80
    return KERCALL_NAMES_71


# ============================================================================
# COMM events  (from trace.h: _NTO_TRACE_COMM_*)
# Identical numbering between QNX 7.1 and 8.0
# ============================================================================
COMM_NAMES = {
    0: 'SMSG',           # _NTO_TRACE_COMM_SMSG  (message send)
    1: 'SPULSE',         # _NTO_TRACE_COMM_SPULSE (send pulse)
    2: 'RMSG',           # _NTO_TRACE_COMM_RMSG  (receive message)
    3: 'RPULSE',         # _NTO_TRACE_COMM_RPULSE (receive pulse)
    4: 'SPULSE_EXE',     # _NTO_TRACE_COMM_SPULSE_EXE (SIGEV_PULSE delivered)
    5: 'SPULSE_DIS',     # _NTO_TRACE_COMM_SPULSE_DIS (_PULSE_CODE_DISCONNECT)
    6: 'SPULSE_DEA',     # _NTO_TRACE_COMM_SPULSE_DEA (_PULSE_CODE_COIDDEATH)
    7: 'SPULSE_UN',      # _NTO_TRACE_COMM_SPULSE_UN (_PULSE_CODE_UNBLOCK)
    8: 'SPULSE_QUN',     # _NTO_TRACE_COMM_SPULSE_QUN (queue unblock)
    9: 'SIGNAL',         # _NTO_TRACE_COMM_SIGNAL
    10: 'REPLY',         # _NTO_TRACE_COMM_REPLY
    11: 'ERROR',         # _NTO_TRACE_COMM_ERROR
}

# COMM event detail: what d1 and d2 contain (from SAT kercall_table_Events.html)
# 0 SMSG:  d1=rcvid, d2=pid
# 1 SPULSE: d1=scoid, d2=pid
# 2 RMSG:  d1=rcvid, d2=pid
# 3 RPULSE: d1=scoid, d2=pid
# 4 SPULSE_EXE: d1=scoid, d2=pid
# 5 SPULSE_DIS: d1=scoid, d2=pid
# 6 SPULSE_DEA: d1=scoid, d2=pid
# 7 SPULSE_UN:  d1=scoid, d2=pid
# 8 SPULSE_QUN: d1=scoid, d2=pid
# 9 SIGNAL: d1=si_signo, d2=si_code
# 10 REPLY: d1=tid, d2=pid
# 11 ERROR: d1=tid, d2=pid

# Descriptive field names per COMM event.
# Fields represent the communication TARGET, prefixed with 'target_'
# to avoid collision with the event owner's pid/tid added later.
COMM_FIELDS = {
    0: ('target_rcvid', 'target_pid'),
    1: ('target_scoid', 'target_pid'),
    2: ('target_rcvid', 'target_pid'),
    3: ('target_scoid', 'target_pid'),
    4: ('target_scoid', 'target_pid'),
    5: ('target_scoid', 'target_pid'),
    6: ('target_scoid', 'target_pid'),
    7: ('target_scoid', 'target_pid'),
    8: ('target_scoid', 'target_pid'),
    9: ('si_signo', 'si_code'),
    10: ('target_tid', 'target_pid'),
    11: ('target_tid', 'target_pid'),
}


# ============================================================================
# SYSTEM events  (from trace.h: _NTO_TRACE_SYS_*)
# Values are the low 5 bits of int_event (masked & 0x1F in handler)
# ============================================================================
SYSTEM_NAMES = {
    0x01: 'SYS_RESERVED',
    0x02: 'PATHMGR',           # pathname operation
    0x03: 'APS_NAME',          # APS partition name
    0x04: 'APS_BUDGETS',       # APS budgets
    0x05: 'APS_BANKRUPTCY',    # APS bankruptcy
    0x06: 'MMAP',              # mmap/mmap64
    0x07: 'MUNMAP',            # munmap
    0x08: 'MAPNAME',           # dlopen
    0x09: 'ADDRESS',           # breakpoint
    0x0a: 'FUNC_ENTER',        # function enter (instrumented)
    0x0b: 'FUNC_EXIT',         # function exit (instrumented)
    0x0c: 'SLOG',              # kernel slog
    0x0d: 'DEFRAG_START',      # defragmentation start
    0x0e: 'RUNSTATE',          # CPU runstate change
    0x0f: 'POWER',             # power/idle/frequency
    0x10: 'IPI',               # inter-processor interrupt
    0x11: 'PAGEWAIT',          # page fault
    0x12: 'TIMER',             # timer expiry
    0x13: 'DEFRAG_END',        # defragmentation end
    0x14: 'PROFILE',           # statistical profile
    0x15: 'MAPNAME_64',        # dlopen (64-bit addr)
    0x16: 'APS_PSTATS',        # APS partition stats
    0x17: 'APS_OSTATS',        # APS overall stats
    0x18: 'APS_INFO',          # APS info
    0x19: 'APS_JOIN',          # APS join
    0x1a: 'APS_THREAD',        # APS thread
    0x1b: 'APS_PROCESS',       # APS process
    0x1c: 'SCHED_CONF',        # sched configure
    0x1d: 'IST_ATTACH',        # IST attach
    0x1e: 'IST_DETACH',        # IST detach
    0x1f: 'PCTRACE',           # PC trace
}


# ============================================================================
# Kercall field layouts  (from SAT kercall_table_Events.html)
# Key: call_num  Value: (fast_fields, wide_fields)
# fast_fields maps to the 2 data words in a SIMPLE event
# wide_fields maps to all data words in a merged combine event
# The same call_num mapping applies to both QNX 7.1 and 8.0 since
# the event data layout is identical, only the __KER_* numbering differs.
# ============================================================================

KERCALL_ENTER_FIELDS = {
    # __KER_NOP (0)
    0:  (['dummy', 'empty'], ['dummy', 'empty']),
    # __KER_TRACE_EVENT (1)
    1:  (['mode', 'class'], ['mode', 'class', 'event', 'data_1', 'data_2']),
    # __KER_RING0 (2)
    2:  (['func_p', 'arg_p'], ['func_p', 'arg_p']),
    # __KER_CACHE_CONTROL / CACHE_FLUSH (3)
    3:  (['addr', 'len'], ['addr', 'len', 'flags']),
    # __KER_MSG_REGISTER_EVENT (5)
    5:  (['empty', 'empty'], ['empty', 'empty']),
    # __KER_MSG_SEND_PULSEPTR (6)
    6:  (['coid', 'code'], ['coid', 'priority', 'code', 'value']),
    # __KER_SYS_CPUPAGE_GET (7)
    7:  (['index', 'empty'], ['index', 'empty']),
    # __KER_SYS_CPUPAGE_SET (8)
    8:  (['index', 'value'], ['index', 'value']),
    # __KER_MSG_PAUSE (9)
    9:  (['rcvid', 'cookie'], ['rcvid', 'cookie']),
    # __KER_MSG_CURRENT (10)
    10: (['rcvid', 'empty'], ['rcvid', 'empty']),
    # __KER_MSG_SENDV (11)
    11: (['coid', 'msg[0]'],
         ['coid', 'sparts', 'rparts', 'msg[0]', 'msg[1]', 'msg[2]']),
    # __KER_MSG_SENDVNC (12)
    12: (['coid', 'msg[0]'],
         ['coid', 'sparts', 'rparts', 'msg[0]', 'msg[1]', 'msg[2]']),
    # __KER_MSG_ERROR (13)
    13: (['rcvid', 'err'], ['rcvid', 'err']),
    # __KER_MSG_RECEIVEV (14)
    14: (['chid', 'rparts'], ['chid', 'rparts']),
    # __KER_MSG_REPLYV (15)
    15: (['rcvid', 'status'],
         ['rcvid', 'sparts', 'status', 'smsg[0]', 'smsg[1]', 'smsg[2]']),
    # __KER_MSG_READV (16)
    16: (['rcvid', 'offset'], ['rcvid', 'rmsg_p', 'rparts', 'offset']),
    # __KER_MSG_WRITEV (17)
    17: (['rcvid', 'offset'],
         ['rcvid', 'sparts', 'offset', 'msg[0]', 'msg[1]', 'msg[2]']),
    # __KER_MSG_INFO (19)
    19: (['rcvid', 'info_p'], ['rcvid', 'info_p']),
    # __KER_MSG_SEND_PULSE (20)
    20: (['coid', 'code'], ['coid', 'priority', 'code', 'value']),
    # __KER_MSG_DELIVER_EVENT (21)
    21: (['rcvid', 'sigev_notify'],
         ['rcvid', 'sigev_notify', 'sigev_func_p', 'sigev_value',
          'sigev_attr_p']),
    # __KER_MSG_RECEIVE_PULSEV (24) — same as MsgReceivev
    24: (['chid', 'rparts'], ['chid', 'rparts']),
    # __KER_MSG_VERIFY_EVENT (25)
    25: (['rcvid', 'sigev_notify'],
         ['rcvid', 'sigev_notify', 'sigev_func_p', 'sigev_value']),
    # __KER_SIGNAL_KILL (26)
    26: (['pid', 'signo'],
         ['nd', 'pid', 'tid', 'signo', 'code', 'value']),
    # __KER_SIGNAL_RETURN (27)
    27: (['signal_p', 'empty'], ['signal_p', 'empty']),
    # __KER_SIGNAL_ACTION (29)
    29: (['signo', 'handler_p'],
         ['pid', 'sigstub_p', 'signo', 'handler_p', 'sa_flags',
          'sa_mask_0', 'sa_mask_1']),
    # __KER_SIGNAL_PROCMASK (30)
    30: (['pid', 'tid'],
         ['pid', 'tid', 'how', 'sig_blocked_0', 'sig_blocked_1']),
    # __KER_SIGNAL_SUSPEND (31)
    31: (['sig_blocked_0', 'sig_blocked_1'],
         ['sig_blocked_0', 'sig_blocked_1']),
    # __KER_SIGNAL_WAITINFO (32)
    32: (['sig_wait_0', 'sig_wait_1'],
         ['sig_wait_0', 'sig_wait_1']),
    # __KER_SIGNAL_KILL_SIGVAL (33)
    33: (['pid', 'signo'],
         ['nd', 'pid', 'tid', 'signo', 'code', 'value']),
    # __KER_CHANNEL_CREATE (35)
    35: (['flags', 'empty'], ['flags', 'empty']),
    # __KER_CHANNEL_DESTROY (36)
    36: (['chid', 'empty'], ['chid', 'empty']),
    # __KER_CHANNEL_CTL (8.0) / CHANCON_ATTR (7.1) (37)
    37: (['chid', 'cmd'], ['chid', 'cmd', 'new_attr']),
    # __KER_CONNECT_ATTACH (39)
    39: (['empty', 'pid'], ['empty', 'pid', 'chid', 'index', 'flags']),
    # __KER_CONNECT_DETACH (40)
    40: (['coid', 'empty'], ['coid', 'empty']),
    # __KER_CONNECT_SERVER_INFO (41)
    41: (['pid', 'coid'], ['pid', 'coid']),
    # __KER_CONNECT_CLIENT_INFO (42)
    42: (['scoid', 'ngroups'], ['scoid', 'ngroups']),
    # __KER_CONNECT_FLAGS (43)
    43: (['coid', 'bits'], ['pid', 'coid', 'masks', 'bits']),
    # __KER_THREAD_CREATE (46)
    46: (['func_p', 'arg_p'],
         ['pid', 'func_p', 'arg_p', 'flags', 'stacksize', 'stackaddr_p',
          'exitfunc_p', 'policy', 'priority', 'curpriority',
          'ss_low_prio', 'ss_max_repl', 'ss_repl_sec', 'ss_repl_nsec',
          'ss_budget_sec', 'ss_budget_nsec', 'guardsize']),
    # __KER_THREAD_DESTROY (47)
    47: (['tid', 'status_p'], ['tid', 'priority', 'status_p']),
    # __KER_THREAD_DETACH (49)
    49: (['tid', 'empty'], ['tid', 'empty']),
    # __KER_THREAD_JOIN (50)
    50: (['tid', 'status_p'], ['tid', 'status_p']),
    # __KER_THREAD_CANCEL (51)
    51: (['tid', 'canstub_p'], ['tid', 'canstub_p']),
    # __KER_THREAD_CTL (52)
    52: (['cmd', 'data_p'], ['cmd', 'data_p']),
    # __KER_THREAD_CTLEXT (53)
    53: (['pid', 'tid'], ['pid', 'tid', 'cmd', 'data_p']),
    # __KER_INTERRUPT_ATTACH (55)
    55: (['intr', 'flags'],
         ['intr', 'handler_p', 'area_p', 'areasize', 'flags']),
    # __KER_INTERRUPT_QUERY (8.0) / DETACH_FUNC (7.1) (56)
    56: (['type_or_intr', 'id_or_handler_p'], ['type_or_intr', 'id_or_handler_p']),
    # __KER_INTERRUPT_DETACH (57)
    57: (['id', 'empty'], ['id', 'empty']),
    # __KER_INTERRUPT_WAIT (58)
    58: (['flags', 'timeout_sec'],
         ['flags', 'timeout_sec', 'timeout_nsec']),
    # __KER_INTERRUPT_MASK (59)
    59: (['intr', 'id'], ['intr', 'id']),
    # __KER_INTERRUPT_UNMASK (60)
    60: (['intr', 'id'], ['intr', 'id']),
    # __KER_CLOCK_TIME (65)
    65: (['id', 'new_sec'], ['id', 'new_sec', 'new_nsec']),
    # __KER_CLOCK_ADJUST (66)
    66: (['id', 'tick_count'], ['id', 'tick_count', 'tick_nsec_inc']),
    # __KER_CLOCK_PERIOD (67)
    67: (['id', 'new_nsec'], ['id', 'new_nsec', 'new_fract']),
    # __KER_CLOCK_ID (68)
    68: (['pid', 'tid'], ['pid', 'tid']),
    # __KER_TIMER_CREATE (70)
    70: (['timer_id', 'sigev_notify'],
         ['timer_id', 'sigev_notify', 'sigev_func_p', 'sigev_value',
          'sigev_attr_p']),
    # __KER_TIMER_DESTROY (71)
    71: (['id', 'empty'], ['id', 'empty']),
    # __KER_TIMER_SETTIME (72)
    72: (['clock_id', 'itime_nsec_sec'],
         ['clock_id', 'flags', 'itime_nsec_sec', 'itime_nsec_nsec',
          'itime_interval_sec', 'itime_interval_nsec']),
    # __KER_TIMER_INFO (73)
    73: (['pid', 'id'], ['pid', 'id', 'flags', 'info_p']),
    # __KER_TIMER_ALARM (74)
    74: (['clock_id', 'itime_sec'],
         ['clock_id', 'itime_sec', 'itime_nsec',
          'interval_sec', 'interval_nsec']),
    # __KER_TIMER_TIMEOUT (75)
    75: (['clock_id', 'timeout_flags'],
         ['clock_id', 'timeout_flags', 'ntime_sec', 'ntime_nsec',
          'sigev_notify', 'sigev_func_p', 'sigev_value', 'sigev_attr_p']),
    # __KER_SYNC_CREATE (78)
    78: (['type', 'sync_p'],
         ['type', 'sync_p', 'count', 'owner', 'protocol', 'flags',
          'prioceiling', 'clockid']),
    # __KER_SYNC_DESTROY (79)
    79: (['sync_p', 'owner'], ['sync_p', 'count', 'owner']),
    # __KER_SYNC_MUTEX_LOCK (80)
    80: (['sync_p', 'owner'], ['sync_p', 'count', 'owner']),
    # __KER_SYNC_MUTEX_UNLOCK (81)
    81: (['sync_p', 'owner'], ['sync_p', 'count', 'owner']),
    # __KER_SYNC_CONDVAR_WAIT (82)
    82: (['sync_p', 'mutex_p'],
         ['sync_p', 'mutex_p', 'sync_count', 'sync_owner',
          'mutex_count', 'mutex_owner']),
    # __KER_SYNC_CONDVAR_SIGNAL (83)
    83: (['sync_p', 'all'],
         ['sync_p', 'all', 'sync_count', 'sync_owner']),
    # __KER_SYNC_SEM_POST (84)
    84: (['sync_p', 'count'], ['sync_p', 'count', 'owner']),
    # __KER_SYNC_SEM_WAIT (85)
    85: (['sync_p', 'count'], ['sync_p', 'try', 'count', 'owner']),
    # __KER_SYNC_CTL (86)
    86: (['cmd', 'sync_p'], ['cmd', 'sync_p', 'data_p', 'count', 'owner']),
    # __KER_SCHED_GET (88)
    88: (['pid', 'tid'], ['pid', 'tid']),
    # __KER_SCHED_SET (89)
    89: (['pid', 'sched_priority'],
         ['pid', 'tid', 'policy', 'sched_priority', 'sched_curpriority',
          'ss_low_prio', 'ss_max_repl', 'ss_repl_sec', 'ss_repl_nsec',
          'ss_budget_sec', 'ss_budget_nsec']),
    # __KER_SCHED_YIELD (90)
    90: (['empty', 'empty'], ['empty', 'empty']),
    # __KER_SCHED_INFO (91)
    91: (['pid', 'policy'], ['pid', 'policy']),
    # __KER_SCHED_CTL (92)
    92: (['cmd', 'data_p'], ['cmd', 'data_p']),
    # __KER_TIMER_DELEGATE (76, QNX 8.0)
    76: (['action', 'delegate_cpu'], ['action', 'delegate_cpu']),
    # __KER_INTERRUPT_CHARACTERISTIC (61, QNX 7.1) / InterruptUnblock (8.0)
    61: (['id', 'type'], ['id', 'type', 'new']),
    # __KER_INTERRUPT_CTL (62, QNX 8.0)
    62: (['cmd', 'data_p'], ['cmd', 'data_p']),
    # __KER_SYNC_RWLOCK (77, QNX 8.0)
    77: (['sync_p_lo', 'sync_p_hi'],
         ['sync_p_lo', 'sync_p_hi', 'op', 'count', 'owner']),
    # __KER_SYNC_MUTEX_REVIVE (87, QNX 7.1) / SyncBarrierWait (8.0)
    87: (['sync_p', 'owner'], ['sync_p', 'count', 'owner']),
    # __KER_NET_CRED (93, QNX 7.1) / MsgQueueOpen (8.0)
    93: (['d1', 'd2'], ['d1', 'd2']),
    # __KER_NET_VTID (94, QNX 7.1) / MsgQueueSend (8.0)
    94: (['fd', 'msg[0]'], ['fd', 'msg[0]', 'msglen_lo', 'msglen_hi',
         'priority']),
    # __KER_NET_UNBLOCK (95, QNX 7.1) / MsgQueueReceive (8.0)
    95: (['fd', 'empty'], ['fd', 'msglen_lo', 'msglen_hi', 'msg_lo',
         'msg_hi', 'prio_lo', 'prio_hi']),
    # __KER_NET_INFOSCOID (96, QNX 7.1) / MsgQueueClose (8.0)
    96: (['rcvid_lo', 'rcvid_hi'], ['rcvid_lo', 'rcvid_hi', 'queueid']),
    # __KER_NET_SIGNAL_KILL (97, QNX 7.1) / MsgQueueCtl (8.0)
    97: (['fd', 'cmd'], ['fd', 'cmd']),
    # __KER_POWER_PARAMETER (100, QNX 7.1) / PipeClose (8.0)
    100: (['d1', 'd2'], ['d1', 'd2']),
    # __KER_SYNC_TYPE_DESTROY (104, QNX 8.0)
    104: (['sync_p', 'owner'], ['sync_p', 'count', 'owner']),
    # __KER_SYS_CUSTOM (106)
    106: (['empty', 'empty'], ['empty', 'empty']),
    # __KER_BAD (107)
    107: (['empty', 'empty'], ['empty', 'empty']),
}

KERCALL_EXIT_FIELDS = {
    0:  (['empty', 'empty'], ['empty', 'empty']),
    1:  (['ret_val', 'empty'], ['ret_val', 'empty']),
    2:  (['ret_val', 'empty'], ['ret_val', 'empty']),
    3:  (['ret_val', 'empty'], ['ret_val', 'empty']),
    5:  (['empty', 'empty'], ['empty', 'empty']),
    6:  (['status', 'empty'], ['status', 'empty']),
    7:  (['ret_val', 'empty'], ['ret_val', 'empty']),
    8:  (['ret_val', 'empty'], ['ret_val', 'empty']),
    9:  (['empty', 'empty'], ['empty', 'empty']),
    10: (['empty', 'empty'], ['empty', 'empty']),
    # __KER_MSG_SENDV EXIT
    11: (['status', 'rmsg[0]'],
         ['status', 'rmsg[0]', 'rmsg[1]', 'rmsg[2]']),
    # __KER_MSG_SENDVNC EXIT
    12: (['ret_val', 'rmsg[0]'],
         ['ret_val', 'rmsg[0]', 'rmsg[1]', 'rmsg[2]']),
    13: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_MSG_RECEIVEV EXIT
    14: (['rcvid', 'rmsg[0]'],
         ['rcvid', 'rmsg[0]', 'rmsg[1]', 'rmsg[2]', 'empty', 'empty',
          'info_pid', 'info_tid', 'info_chid', 'info_scoid', 'info_coid',
          'info_msglen', 'info_srcmsglen', 'info_dstmsglen',
          'info_priority', 'info_flags', 'info_reserved']),
    15: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_MSG_READV EXIT
    16: (['rbytes', 'rmsg[0]'],
         ['rbytes', 'rmsg[0]', 'rmsg[1]', 'rmsg[2]']),
    17: (['wbytes', 'empty'], ['wbytes', 'empty']),
    # __KER_MSG_INFO EXIT
    19: (['ret_val', 'empty'],
         ['ret_val', 'empty', 'empty', 'info_pid', 'info_tid', 'info_chid',
          'info_scoid', 'info_coid', 'info_msglen', 'info_srcmsglen',
          'info_dstmsglen', 'info_priority', 'info_flags',
          'info_reserved']),
    20: (['status', 'empty'], ['status', 'empty']),
    # __KER_MSG_DELIVER_EVENT EXIT
    21: (['ret_val', 'eventp'], ['ret_val', 'eventp']),
    # __KER_MSG_RECEIVE_PULSEV EXIT — same layout as MsgReceivev EXIT
    24: (['rcvid', 'rmsg[0]'],
         ['rcvid', 'rmsg[0]', 'rmsg[1]', 'rmsg[2]', 'empty', 'empty',
          'info_pid', 'info_tid', 'info_chid', 'info_scoid', 'info_coid',
          'info_msglen', 'info_srcmsglen', 'info_dstmsglen',
          'info_priority', 'info_flags', 'info_reserved']),
    26: (['ret_val', 'empty'], ['ret_val', 'empty']),
    27: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_SIGNAL_ACTION EXIT
    29: (['ret_val', 'handler_p'],
         ['ret_val', 'handler_p', 'sa_flags', 'sa_mask_0', 'sa_mask_1']),
    # __KER_SIGNAL_PROCMASK EXIT
    30: (['ret_val', 'sig_blocked_0'],
         ['ret_val', 'sig_blocked_0', 'sig_blocked_1']),
    31: (['ret_val', 'sig_blocked_p'], ['ret_val', 'sig_blocked_p']),
    # __KER_SIGNAL_WAITINFO EXIT
    32: (['sig_num', 'si_code'],
         ['sig_num', 'si_signo', 'si_code', 'si_errno',
          'p[0]', 'p[1]', 'p[2]', 'p[3]', 'p[4]', 'p[5]', 'p[6]']),
    33: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_CHANNEL_CREATE EXIT
    35: (['chid', 'empty'], ['chid', 'empty']),
    36: (['ret_val', 'empty'], ['ret_val', 'empty']),
    37: (['ret_val', 'empty'], ['ret_val', 'empty']),
    39: (['coid', 'empty'], ['coid', 'empty']),
    40: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_CONNECT_SERVER_INFO EXIT
    41: (['coid', 'empty'],
         ['coid', 'empty', 'empty', 'info_pid', 'info_tid', 'info_chid',
          'info_scoid', 'info_coid', 'info_msglen', 'info_srcmsglen',
          'info_dstmsglen', 'info_priority', 'info_flags',
          'info_reserved']),
    # __KER_CONNECT_CLIENT_INFO EXIT
    42: (['ret_val', 'empty'],
         ['ret_val', 'empty', 'info_pid', 'info_sid', 'flags',
          'info_ruid', 'info_euid', 'info_suid', 'info_rgid',
          'info_egid', 'info_sgid', 'info_ngroups',
          'group[0]', 'group[1]', 'group[2]', 'group[3]',
          'group[4]', 'group[5]', 'group[6]', 'group[7]']),
    43: (['old_flags', 'empty'], ['old_flags', 'empty']),
    # __KER_THREAD_CREATE EXIT
    46: (['thread_id', 'owner'], ['thread_id', 'owner']),
    47: (['ret_val', 'empty'], ['ret_val', 'empty']),
    49: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_THREAD_JOIN EXIT
    50: (['ret_val', 'status_p'], ['ret_val', 'status_p']),
    51: (['ret_val', 'empty'], ['ret_val', 'empty']),
    52: (['ret_val', 'empty'], ['ret_val', 'empty']),
    53: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_INTERRUPT_ATTACH EXIT
    55: (['int_fun_id', 'empty'], ['int_fun_id', 'empty']),
    56: (['ret_val', 'empty'], ['ret_val', 'empty']),
    57: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_INTERRUPT_WAIT EXIT
    58: (['ret_val', 'timeout_p'], ['ret_val', 'timeout_p']),
    59: (['mask_level', 'empty'], ['mask_level', 'empty']),
    60: (['mask_level', 'empty'], ['mask_level', 'empty']),
    # __KER_INTERRUPT_CHARACTERISTIC EXIT (61, QNX 7.1)
    61: (['ret_val', 'old'], ['ret_val', 'old']),
    # __KER_INTERRUPT_CTL EXIT (62, QNX 8.0)
    62: (['ret_val', 'empty'], ['ret_val', 'empty']),
    65: (['ret_val', 'old_sec'], ['ret_val', 'old_sec', 'old_nsec']),
    66: (['ret_val', 'old_tick_count'],
         ['ret_val', 'old_tick_count', 'old_tick_nsec_inc']),
    67: (['ret_val', 'old_nsec'], ['ret_val', 'old_nsec', 'old_fract']),
    68: (['ret_val', 'empty'], ['ret_val', 'empty']),
    70: (['timer_id', 'empty'], ['timer_id', 'empty']),
    71: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_TIMER_SETTIME EXIT
    72: (['ret_val', 'oitime_sec'],
         ['ret_val', 'oitime_sec', 'oitime_nsec',
          'oitime_interval_sec', 'oitime_interval_nsec']),
    # __KER_TIMER_INFO EXIT
    73: (['prev_id', 'itime_nsec'],
         ['prev_id', 'itime_nsec', 'itime_interval_nsec',
          'otime_nsec', 'otime_interval_nsec', 'flags', 'tid', 'notify',
          'clockid', 'overruns', 'sigev_notify', 'sigev_func_p',
          'sigev_value', 'sigev_attr_p']),
    # __KER_TIMER_ALARM EXIT
    74: (['ret_val', 'otime_sec'],
         ['ret_val', 'otime_sec', 'otime_nsec',
          'interval_sec', 'interval_nsec']),
    # __KER_TIMER_TIMEOUT EXIT
    75: (['prev_timeout_flags', 'otime_sec'],
         ['prev_timeout_flags', 'otime_sec', 'otime_nsec']),
    76: (['ret_val', 'empty'], ['ret_val', 'empty']),
    78: (['ret_val', 'empty'], ['ret_val', 'empty']),
    79: (['ret_val', 'empty'], ['ret_val', 'empty']),
    80: (['ret_val', 'empty'], ['ret_val', 'empty']),
    81: (['ret_val', 'empty'], ['ret_val', 'empty']),
    82: (['ret_val', 'empty'], ['ret_val', 'empty']),
    83: (['ret_val', 'empty'], ['ret_val', 'empty']),
    84: (['ret_val', 'empty'], ['ret_val', 'empty']),
    85: (['ret_val', 'empty'], ['ret_val', 'empty']),
    86: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_SCHED_GET EXIT
    88: (['ret_val', 'sched_priority'],
         ['ret_val', 'sched_priority', 'sched_curpriority',
          'ss_low_prio', 'ss_max_repl', 'ss_repl_sec', 'ss_repl_nsec',
          'ss_budget_sec', 'ss_budget_nsec']),
    89: (['ret_val', 'empty'], ['ret_val', 'empty']),
    90: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_SCHED_INFO EXIT
    91: (['ret_val', 'priority_max'],
         ['ret_val', 'priority_min', 'priority_max',
          'interval_sec', 'interval_nsec', 'priority_priv']),
    92: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_SYNC_RWLOCK EXIT (77, QNX 8.0)
    77: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_SYNC_MUTEX_REVIVE (87, 7.1) / SyncBarrierWait (87, 8.0) EXIT
    87: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # MsgQueueOpen EXIT (93, 8.0) / NetCred (7.1)
    93: (['ret_val', 'empty'], ['ret_val', 'empty']),
    94: (['ret_val', 'empty'], ['ret_val', 'empty']),
    95: (['msglen', 'msg[0]'], ['msglen', 'msg[0]', 'priority']),
    96: (['ret_val', 'empty'], ['ret_val', 'empty']),
    97: (['ret_val', 'cmd'], ['ret_val', 'cmd']),
    100: (['ret_val', 'empty'], ['ret_val', 'empty']),
    # __KER_SYNC_TYPE_DESTROY EXIT (104, 8.0)
    104: (['ret_val', 'empty'], ['ret_val', 'empty']),
    106: (['ret_val', 'empty'], ['ret_val', 'empty']),
    107: (['ret_val', 'empty'], ['ret_val', 'empty']),
}


# ============================================================================
# 64-bit wide-mode field layouts (_NTO_TRACE_KERCALL64)
#
# Fields annotated "(64)" in the SAT docs occupy 2 data words (lo, hi).
# Only events that differ from the 32-bit layout are listed here;
# events not present fall back to the standard wide layout.
# ============================================================================

_KERCALL_ENTER_WIDE64 = {
    # CacheFlush: addr (64), nlines (64), flags, index
    3:  ['addr_lo', 'addr_hi', 'nlines_lo', 'nlines_hi', 'flags', 'index'],
    # MsgSendPulsePtr: coid, priority, code, value (64)
    6:  ['coid', 'priority', 'code', 'value_lo', 'value_hi'],
    # MsgSendv: coid, sparts (64), rparts (64), msg[0], msg[1], msg[2]
    11: ['coid', 'sparts_lo', 'sparts_hi', 'rparts_lo', 'rparts_hi',
         'msg[0]', 'msg[1]', 'msg[2]'],
    # MsgSendvnc: same as MsgSendv
    12: ['coid', 'sparts_lo', 'sparts_hi', 'rparts_lo', 'rparts_hi',
         'msg[0]', 'msg[1]', 'msg[2]'],
    # MsgReceivev: chid, rparts (64)
    14: ['chid', 'rparts_lo', 'rparts_hi'],
    # MsgReplyv: rcvid, sparts (64), status (64), smsg[0], smsg[1], smsg[2]
    15: ['rcvid', 'sparts_lo', 'sparts_hi', 'status_lo', 'status_hi',
         'smsg[0]', 'smsg[1]', 'smsg[2]'],
    # MsgReadv: rcvid, rmsg_p (64), rparts (64), offset (64)
    16: ['rcvid', 'rmsg_p_lo', 'rmsg_p_hi', 'rparts_lo', 'rparts_hi',
         'offset_lo', 'offset_hi'],
    # MsgWritev: rcvid, sparts (64), offset (64), msg[0], msg[1], msg[2]
    17: ['rcvid', 'sparts_lo', 'sparts_hi', 'offset_lo', 'offset_hi',
         'msg[0]', 'msg[1]', 'msg[2]'],
    # MsgSendPulse: coid, priority, code, value (64) — same as MsgSendPulsePtr
    20: ['coid', 'priority', 'code', 'value_lo', 'value_hi'],
    # MsgDeliverEvent: rcvid, sigev_notify, pad, sigev_func_p (64),
    #   sigev_value (64), sigev_attr_p (64)
    21: ['rcvid', 'sigev_notify', 'pad', 'sigev_func_p_lo', 'sigev_func_p_hi',
         'sigev_value_lo', 'sigev_value_hi',
         'sigev_attr_p_lo', 'sigev_attr_p_hi'],
    # MsgReceivePulsev: same as MsgReceivev
    24: ['chid', 'rparts_lo', 'rparts_hi'],
    # SignalKill: nd, pid, tid, signo, code, value (64)
    26: ['nd', 'pid', 'tid', 'signo', 'code', 'value_lo', 'value_hi'],
    # SignalReturn: s_p (64)
    27: ['signal_p_lo', 'signal_p_hi'],
    # SignalAction: pid, sigstub_p (64), signo, handler_p (64),
    #   sa_flags, sa_mask_0, sa_mask_1
    29: ['pid', 'sigstub_p_lo', 'sigstub_p_hi', 'signo',
         'handler_p_lo', 'handler_p_hi', 'sa_flags', 'sa_mask_0', 'sa_mask_1'],
    # SignalKillSigval: same as SignalKill 64
    33: ['nd', 'pid', 'tid', 'signo', 'code', 'value_lo', 'value_hi'],
    # ThreadCreate: pid, func_p (64), arg_p (64), flags, stacksize,
    #   stackaddr_p (64), exitfunc_p (64), policy, priority, curpriority,
    #   ss_low_prio, ss_max_repl, ss_repl_sec, ss_repl_nsec,
    #   ss_budget_sec, ss_budget_nsec, guardsize
    46: ['pid', 'func_p_lo', 'func_p_hi', 'arg_p_lo', 'arg_p_hi',
         'flags', 'stacksize',
         'stackaddr_p_lo', 'stackaddr_p_hi',
         'exitfunc_p_lo', 'exitfunc_p_hi',
         'policy', 'priority', 'curpriority',
         'ss_low_prio', 'ss_max_repl',
         'ss_repl_sec', 'ss_repl_nsec',
         'ss_budget_sec', 'ss_budget_nsec', 'guardsize'],
    # ThreadDestroy: tid, priority, status_p (64)
    47: ['tid', 'priority', 'status_p_lo', 'status_p_hi'],
    # ThreadCancel: tid, canstub_p (64)
    51: ['tid', 'canstub_p_lo', 'canstub_p_hi'],
    # ThreadCtl: cmd, data_p (64)
    52: ['cmd', 'data_p_lo', 'data_p_hi'],
    # ThreadCtlExt: pid, tid, cmd, data_p (64)
    53: ['pid', 'tid', 'cmd', 'data_p_lo', 'data_p_hi'],
    # InterruptAttach: intr, handler_p (64), area_p (64), areasize, flags
    55: ['intr', 'handler_p_lo', 'handler_p_hi',
         'area_p_lo', 'area_p_hi', 'areasize', 'flags'],
    # TimerCreate: timer_id, sigev_notify, pad, sigev_func_p (64),
    #   sigev_value (64), sigev_attr_p (64)
    70: ['timer_id', 'sigev_notify', 'pad',
         'sigev_func_p_lo', 'sigev_func_p_hi',
         'sigev_value_lo', 'sigev_value_hi',
         'sigev_attr_p_lo', 'sigev_attr_p_hi'],
    # TimerInfo: pid, id, flags, info_p (64)
    73: ['pid', 'id', 'flags', 'info_p_lo', 'info_p_hi'],
    # TimerTimeout: clock_id, timeout_flags, ntime_sec, ntime_nsec,
    #   sigev_notify, pad, sigev_func_p (64), sigev_value (64), sigev_attr_p (64)
    75: ['clock_id', 'timeout_flags', 'ntime_sec', 'ntime_nsec',
         'sigev_notify', 'pad', 'sigev_func_p_lo', 'sigev_func_p_hi',
         'sigev_value_lo', 'sigev_value_hi',
         'sigev_attr_p_lo', 'sigev_attr_p_hi'],
    # SyncCreate: type, sync_p (64), count, owner, protocol, flags,
    #   prioceiling, clockid
    78: ['type', 'sync_p_lo', 'sync_p_hi', 'count', 'owner',
         'protocol', 'flags', 'prioceiling', 'clockid'],
    # SyncDestroy: sync_p (64), count, owner
    79: ['sync_p_lo', 'sync_p_hi', 'count', 'owner'],
    # SyncMutexLock: sync_p (64), count, owner
    80: ['sync_p_lo', 'sync_p_hi', 'count', 'owner'],
    # SyncMutexUnlock: sync_p (64), count, owner
    81: ['sync_p_lo', 'sync_p_hi', 'count', 'owner'],
    # SyncCondvarWait: sync_p (64), mutex_p (64), sync_count, sync_owner,
    #   mutex_count, mutex_owner
    82: ['sync_p_lo', 'sync_p_hi', 'mutex_p_lo', 'mutex_p_hi',
         'sync_count', 'sync_owner', 'mutex_count', 'mutex_owner'],
    # SyncCondvarSignal: sync_p (64), all, sync_count, sync_owner
    83: ['sync_p_lo', 'sync_p_hi', 'all', 'sync_count', 'sync_owner'],
    # SyncSemPost: sync_p (64), count, owner
    84: ['sync_p_lo', 'sync_p_hi', 'count', 'owner'],
    # SyncSemWait: sync_p (64), try, count, owner
    85: ['sync_p_lo', 'sync_p_hi', 'try', 'count', 'owner'],
    # SyncCtl: cmd, sync_p (64), data_p (64), count, owner
    86: ['cmd', 'sync_p_lo', 'sync_p_hi', 'data_p_lo', 'data_p_hi',
         'count', 'owner'],
    # SchedCtl: cmd, data_p (64)
    92: ['cmd', 'data_p_lo', 'data_p_hi'],
}

_KERCALL_EXIT_WIDE64 = {
    # MsgSendv EXIT: status (64), rmsg[0], rmsg[1], rmsg[2]
    11: ['status_lo', 'status_hi', 'rmsg[0]', 'rmsg[1]', 'rmsg[2]'],
    # MsgSendvnc EXIT: ret_val (64), rmsg[0], rmsg[1], rmsg[2]
    12: ['ret_val_lo', 'ret_val_hi', 'rmsg[0]', 'rmsg[1]', 'rmsg[2]'],
    # MsgReceivev EXIT (64): rcvid, rmsg[0..2], nd, srcnd, pid, tid, chid,
    #   scoid, coid, msglen (64), srcmsglen (64), dstmsglen (64),
    #   priority, flags, reserved[0], reserved[1]
    14: ['rcvid', 'rmsg[0]', 'rmsg[1]', 'rmsg[2]',
         'info_nd', 'info_srcnd', 'info_pid', 'info_tid',
         'info_chid', 'info_scoid', 'info_coid',
         'info_msglen_lo', 'info_msglen_hi',
         'info_srcmsglen_lo', 'info_srcmsglen_hi',
         'info_dstmsglen_lo', 'info_dstmsglen_hi',
         'info_priority', 'info_flags',
         'info_reserved_0', 'info_reserved_1'],
    # MsgReadv EXIT: rbytes (64), rmsg[0], rmsg[1], rmsg[2]
    16: ['rbytes_lo', 'rbytes_hi', 'rmsg[0]', 'rmsg[1]', 'rmsg[2]'],
    # MsgWritev EXIT: wbytes (64)
    17: ['wbytes_lo', 'wbytes_hi'],
    # MsgDeliverEvent EXIT: ret_val, eventp (64)
    21: ['ret_val', 'eventp_lo', 'eventp_hi'],
    # MsgReceivePulsev EXIT (64): same as MsgReceivev EXIT (64)
    24: ['rcvid', 'rmsg[0]', 'rmsg[1]', 'rmsg[2]',
         'info_nd', 'info_srcnd', 'info_pid', 'info_tid',
         'info_chid', 'info_scoid', 'info_coid',
         'info_msglen_lo', 'info_msglen_hi',
         'info_srcmsglen_lo', 'info_srcmsglen_hi',
         'info_dstmsglen_lo', 'info_dstmsglen_hi',
         'info_priority', 'info_flags',
         'info_reserved_0', 'info_reserved_1'],
    # SignalAction EXIT: ret_val, handler_p (64), sa_flags, sa_mask_0, sa_mask_1
    29: ['ret_val', 'handler_p_lo', 'handler_p_hi',
         'sa_flags', 'sa_mask_0', 'sa_mask_1'],
    # ThreadJoin EXIT: ret_val, status_p (64)
    50: ['ret_val', 'status_p_lo', 'status_p_hi'],
    # InterruptWait EXIT: ret_val, timeout_p (64)
    58: ['ret_val', 'timeout_p_lo', 'timeout_p_hi'],
}


def get_kercall_fields(call_num: int, is_enter: bool,
                       is_64: bool = False) -> tuple:
    """Return (fast_fields, wide_fields) for a kercall, or generic fallback.

    When is_64 is True, returns the 64-bit wide-mode field layout as the
    wide_fields element (fields like ``sync_p (64)`` expand to two words:
    ``sync_p_lo, sync_p_hi``).
    """
    if is_64:
        tbl64 = _KERCALL_ENTER_WIDE64 if is_enter else _KERCALL_EXIT_WIDE64
        fields_64 = tbl64.get(call_num)
        if fields_64 is not None:
            table = KERCALL_ENTER_FIELDS if is_enter else KERCALL_EXIT_FIELDS
            fast_f, _ = table.get(call_num, (['d1', 'd2'], ['d1', 'd2']))
            return (fast_f, fields_64)
    table = KERCALL_ENTER_FIELDS if is_enter else KERCALL_EXIT_FIELDS
    return table.get(call_num, (['d1', 'd2'], ['d1', 'd2']))


# ============================================================================
# COMM wide-mode field layouts  (from SAT kercall_table_Events.html)
# Most COMM events have the same fields in fast and wide mode.
# Exception: SIGNAL (9) has extra fields in wide mode.
# ============================================================================

COMM_WIDE_FIELDS = {
    0: ['target_rcvid', 'target_pid'],
    1: ['target_scoid', 'target_pid'],
    2: ['target_rcvid', 'target_pid'],
    3: ['target_scoid', 'target_pid'],
    4: ['target_scoid', 'target_pid'],
    5: ['target_scoid', 'target_pid'],
    6: ['target_scoid', 'target_pid'],
    7: ['target_scoid', 'target_pid'],
    8: ['target_scoid', 'target_pid'],
    9: ['si_signo', 'si_code', 'si_errno',
        'pad[0]', 'pad[1]', 'pad[2]', 'pad[3]', 'pad[4]', 'pad[5]'],
    10: ['target_tid', 'target_pid'],
    11: ['target_tid', 'target_pid'],
}


# ============================================================================
# SYSTEM wide-mode field layouts  (from SAT kercall_table_Events.html)
# ============================================================================

SYSTEM_FIELDS = {
    # PATHMGR: pathname operations (combine with string data)
    0x02: (['pid', 'tid'],
           ['pid', 'tid']),
    # APS_NAME: partition name
    0x03: (['partition_id'],
           ['partition_id']),
    # APS_BUDGETS: partition budgets
    0x04: (['partition_id', 'cpu_budget_pct', 'critical_budget_ms',
            'max_cpu_budget', 'critical_priority', 'budget_pct_scale'],
           ['partition_id', 'cpu_budget_pct', 'critical_budget_ms',
            'max_cpu_budget', 'critical_priority', 'budget_pct_scale']),
    # APS_BANKRUPTCY
    0x05: (['suspect_pid', 'suspect_tid', 'partition_id'],
           ['suspect_pid', 'suspect_tid', 'partition_id']),
    # MMAP
    0x06: (['pid', 'addr_lo', 'addr_hi', 'len_lo', 'len_hi', 'flags'],
           ['pid', 'addr_lo', 'addr_hi', 'len_lo', 'len_hi', 'flags',
            'prot', 'fd', 'align_lo', 'align_hi',
            'offset_lo', 'offset_hi']),
    # MUNMAP
    0x07: (['pid', 'addr_lo', 'addr_hi', 'len_lo', 'len_hi'],
           ['pid', 'addr_lo', 'addr_hi', 'len_lo', 'len_hi']),
    # MAPNAME: dlopen (32-bit)
    0x08: (['pid', 'addr', 'len'],
           ['pid', 'addr', 'len']),
    # ADDRESS: breakpoint
    0x09: (['addr', 'empty'],
           ['addr', 'empty']),
    # FUNC_ENTER
    0x0a: (['thisfn', 'call_site'],
           ['thisfn', 'call_site']),
    # FUNC_EXIT
    0x0b: (['thisfn', 'call_site'],
           ['thisfn', 'call_site']),
    # SLOG
    0x0c: (['opcode', 'severity'],
           ['opcode', 'severity']),
    # DEFRAG_START
    0x0d: (['d1', 'd2'],
           ['d1', 'd2']),
    # RUNSTATE
    0x0e: (['bitset'],
           ['bitset']),
    # POWER
    0x0f: (['bitset', 'mode'],
           ['bitset', 'mode']),
    # IPI / IPI_64
    0x10: (['ipicmd', 'pad', 'ip_lo', 'ip_hi', 'tid', 'pid'],
           ['ipicmd', 'pad', 'ip_lo', 'ip_hi', 'tid', 'pid']),
    # PAGEWAIT
    0x11: (['pid', 'tid', 'ip', 'vaddr'],
           ['pid', 'tid', 'ip', 'vaddr', 'fault_type', 'mmap_flags',
            'obj_offset_lo', 'obj_offset_hi']),
    # TIMER
    0x12: (['pid', 'tid', 'timer_id', 'flags'],
           ['pid', 'tid', 'timer_id', 'flags']),
    # DEFRAG_END
    0x13: (['rc', 'freemem', 'maxblock'],
           ['rc', 'freemem', 'maxblock']),
    # PROFILE
    0x14: (['ip', 'tid', 'pid'],
           ['ip', 'tid', 'pid']),
    # MAPNAME_64: dlopen (64-bit)
    0x15: (['pid', 'addr_lo', 'addr_hi', 'len_lo', 'len_hi'],
           ['pid', 'addr_lo', 'addr_hi', 'len_lo', 'len_hi']),
    # APS_PSTATS
    0x16: (['partition_id', 'flags'],
           ['partition_id', 'flags']),
    # APS_OSTATS
    0x17: (['bnkr_pid', 'bnkr_tid', 'bnkr_partition_id'],
           ['bnkr_pid', 'bnkr_tid', 'bnkr_partition_id']),
    # APS_INFO
    0x18: (['window_size_ms', 'sched_flags', 'sec_flags', 'bnkr_flags',
            'num_partitions', 'max_partitions'],
           ['window_size_ms', 'sched_flags', 'sec_flags', 'bnkr_flags',
            'num_partitions', 'max_partitions']),
    # APS_JOIN
    0x19: (['partition_id', 'pid', 'tid', 'aid'],
           ['partition_id', 'pid', 'tid', 'aid']),
    # APS_THREAD
    0x1a: (['pid', 'tid', 'orig_partition', 'inherited_partition', 'flags'],
           ['pid', 'tid', 'orig_partition', 'inherited_partition', 'flags']),
    # APS_PROCESS
    0x1b: (['pid', 'partition_id'],
           ['pid', 'partition_id']),
    # SCHED_CONF
    0x1c: (['old_low_latency', 'old_migrate', 'new_low_latency',
            'new_migrate'],
           ['old_low_latency', 'old_migrate', 'new_low_latency',
            'new_migrate']),
    # IST_ATTACH (QNX 8.0)
    0x1d: (['pid', 'tid', 'intr_id', 'intr_cpu', 'flags', 'ist_prio'],
           ['pid', 'tid', 'intr_id', 'intr_cpu', 'flags', 'ist_prio']),
    # IST_DETACH (QNX 8.0)
    0x1e: (['pid', 'tid', 'intr_id', 'intr_cpu', 'flags', 'ist_prio'],
           ['pid', 'tid', 'intr_id', 'intr_cpu', 'flags', 'ist_prio']),
}

# ---------------------------------------------------------------------------
# QNX Resource Manager IO Message Type Decoding (sys/iomsg.h)
# ---------------------------------------------------------------------------

IO_MSG_TYPES = {
    0x100: '_IO_CONNECT',
    0x101: '_IO_READ',
    0x102: '_IO_WRITE',
    0x103: '_IO_RSVD_CLOSE_OCB',
    0x104: '_IO_STAT',
    0x105: '_IO_NOTIFY',
    0x106: '_IO_DEVCTL',
    0x107: '_IO_RSVD_UNBLOCK',
    0x108: '_IO_PATHCONF',
    0x109: '_IO_LSEEK',
    0x10A: '_IO_CHMOD',
    0x10B: '_IO_CHOWN',
    0x10C: '_IO_UTIME',
    0x10D: '_IO_OPENFD',
    0x10E: '_IO_FDINFO',
    0x10F: '_IO_LOCK',
    0x110: '_IO_SPACE',
    0x111: '_IO_SHUTDOWN',
    0x112: '_IO_MMAP',
    0x113: '_IO_MSG',
    0x115: '_IO_DUP',
    0x116: '_IO_CLOSE',
    0x117: '_IO_RSVD_LOCK_OCB',
    0x118: '_IO_RSVD_UNLOCK_OCB',
    0x119: '_IO_SYNC',
    0x11A: '_IO_POWER',
    0x11B: '_IO_ACL',
    0x11C: '_IO_RSVD_PAUSE',
    0x11D: '_IO_RSVD_UNPAUSE',
    0x11E: '_IO_READ64',
    0x11F: '_IO_WRITE64',
    0x120: '_IO_NOTIFY64',
    0x121: '_IO_UTIME64',
}

_IO_CONNECT_SUBTYPES = {
    0: 'COMBINE', 1: 'COMBINE_CLOSE', 2: 'OPEN', 3: 'UNLINK',
    4: 'RENAME', 5: 'MKNOD', 6: 'READLINK', 7: 'LINK',
    8: 'RSVD_UNBLOCK', 9: 'MOUNT',
}

_IO_XTYPE_NAMES = {
    0x0000: 'none', 0x0001: 'readcond', 0x0002: 'offset',
    0x0003: 'registry',
}

_IO_NOTIFY_ACTIONS = {
    0: 'POLL', 1: 'POLLARM', 2: 'TRANARM', 3: 'POLLARMED',
}

_IO_NOTIFY_COND_FLAGS = [
    (0x00000001, 'INPUT'), (0x00000002, 'OUTPUT'), (0x00000004, 'OBAND'),
    (0x10000000, 'EXTEN'),
]

_IO_FTYPE_NAMES = {
    0: 'ANY', 1: 'FILE', 2: 'SOCKET', 3: 'PIPE', 4: 'SHMEM',
    5: 'MQUEUE', 6: 'TYPED_MEM', 7: 'TYMEM', 10: 'NAME',
}

_IO_SYNC_FLAGS = [
    (0x0010, 'DSYNC'), (0x0020, 'SYNC'), (0x0040, 'RSYNC'),
]

_IO_LSEEK_WHENCE = {0: 'SET', 1: 'CUR', 2: 'END'}

_IO_MSG_FIELD_DEFS: dict = {
    '_IO_READ':    [('nbytes', None), ('xtype', 'xtype')],
    '_IO_READ64':  [('nbytes', None), ('xtype', 'xtype')],
    '_IO_WRITE':   [('nbytes', None), ('xtype', 'xtype')],
    '_IO_WRITE64': [('nbytes', None), ('xtype', 'xtype')],
    '_IO_STAT':    [('zero', None)],
    '_IO_DEVCTL':  [('dcmd', 'hex'), ('nbytes', None)],
    '_IO_NOTIFY':  [('action', 'notify_action'), ('flags', 'notify_flags')],
    '_IO_NOTIFY64': [('action', 'notify_action'), ('flags', 'notify_flags')],
    '_IO_LSEEK':   [('whence_flags', 'lseek_whence'), ('offset_lo', None)],
    '_IO_CLOSE':   [],
    '_IO_SHUTDOWN': [],
    '_IO_SYNC':    [('flag', 'sync_flags')],
    '_IO_MMAP':    [('prot', 'hex'), ('offset_lo', None)],
    '_IO_MSG':     [('mgrid_subtype', 'msg_mgr')],
    '_IO_CHMOD':   [('mode', 'oct')],
    '_IO_CHOWN':   [('uid', None), ('gid', None)],
    '_IO_OPENFD':  [('sflag', 'hex'), ('xtype', 'xtype')],
    '_IO_FDINFO':  [('flags', 'hex'), ('path_len', None)],
    '_IO_LOCK':    [('subtype', None), ('nbytes', None)],
    '_IO_SPACE':   [('subtype_whence', 'space_sub'), ('start_lo', None)],
    '_IO_PATHCONF': [('name', None)],
    '_IO_POWER':   [('subtype', None), ('mode', None)],
    '_IO_ACL':     [('subtype', None), ('nentries', None)],
    '_IO_DUP':     [('info', 'hex')],
    '_IO_UTIME':   [],
    '_IO_UTIME64': [],
    '_IO_CONNECT': [('file_type', 'ftype'), ('reply_entry_max', 'connect_re')],
}


def _format_io_field(val: int, fmt) -> str:
    if fmt == 'hex':
        return f'0x{val:x}'
    if fmt == 'oct':
        return f'0{val:o}'
    if fmt == 'xtype':
        name = _IO_XTYPE_NAMES.get(val & 0xFFFF, f'0x{val:x}')
        return f'{name}({val})'
    if fmt == 'notify_action':
        name = _IO_NOTIFY_ACTIONS.get(val, str(val))
        return f'{name}({val})'
    if fmt == 'notify_flags':
        parts = []
        for mask, name in _IO_NOTIFY_COND_FLAGS:
            if val & mask:
                parts.append(name)
        label = '|'.join(parts) if parts else '0'
        return f'{label}(0x{val:x})'
    if fmt == 'sync_flags':
        parts = []
        for mask, name in _IO_SYNC_FLAGS:
            if val & mask:
                parts.append(name)
        label = '|'.join(parts) if parts else '0'
        return f'{label}(0x{val:x})'
    if fmt == 'lseek_whence':
        whence = val & 0xFFFF
        flags = (val >> 16) & 0xFFFF
        wname = _IO_LSEEK_WHENCE.get(whence, str(whence))
        return f'whence={wname} flags=0x{flags:x}'
    if fmt == 'ftype':
        name = _IO_FTYPE_NAMES.get(val, str(val))
        return f'{name}({val})'
    if fmt == 'msg_mgr':
        mgrid = val & 0xFFFF
        subtype = (val >> 16) & 0xFFFF
        return f'mgrid={mgrid} subtype={subtype}'
    if fmt == 'space_sub':
        subtype = val & 0xFFFF
        whence = (val >> 16) & 0xFFFF
        wname = _IO_LSEEK_WHENCE.get(whence, str(whence))
        return f'subtype={subtype} whence={wname}'
    if fmt == 'connect_re':
        reply_max = val & 0xFFFF
        entry_max = (val >> 16) & 0xFFFF
        return f'reply_max={reply_max} entry_max={entry_max}'
    return str(val)


_IO_COMBINE_FLAG = 0x8000


def decode_io_msg_fields(args: dict, prefix: str) -> None:
    """Decode QNX IO message header in rmsg[0]/msg[0]/smsg[0].

    Modifies *args* in-place: replaces raw word values with named fields.
    Only decodes if the type falls in the standard IO range (0x100-0x1FF).
    """
    w0_key = f'{prefix}[0]'
    w0 = args.get(w0_key)
    if not isinstance(w0, int):
        return

    msg_type = w0 & 0xFFFF
    if not (0x100 <= msg_type <= 0x1FF):
        return

    type_name = IO_MSG_TYPES.get(msg_type, f'_IO_0x{msg_type:x}')
    args[w0_key] = f'{type_name}({msg_type})'

    if msg_type == 0x100:
        subtype = (w0 >> 16) & 0xFFFF
        sub_name = _IO_CONNECT_SUBTYPES.get(subtype, str(subtype))
        args['connect_subtype'] = sub_name
    else:
        combine_raw = (w0 >> 16) & 0xFFFF
        combine_len = combine_raw & ~_IO_COMBINE_FLAG
        args['combine_len'] = combine_len

    field_defs = _IO_MSG_FIELD_DEFS.get(type_name)
    if not field_defs:
        return

    for i, (fname, fmt) in enumerate(field_defs):
        wkey = f'{prefix}[{i + 1}]'
        val = args.get(wkey)
        if val is None or not isinstance(val, int):
            break
        del args[wkey]
        args[fname] = _format_io_field(val, fmt) if fmt else val


# --------------------------------------------------------------------------
# Sync object field decode helpers  (sys/target_nto.h)
# --------------------------------------------------------------------------

_SYNC_OWNER_TYPES = {
    0x00000000: 'MUTEX_FREE',
    0xFFFFFFFF: 'INITIALIZER',
    0xFFFFFFFE: 'DESTROYED',
    0xFFFFFFFD: 'NAMED_SEM',
    0xFFFFFFFC: 'SEM',
    0xFFFFFFFB: 'COND',
    0xFFFFFFFA: 'SPIN',
    0xFFFFFFF9: 'JOB',
    0xFFFFFFF8: 'BARRIER',
    0xFFFFFFF7: 'RWLOCK',
    0xFFFFFF00: 'OWNERDEAD',
}

_NTO_SYNC_WAITING    = 0x80000000
_NTO_SYNC_OWNER_MASK = 0x7FFFFFFF
_NTO_SYNC_COUNTMASK  = 0x00FFFFFF

_SYNC_COUNT_FLAGS = [
    (0x80000000, 'NONRECURSIVE'),
    (0x40000000, 'SHARED'),
    (0x20000000, 'PRIOCEILING'),
    (0x10000000, 'PRIONONE'),
    (0x08000000, 'WAKEUP'),
    (0x04000000, 'KFORCE'),
    (0x02000000, 'ROBUST'),
    (0x01000000, 'NOERRORCHECK'),
]

_SYNC_KERCALL_NUMS = frozenset({78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 77})


def format_sync_owner(val: int) -> str:
    """Decode sync_t __owner field to human-readable string."""
    special = _SYNC_OWNER_TYPES.get(val)
    if special:
        return special
    waiting = val & _NTO_SYNC_WAITING
    vtid = val & _NTO_SYNC_OWNER_MASK
    if waiting:
        return f'WAITING|0x{vtid:x}'
    return f'0x{val:x}'


def format_sync_count(val: int) -> str:
    """Decode sync_t __count field to 'hex (count_n=N flags)' string."""
    count_n = val & _NTO_SYNC_COUNTMASK
    flags = []
    for mask, name in _SYNC_COUNT_FLAGS:
        if val & mask:
            flags.append(name)
    if flags:
        return f'0x{val:x} (count_n={count_n} {"|".join(flags)})'
    return str(count_n)


# Fields that should be displayed in hex (pointers, bitfields, IDs)
_HEX_FIELDS = frozenset({
    'nd', 'coid', 'rcvid', 'scoid',
    'target_rcvid', 'target_scoid',
    'flags', 'sa_flags', 'sa_mask_0', 'sa_mask_1',
    'sig_blocked_0', 'sig_blocked_1',
    'sig_wait_0', 'sig_wait_1',
    'masks', 'bits', 'new_attr',
    'code', 'value', 'status', 'offset',
    'signal_p',
})

_HEX_FIELD_PREFIXES = (
    'sync_p', 'mutex_p', 'handler_p', 'func_p', 'data_p',
    'area_p', 'addr', 'sigstub_p', 'signal_p', 'status_p',
    'canstub_p', 'stackaddr_p', 'exitfunc_p', 'eventp',
    'rmsg_p',
)


def _should_hex(key: str) -> bool:
    """Check if a field name should be displayed in hex."""
    if key in _HEX_FIELDS:
        return True
    if key.endswith('_ptr'):
        return True
    kl = key.lower()
    for prefix in _HEX_FIELD_PREFIXES:
        if kl.startswith(prefix):
            return True
    if kl.startswith('msg[') or kl.startswith('rmsg[') or kl.startswith('smsg['):
        return True
    return False


def format_kercall_args(args: dict, call_num: int) -> None:
    """Post-process kercall args: combine lo/hi pointers, hex format, sync decode.

    Modifies *args* in-place.
    """
    _combine_lo_hi(args)

    is_sync = call_num in _SYNC_KERCALL_NUMS
    for key in list(args.keys()):
        val = args[key]
        if not isinstance(val, int):
            continue

        if is_sync and key in ('count', 'mutex_count', 'sync_count'):
            args[key] = format_sync_count(val)
        elif is_sync and key in ('owner', 'mutex_owner', 'sync_owner'):
            args[key] = format_sync_owner(val)
        elif _should_hex(key):
            args[key] = f'0x{val:x}'

    for key in list(args.keys()):
        if key[0] == 'd' and key[1:].isdigit() and args[key] == 0:
            del args[key]


def format_general_args(args: dict) -> None:
    """Format hex fields and combine lo/hi pairs for any event type."""
    _combine_lo_hi(args)
    for key in list(args.keys()):
        val = args[key]
        if isinstance(val, int) and _should_hex(key):
            args[key] = f'0x{val:x}'


def _combine_lo_hi(args: dict) -> None:
    """Merge foo_lo + foo_hi pairs into a single integer field.

    Renames ``*_p_lo/*_p_hi`` → ``*_ptr``, other ``*_lo/*_hi`` → ``*``.
    Values stay as integers; hex formatting is done separately.
    """
    lo_keys = [k for k in args if k.endswith('_lo')]
    for lo_key in lo_keys:
        hi_key = lo_key[:-3] + '_hi'
        lo_val = args.get(lo_key)
        hi_val = args.get(hi_key)
        if not isinstance(lo_val, int) or not isinstance(hi_val, int):
            continue
        base = lo_key[:-3]
        if base.endswith('_p'):
            base = base[:-2] + '_ptr'
        combined = (hi_val << 32) | (lo_val & 0xFFFFFFFF)
        del args[lo_key]
        del args[hi_key]
        args[base] = combined
