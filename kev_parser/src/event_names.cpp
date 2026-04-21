#include "kev/event_names.h"

namespace kev {
namespace event_names {

static const char* const THREAD_STATES[] = {
    "DEAD",          // 0
    "RUNNING",       // 1
    "READY",         // 2
    "STOPPED",       // 3
    "SEND",          // 4
    "RECEIVE",       // 5
    "REPLY",         // 6
#if KEV_QNX_VERSION >= 80
    "MQ_SEND",       // 7  (QNX 8.0: STATE_MQ_SEND)
    "MQ_RECEIVE",    // 8  (QNX 8.0: STATE_MQ_RECEIVE)
#else
    "STACK",         // 7  (QNX 7.1)
    "WAITTHREAD",    // 8  (QNX 7.1)
#endif
    "WAITPAGE",      // 9
    "SIGSUSPEND",    // 10
    "SIGWAITINFO",   // 11
    "NANOSLEEP",     // 12
    "MUTEX",         // 13
    "CONDVAR",       // 14
    "JOIN",          // 15
    "INTR",          // 16
    "SEM",           // 17
    "WAITCTX",       // 18
#if KEV_QNX_VERSION >= 80
    "RWLOCK_READ",   // 19 (QNX 8.0: STATE_RWLOCK_READ)
    "RWLOCK_WRITE",  // 20 (QNX 8.0: STATE_RWLOCK_WRITE)
    "BARRIER",       // 21 (QNX 8.0: STATE_BARRIER)
    "PIPE",          // 22 (QNX 8.0: STATE_PIPE)
    nullptr,         // 23 reserved
#else
    "NET_SEND",      // 19 (QNX 7.1)
    "NET_REPLY",     // 20 (QNX 7.1)
    nullptr, nullptr, nullptr,  // 21-23 reserved
#endif
    "CREATE",        // 24
    "DESTROY",       // 25
#if KEV_QNX_VERSION >= 80
    "MUON_MUTEX",    // 26 (QNX 8.0)
    "TRACEBUFFER",   // 27 (QNX 8.0)
    "INTR_ATTACH_EV",// 28 (QNX 8.0)
    "TIMER_DELEGATE",// 29 (QNX 8.0)
#endif
};
static constexpr int NUM_THREAD_STATES = sizeof(THREAD_STATES) / sizeof(THREAD_STATES[0]);

const char* thread_state_name(int state_index) {
    if (state_index >= 0 && state_index < NUM_THREAD_STATES && THREAD_STATES[state_index])
        return THREAD_STATES[state_index];
    return "UNKNOWN";
}

int bitmask_to_state_index(unsigned bitmask) {
    if (bitmask == 0) return -1;
    int idx = 0;
    unsigned v = bitmask;
    while ((v & 1u) == 0 && idx < 31) { v >>= 1; ++idx; }
    return idx;
}

int event_to_state_index(unsigned event_value) {
    // libtraceparser callback event values are always direct state indices,
    // regardless of QNX version. The bitmask encoding in _NTO_TRACE_TH*
    // macros (QNX 7.1) is only used for traceparser_cs() registration;
    // libtraceparser internally converts them before invoking callbacks.
    return static_cast<int>(event_value);
}

// Indexed by __KER_* constants (from kercalls.h), sparse with nullptr for gaps
static const char* const KERCALL_NAMES[] = {
    "Nop",                     // 0  __KER_NOP
    "TraceEvent",              // 1  __KER_TRACE_EVENT
    "Ring0",                   // 2  __KER_RING0
    "CacheFlush",              // 3  __KER_CACHE_FLUSH
    "SysSrandom",              // 4  __KER_SYS_SRANDOM
    "MsgRegisterEvent",        // 5  __KER_MSG_REGISTER_EVENT
    "MsgSendPulsePtr",         // 6  __KER_MSG_SEND_PULSEPTR
    "SysCpupageGet",           // 7  __KER_SYS_CPUPAGE_GET
    "SysCpupageSet",           // 8  __KER_SYS_CPUPAGE_SET
    "MsgPause",                // 9  __KER_MSG_PAUSE
    "MsgCurrent",              // 10 __KER_MSG_CURRENT
    "MsgSendv",                // 11 __KER_MSG_SENDV
    "MsgSendvnc",              // 12 __KER_MSG_SENDVNC
    "MsgError",                // 13 __KER_MSG_ERROR
    "MsgReceivev",             // 14 __KER_MSG_RECEIVEV
    "MsgReplyv",               // 15 __KER_MSG_REPLYV
    "MsgReadv",                // 16 __KER_MSG_READV
    "MsgWritev",               // 17 __KER_MSG_WRITEV
    "MsgReadwritev",           // 18 __KER_MSG_READWRITEV
    "MsgInfo",                 // 19 __KER_MSG_INFO
    "MsgSendPulse",            // 20 __KER_MSG_SEND_PULSE
    "MsgDeliverEvent",         // 21 __KER_MSG_DELIVER_EVENT
    "MsgKeyData",              // 22 __KER_MSG_KEYDATA
    "MsgReadiov",              // 23 __KER_MSG_READIOV
    "MsgReceivePulsev",        // 24 __KER_MSG_RECEIVEPULSEV
    "MsgVerifyEvent",          // 25 __KER_MSG_VERIFY_EVENT
    "SignalKill",              // 26 __KER_SIGNAL_KILL
    "SignalReturn",            // 27 __KER_SIGNAL_RETURN
    "SignalFault",             // 28 __KER_SIGNAL_FAULT
    "SignalAction",            // 29 __KER_SIGNAL_ACTION
    "SignalProcmask",          // 30 __KER_SIGNAL_PROCMASK
    "SignalSuspend",           // 31 __KER_SIGNAL_SUSPEND
    "SignalWaitinfo",          // 32 __KER_SIGNAL_WAITINFO
    "SignalKillSigval",        // 33 __KER_SIGNAL_KILL_SIGVAL
    nullptr,                   // 34 (unused)
    "ChannelCreate",           // 35 __KER_CHANNEL_CREATE
    "ChannelDestroy",          // 36 __KER_CHANNEL_DESTROY
    "ChanconAttr",             // 37 __KER_CHANCON_ATTR
    nullptr,                   // 38 (unused)
    "ConnectAttach",           // 39 __KER_CONNECT_ATTACH
    "ConnectDetach",           // 40 __KER_CONNECT_DETACH
    "ConnectServerInfo",       // 41 __KER_CONNECT_SERVER_INFO
    "ConnectClientInfo",       // 42 __KER_CONNECT_CLIENT_INFO
    "ConnectFlags",            // 43 __KER_CONNECT_FLAGS
    nullptr,                   // 44 (unused)
    nullptr,                   // 45 (unused)
    "ThreadCreate",            // 46 __KER_THREAD_CREATE
    "ThreadDestroy",           // 47 __KER_THREAD_DESTROY
    "ThreadDestroyAll",        // 48 __KER_THREAD_DESTROYALL
    "ThreadDetach",            // 49 __KER_THREAD_DETACH
    "ThreadJoin",              // 50 __KER_THREAD_JOIN
    "ThreadCancel",            // 51 __KER_THREAD_CANCEL
    "ThreadCtl",               // 52 __KER_THREAD_CTL
    "ThreadCtlExt",            // 53 __KER_THREAD_CTLEXT
    nullptr,                   // 54 (unused)
    "InterruptAttach",         // 55 __KER_INTERRUPT_ATTACH
    "InterruptDetachFunc",     // 56 __KER_INTERRUPT_DETACH_FUNC
    "InterruptDetach",         // 57 __KER_INTERRUPT_DETACH
    "InterruptWait",           // 58 __KER_INTERRUPT_WAIT
    "InterruptMask",           // 59 __KER_INTERRUPT_MASK
    "InterruptUnmask",         // 60 __KER_INTERRUPT_UNMASK
    "InterruptCharacteristic", // 61 __KER_INTERRUPT_CHARACTERISTIC
    "InterruptCtl",            // 62 __KER_INTERRUPT_CTL (QNX 8.0)
    nullptr,                   // 63 (unused)
    nullptr,                   // 64 (unused)
    "ClockTime",               // 65 __KER_CLOCK_TIME
    "ClockAdjust",             // 66 __KER_CLOCK_ADJUST
    "ClockPeriod",             // 67 __KER_CLOCK_PERIOD
    "ClockId",                 // 68 __KER_CLOCK_ID
    nullptr,                   // 69 (unused)
    "TimerCreate",             // 70 __KER_TIMER_CREATE
    "TimerDestroy",            // 71 __KER_TIMER_DESTROY
    "TimerSettime",            // 72 __KER_TIMER_SETTIME
    "TimerInfo",               // 73 __KER_TIMER_INFO
    "TimerAlarm",              // 74 __KER_TIMER_ALARM
    "TimerTimeout",            // 75 __KER_TIMER_TIMEOUT
    "TimerDelegate",           // 76 __KER_TIMER_DELEGATE (QNX 8.0)
    "SyncRwlock",              // 77 __KER_SYNC_RWLOCK (QNX 8.0)
    "SyncCreate",              // 78 __KER_SYNC_CREATE
    "SyncDestroy",             // 79 __KER_SYNC_DESTROY
    "SyncMutexLock",           // 80 __KER_SYNC_MUTEX_LOCK
    "SyncMutexUnlock",         // 81 __KER_SYNC_MUTEX_UNLOCK
    "SyncCondvarWait",         // 82 __KER_SYNC_CONDVAR_WAIT
    "SyncCondvarSignal",       // 83 __KER_SYNC_CONDVAR_SIGNAL
    "SyncSemPost",             // 84 __KER_SYNC_SEM_POST
    "SyncSemWait",             // 85 __KER_SYNC_SEM_WAIT
    "SyncCtl",                 // 86 __KER_SYNC_CTL
    "SyncMutexRevive",         // 87 __KER_SYNC_MUTEX_REVIVE
    "SchedGet",                // 88 __KER_SCHED_GET
    "SchedSet",                // 89 __KER_SCHED_SET
    "SchedYield",              // 90 __KER_SCHED_YIELD
    "SchedInfo",               // 91 __KER_SCHED_INFO
    "SchedCtl",                // 92 __KER_SCHED_CTL
    "NetCred",                 // 93 __KER_NET_CRED
    "NetVtid",                 // 94 __KER_NET_VTID
    "NetUnblock",              // 95 __KER_NET_UNBLOCK
    "NetInfoscoid",            // 96 __KER_NET_INFOSCOID
    "NetSignalKill",           // 97 __KER_NET_SIGNAL_KILL
    nullptr,                   // 98 (unused)
    nullptr,                   // 99 (unused)
    "PowerParameter",          // 100 __KER_POWER_PARAMETER
    "PowerActive",             // 101 __KER_POWER_ACTIVE
    "SchedWaypoint",           // 102 __KER_SCHED_WAYPOINT
    nullptr,                   // 103 (unused)
    "SyncTypeDestroy",         // 104 __KER_SYNC_TYPE_DESTROY (QNX 8.0)
    nullptr,                   // 105 (unused)
    "SysCustom",               // 106 __KER_SYS_CUSTOM
    "Bad",                     // 107 __KER_BAD
};
static constexpr int NUM_KERCALLS = sizeof(KERCALL_NAMES) / sizeof(KERCALL_NAMES[0]);

const char* kercall_name(int call_num) {
    if (call_num >= 0 && call_num < NUM_KERCALLS && KERCALL_NAMES[call_num])
        return KERCALL_NAMES[call_num];
    return "UNKNOWN_KERCALL";
}

static const char* const COMM_NAMES[] = {
    "SND_MSG",              // 0
    "RCV_MSG",              // 1
    "REPLY_MSG",            // 2
    "SND_PULSE_EXE",        // 3
    "SND_PULSE_DIS",        // 4
    "SND_PULSE_DEA",        // 5
    "RCV_PULSE_EXE",        // 6
    "RCV_PULSE_DIS",        // 7
    "RCV_PULSE_DEA",        // 8
    "SND_SIGNAL",           // 9
    "RCV_SIGNAL",           // 10
    "ERROR",                // 11
};
static constexpr int NUM_COMMS = sizeof(COMM_NAMES) / sizeof(COMM_NAMES[0]);

const char* comm_name(int event_index) {
    if (event_index >= 0 && event_index < NUM_COMMS)
        return COMM_NAMES[event_index];
    return "UNKNOWN_COMM";
}

// Indexed by _NTO_TRACE_SYS_* codes (low bits of event, 0x00-0x17+)
static const char* const SYSTEM_NAMES[] = {
    nullptr,           // 0x00
    "SYS_RESERVED",    // 0x01
    "PATHMGR",         // 0x02
    "APS_NAME",        // 0x03
    "APS_BUDGETS",     // 0x04
    "APS_BANKRUPTCY",  // 0x05
    "MMAP",            // 0x06
    "MUNMAP",          // 0x07
    "MAPNAME",         // 0x08
    "ADDRESS",         // 0x09
    "FUNC_ENTER",      // 0x0a
    "FUNC_EXIT",       // 0x0b
    "SLOG",            // 0x0c
    "DEFRAG_START",    // 0x0d
    "RUNSTATE",        // 0x0e
    "POWER",           // 0x0f
    "IPI",             // 0x10
    "PAGEWAIT",        // 0x11
    "TIMER",           // 0x12
    "DEFRAG_END",      // 0x13
    "PROFILE",         // 0x14
    "MAPNAME_64",      // 0x15
    "APS_PSTATS",      // 0x16
    "APS_OSTATS",      // 0x17
};
static constexpr int NUM_SYSTEMS = sizeof(SYSTEM_NAMES) / sizeof(SYSTEM_NAMES[0]);

const char* system_name(int event_index) {
    if (event_index >= 0 && event_index < NUM_SYSTEMS && SYSTEM_NAMES[event_index])
        return SYSTEM_NAMES[event_index];
    return "UNKNOWN_SYSTEM";
}

} // namespace event_names
} // namespace kev
