#include "kev/kercall_fields.h"

namespace kev {
namespace {

// Sentinel for "no fields defined" -- fall back to generic arg0/arg1
static const char* const GENERIC_FAST[] = {"d1", "d2"};
static const char* const GENERIC_WIDE[] = {"d1", "d2"};
static const KercallFieldDef DEFAULT_DEF = {GENERIC_FAST, 2, GENERIC_WIDE, 2};

#define F static const char* const

// ============================================================================
// ENTER field tables
// ============================================================================

// 0: NOP
F E0_F[] = {"dummy", "empty"};
F E0_W[] = {"dummy", "empty"};

// 1: TraceEvent
F E1_F[] = {"mode", "class"};
F E1_W[] = {"mode", "class", "event", "data_1", "data_2"};

// 2: Ring0
F E2_F[] = {"func_p", "arg_p"};

// 3: CacheFlush
F E3_F[] = {"addr", "len"};
F E3_W[] = {"addr", "len", "flags"};

// 6: MsgSendPulsePtr
F E6_F[] = {"coid", "code"};
F E6_W[] = {"coid", "priority", "code", "value"};

// 7: SysCpupageGet
F E7_F[] = {"index", "empty"};

// 8: SysCpupageSet
F E8_F[] = {"index", "value"};

// 9: MsgPause
F E9_F[] = {"rcvid", "cookie"};

// 10: MsgCurrent
F E10_F[] = {"rcvid", "empty"};

// 11: MsgSendv
F E11_F[] = {"coid", "msg[0]"};
F E11_W[] = {"coid", "sparts", "rparts", "msg[0]", "msg[1]", "msg[2]"};

// 12: MsgSendvnc
F E12_F[] = {"coid", "msg[0]"};
F E12_W[] = {"coid", "sparts", "rparts", "msg[0]", "msg[1]", "msg[2]"};

// 13: MsgError
F E13_F[] = {"rcvid", "err"};

// 14: MsgReceivev
F E14_F[] = {"chid", "rparts"};

// 15: MsgReplyv
F E15_F[] = {"rcvid", "status"};
F E15_W[] = {"rcvid", "sparts", "status", "smsg[0]", "smsg[1]", "smsg[2]"};

// 16: MsgReadv
F E16_F[] = {"rcvid", "offset"};
F E16_W[] = {"rcvid", "rmsg_p", "rparts", "offset"};

// 17: MsgWritev
F E17_F[] = {"rcvid", "offset"};
F E17_W[] = {"rcvid", "sparts", "offset", "msg[0]", "msg[1]", "msg[2]"};

// 19: MsgInfo
F E19_F[] = {"rcvid", "info_p"};

// 20: MsgSendPulse
F E20_F[] = {"coid", "code"};
F E20_W[] = {"coid", "priority", "code", "value"};

// 21: MsgDeliverEvent
F E21_F[] = {"rcvid", "sigev_notify"};
F E21_W[] = {"rcvid", "sigev_notify", "sigev_func_p", "sigev_value", "sigev_attr_p"};

// 24: MsgReceivePulsev
F E24_F[] = {"chid", "rparts"};

// 25: MsgVerifyEvent
F E25_F[] = {"rcvid", "sigev_notify"};
F E25_W[] = {"rcvid", "sigev_notify", "sigev_func_p", "sigev_value"};

// 26: SignalKill
F E26_F[] = {"pid", "signo"};
F E26_W[] = {"nd", "pid", "tid", "signo", "code", "value"};

// 27: SignalReturn
F E27_F[] = {"signal_p", "empty"};

// 29: SignalAction
F E29_F[] = {"signo", "handler_p"};
F E29_W[] = {"pid", "sigstub_p", "signo", "handler_p", "sa_flags", "sa_mask_0", "sa_mask_1"};

// 30: SignalProcmask
F E30_F[] = {"pid", "tid"};
F E30_W[] = {"pid", "tid", "how", "sig_blocked_0", "sig_blocked_1"};

// 31: SignalSuspend
F E31_F[] = {"sig_blocked_0", "sig_blocked_1"};

// 32: SignalWaitinfo
F E32_F[] = {"sig_wait_0", "sig_wait_1"};

// 33: SignalKillSigval
F E33_F[] = {"pid", "signo"};
F E33_W[] = {"nd", "pid", "tid", "signo", "code", "value"};

// 35: ChannelCreate
F E35_F[] = {"flags", "empty"};

// 36: ChannelDestroy
F E36_F[] = {"chid", "empty"};

// 37: ChannelCtl
F E37_F[] = {"chid", "cmd"};
F E37_W[] = {"chid", "cmd", "new_attr"};

// 39: ConnectAttach
F E39_F[] = {"empty", "pid"};
F E39_W[] = {"empty", "pid", "chid", "index", "flags"};

// 40: ConnectDetach
F E40_F[] = {"coid", "empty"};

// 41: ConnectServerInfo
F E41_F[] = {"pid", "coid"};

// 42: ConnectClientInfo
F E42_F[] = {"scoid", "ngroups"};

// 43: ConnectFlags
F E43_F[] = {"coid", "bits"};
F E43_W[] = {"pid", "coid", "masks", "bits"};

// 46: ThreadCreate
F E46_F[] = {"func_p", "arg_p"};
F E46_W[] = {"pid", "func_p", "arg_p", "flags", "stacksize", "stackaddr_p",
             "exitfunc_p", "policy", "priority", "curpriority",
             "ss_low_prio", "ss_max_repl", "ss_repl_sec", "ss_repl_nsec",
             "ss_budget_sec", "ss_budget_nsec", "guardsize"};

// 47: ThreadDestroy
F E47_F[] = {"tid", "status_p"};
F E47_W[] = {"tid", "priority", "status_p"};

// 49: ThreadDetach
F E49_F[] = {"tid", "empty"};

// 50: ThreadJoin
F E50_F[] = {"tid", "status_p"};

// 51: ThreadCancel
F E51_F[] = {"tid", "canstub_p"};

// 52: ThreadCtl
F E52_F[] = {"cmd", "data_p"};

// 53: ThreadCtlExt
F E53_F[] = {"pid", "tid"};
F E53_W[] = {"pid", "tid", "cmd", "data_p"};

// 55: InterruptAttach
F E55_F[] = {"intr", "flags"};
F E55_W[] = {"intr", "handler_p", "area_p", "areasize", "flags"};

// 56: InterruptQuery/DetachFunc
F E56_F[] = {"type_or_intr", "id_or_handler_p"};

// 57: InterruptDetach
F E57_F[] = {"id", "empty"};

// 58: InterruptWait
F E58_F[] = {"flags", "timeout_sec"};
F E58_W[] = {"flags", "timeout_sec", "timeout_nsec"};

// 59: InterruptMask
F E59_F[] = {"intr", "id"};

// 60: InterruptUnmask
F E60_F[] = {"intr", "id"};

// 61: InterruptCharacteristic
F E61_F[] = {"id", "type"};
F E61_W[] = {"id", "type", "new"};

// 65: ClockTime
F E65_F[] = {"id", "new_sec"};
F E65_W[] = {"id", "new_sec", "new_nsec"};

// 66: ClockAdjust
F E66_F[] = {"id", "tick_count"};
F E66_W[] = {"id", "tick_count", "tick_nsec_inc"};

// 67: ClockPeriod
F E67_F[] = {"id", "new_nsec"};
F E67_W[] = {"id", "new_nsec", "new_fract"};

// 68: ClockId
F E68_F[] = {"pid", "tid"};

// 70: TimerCreate
F E70_F[] = {"timer_id", "sigev_notify"};
F E70_W[] = {"timer_id", "sigev_notify", "sigev_func_p", "sigev_value", "sigev_attr_p"};

// 71: TimerDestroy
F E71_F[] = {"id", "empty"};

// 72: TimerSettime
F E72_F[] = {"clock_id", "itime_nsec_sec"};
F E72_W[] = {"clock_id", "flags", "itime_nsec_sec", "itime_nsec_nsec",
             "itime_interval_sec", "itime_interval_nsec"};

// 73: TimerInfo
F E73_F[] = {"pid", "id"};
F E73_W[] = {"pid", "id", "flags", "info_p"};

// 74: TimerAlarm
F E74_F[] = {"clock_id", "itime_sec"};
F E74_W[] = {"clock_id", "itime_sec", "itime_nsec", "interval_sec", "interval_nsec"};

// 75: TimerTimeout
F E75_F[] = {"clock_id", "timeout_flags"};
F E75_W[] = {"clock_id", "timeout_flags", "ntime_sec", "ntime_nsec",
             "sigev_notify", "sigev_func_p", "sigev_value", "sigev_attr_p"};

// 78: SyncCreate
F E78_F[] = {"type", "sync_p"};
F E78_W[] = {"type", "sync_p", "count", "owner", "protocol", "flags",
             "prioceiling", "clockid"};

// 79: SyncDestroy
F E79_F[] = {"sync_p", "owner"};
F E79_W[] = {"sync_p", "count", "owner"};

// 80: SyncMutexLock
F E80_F[] = {"sync_p", "owner"};
F E80_W[] = {"sync_p", "count", "owner"};

// 81: SyncMutexUnlock
F E81_F[] = {"sync_p", "owner"};
F E81_W[] = {"sync_p", "count", "owner"};

// 82: SyncCondvarWait
F E82_F[] = {"sync_p", "mutex_p"};
F E82_W[] = {"sync_p", "mutex_p", "sync_count", "sync_owner",
             "mutex_count", "mutex_owner"};

// 83: SyncCondvarSignal
F E83_F[] = {"sync_p", "all"};
F E83_W[] = {"sync_p", "all", "sync_count", "sync_owner"};

// 84: SyncSemPost
F E84_F[] = {"sync_p", "count"};
F E84_W[] = {"sync_p", "count", "owner"};

// 85: SyncSemWait
F E85_F[] = {"sync_p", "count"};
F E85_W[] = {"sync_p", "try", "count", "owner"};

// 86: SyncCtl
F E86_F[] = {"cmd", "sync_p"};
F E86_W[] = {"cmd", "sync_p", "data_p", "count", "owner"};

// 88: SchedGet
F E88_F[] = {"pid", "tid"};

// 89: SchedSet
F E89_F[] = {"pid", "sched_priority"};
F E89_W[] = {"pid", "tid", "policy", "sched_priority", "sched_curpriority",
             "ss_low_prio", "ss_max_repl", "ss_repl_sec", "ss_repl_nsec",
             "ss_budget_sec", "ss_budget_nsec"};

// 90: SchedYield
F E90_F[] = {"empty", "empty"};

// 91: SchedInfo
F E91_F[] = {"pid", "policy"};

// 92: SchedCtl
F E92_F[] = {"cmd", "data_p"};

// 96: NetInfoscoid
F E96_F[] = {"rcvid_lo", "rcvid_hi"};
F E96_W[] = {"rcvid_lo", "rcvid_hi", "queueid"};

#undef F

// ============================================================================
// ENTER lookup table (indexed by call number)
// ============================================================================

#define SZ(a) (int)(sizeof(a)/sizeof(a[0]))

// Helper for entries where fast == wide
#define SAME(f) {f, SZ(f), f, SZ(f)}
#define PAIR(f, w) {f, SZ(f), w, SZ(w)}

static const KercallFieldDef ENTER_TABLE[] = {
    /* 0  NOP              */ PAIR(E0_F, E0_W),
    /* 1  TraceEvent       */ PAIR(E1_F, E1_W),
    /* 2  Ring0            */ SAME(E2_F),
    /* 3  CacheFlush       */ PAIR(E3_F, E3_W),
    /* 4  (unused)         */ {nullptr, 0, nullptr, 0},
    /* 5  MsgRegisterEvent */ {nullptr, 0, nullptr, 0},
    /* 6  MsgSendPulsePtr  */ PAIR(E6_F, E6_W),
    /* 7  SysCpupageGet    */ SAME(E7_F),
    /* 8  SysCpupageSet    */ SAME(E8_F),
    /* 9  MsgPause         */ SAME(E9_F),
    /* 10 MsgCurrent       */ SAME(E10_F),
    /* 11 MsgSendv         */ PAIR(E11_F, E11_W),
    /* 12 MsgSendvnc       */ PAIR(E12_F, E12_W),
    /* 13 MsgError         */ SAME(E13_F),
    /* 14 MsgReceivev      */ SAME(E14_F),
    /* 15 MsgReplyv        */ PAIR(E15_F, E15_W),
    /* 16 MsgReadv         */ PAIR(E16_F, E16_W),
    /* 17 MsgWritev        */ PAIR(E17_F, E17_W),
    /* 18 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 19 MsgInfo          */ SAME(E19_F),
    /* 20 MsgSendPulse     */ PAIR(E20_F, E20_W),
    /* 21 MsgDeliverEvent  */ PAIR(E21_F, E21_W),
    /* 22 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 23 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 24 MsgReceivePulsev */ SAME(E24_F),
    /* 25 MsgVerifyEvent   */ PAIR(E25_F, E25_W),
    /* 26 SignalKill       */ PAIR(E26_F, E26_W),
    /* 27 SignalReturn     */ SAME(E27_F),
    /* 28 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 29 SignalAction     */ PAIR(E29_F, E29_W),
    /* 30 SignalProcmask   */ PAIR(E30_F, E30_W),
    /* 31 SignalSuspend    */ SAME(E31_F),
    /* 32 SignalWaitinfo   */ SAME(E32_F),
    /* 33 SignalKillSigval */ PAIR(E33_F, E33_W),
    /* 34 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 35 ChannelCreate    */ SAME(E35_F),
    /* 36 ChannelDestroy   */ SAME(E36_F),
    /* 37 ChannelCtl       */ PAIR(E37_F, E37_W),
    /* 38 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 39 ConnectAttach    */ PAIR(E39_F, E39_W),
    /* 40 ConnectDetach    */ SAME(E40_F),
    /* 41 ConnectServerInfo*/ SAME(E41_F),
    /* 42 ConnectClientInfo*/ SAME(E42_F),
    /* 43 ConnectFlags     */ PAIR(E43_F, E43_W),
    /* 44 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 45 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 46 ThreadCreate     */ PAIR(E46_F, E46_W),
    /* 47 ThreadDestroy    */ PAIR(E47_F, E47_W),
    /* 48 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 49 ThreadDetach     */ SAME(E49_F),
    /* 50 ThreadJoin       */ SAME(E50_F),
    /* 51 ThreadCancel     */ SAME(E51_F),
    /* 52 ThreadCtl        */ SAME(E52_F),
    /* 53 ThreadCtlExt     */ PAIR(E53_F, E53_W),
    /* 54 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 55 InterruptAttach  */ PAIR(E55_F, E55_W),
    /* 56 InterruptQuery   */ SAME(E56_F),
    /* 57 InterruptDetach  */ SAME(E57_F),
    /* 58 InterruptWait    */ PAIR(E58_F, E58_W),
    /* 59 InterruptMask    */ SAME(E59_F),
    /* 60 InterruptUnmask  */ SAME(E60_F),
    /* 61 InterruptChar    */ PAIR(E61_F, E61_W),
    /* 62 InterruptCtl     */ SAME(E92_F),
    /* 63 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 64 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 65 ClockTime        */ PAIR(E65_F, E65_W),
    /* 66 ClockAdjust      */ PAIR(E66_F, E66_W),
    /* 67 ClockPeriod      */ PAIR(E67_F, E67_W),
    /* 68 ClockId          */ SAME(E68_F),
    /* 69 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 70 TimerCreate      */ PAIR(E70_F, E70_W),
    /* 71 TimerDestroy     */ SAME(E71_F),
    /* 72 TimerSettime     */ PAIR(E72_F, E72_W),
    /* 73 TimerInfo        */ PAIR(E73_F, E73_W),
    /* 74 TimerAlarm       */ PAIR(E74_F, E74_W),
    /* 75 TimerTimeout     */ PAIR(E75_F, E75_W),
    /* 76 TimerDelegate    */ {nullptr, 0, nullptr, 0},
    /* 77 SyncRwlock       */ {nullptr, 0, nullptr, 0},
    /* 78 SyncCreate       */ PAIR(E78_F, E78_W),
    /* 79 SyncDestroy      */ PAIR(E79_F, E79_W),
    /* 80 SyncMutexLock    */ PAIR(E80_F, E80_W),
    /* 81 SyncMutexUnlock  */ PAIR(E81_F, E81_W),
    /* 82 SyncCondvarWait  */ PAIR(E82_F, E82_W),
    /* 83 SyncCondvarSignal*/ PAIR(E83_F, E83_W),
    /* 84 SyncSemPost      */ PAIR(E84_F, E84_W),
    /* 85 SyncSemWait      */ PAIR(E85_F, E85_W),
    /* 86 SyncCtl          */ PAIR(E86_F, E86_W),
    /* 87 SyncMutexRevive  */ PAIR(E79_F, E79_W),
    /* 88 SchedGet         */ SAME(E88_F),
    /* 89 SchedSet         */ PAIR(E89_F, E89_W),
    /* 90 SchedYield       */ SAME(E90_F),
    /* 91 SchedInfo        */ SAME(E91_F),
    /* 92 SchedCtl         */ SAME(E92_F),
    /* 93 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 94 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 95 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 96 NetInfoscoid     */ PAIR(E96_F, E96_W),
};

static constexpr int ENTER_TABLE_SIZE = (int)(sizeof(ENTER_TABLE) / sizeof(ENTER_TABLE[0]));

// ============================================================================
// EXIT field tables
// ============================================================================

#define F static const char* const

F X_RV[]  = {"ret_val", "empty"};
F X_ST[]  = {"status", "empty"};

// 11: MsgSendv EXIT
F X11_F[] = {"status", "rmsg[0]"};
F X11_W[] = {"status", "rmsg[0]", "rmsg[1]", "rmsg[2]"};

// 12: MsgSendvnc EXIT
F X12_F[] = {"ret_val", "rmsg[0]"};
F X12_W[] = {"ret_val", "rmsg[0]", "rmsg[1]", "rmsg[2]"};

// 14: MsgReceivev EXIT
F X14_F[] = {"rcvid", "rmsg[0]"};
F X14_W[] = {"rcvid", "rmsg[0]", "rmsg[1]", "rmsg[2]", "empty", "empty",
             "info_pid", "info_tid", "info_chid", "info_scoid", "info_coid",
             "info_msglen", "info_srcmsglen", "info_dstmsglen",
             "info_priority", "info_flags", "info_reserved"};

// 16: MsgReadv EXIT
F X16_F[] = {"rbytes", "rmsg[0]"};
F X16_W[] = {"rbytes", "rmsg[0]", "rmsg[1]", "rmsg[2]"};

// 17: MsgWritev EXIT
F X17_F[] = {"wbytes", "empty"};

// 19: MsgInfo EXIT
F X19_F[] = {"ret_val", "empty"};
F X19_W[] = {"ret_val", "empty", "empty", "info_pid", "info_tid", "info_chid",
             "info_scoid", "info_coid", "info_msglen", "info_srcmsglen",
             "info_dstmsglen", "info_priority", "info_flags", "info_reserved"};

// 21: MsgDeliverEvent EXIT
F X21_F[] = {"ret_val", "eventp"};

// 29: SignalAction EXIT
F X29_F[] = {"ret_val", "handler_p"};
F X29_W[] = {"ret_val", "handler_p", "sa_flags", "sa_mask_0", "sa_mask_1"};

// 30: SignalProcmask EXIT
F X30_F[] = {"ret_val", "sig_blocked_0"};
F X30_W[] = {"ret_val", "sig_blocked_0", "sig_blocked_1"};

// 31: SignalSuspend EXIT
F X31_F[] = {"ret_val", "sig_blocked_p"};

// 32: SignalWaitinfo EXIT
F X32_F[] = {"sig_num", "si_code"};
F X32_W[] = {"sig_num", "si_signo", "si_code", "si_errno",
             "p[0]", "p[1]", "p[2]", "p[3]", "p[4]", "p[5]", "p[6]"};

// 35: ChannelCreate EXIT
F X35_F[] = {"chid", "empty"};

// 39: ConnectAttach EXIT
F X39_F[] = {"coid", "empty"};

// 41: ConnectServerInfo EXIT
F X41_F[] = {"coid", "empty"};
F X41_W[] = {"coid", "empty", "empty", "info_pid", "info_tid", "info_chid",
             "info_scoid", "info_coid", "info_msglen", "info_srcmsglen",
             "info_dstmsglen", "info_priority", "info_flags", "info_reserved"};

// 42: ConnectClientInfo EXIT
F X42_F[] = {"ret_val", "empty"};
F X42_W[] = {"ret_val", "empty", "info_pid", "info_sid", "flags",
             "info_ruid", "info_euid", "info_suid", "info_rgid",
             "info_egid", "info_sgid", "info_ngroups",
             "group[0]", "group[1]", "group[2]", "group[3]",
             "group[4]", "group[5]", "group[6]", "group[7]"};

// 43: ConnectFlags EXIT
F X43_F[] = {"old_flags", "empty"};

// 46: ThreadCreate EXIT
F X46_F[] = {"thread_id", "owner"};

// 50: ThreadJoin EXIT
F X50_F[] = {"ret_val", "status_p"};

// 55: InterruptAttach EXIT
F X55_F[] = {"int_fun_id", "empty"};

// 58: InterruptWait EXIT
F X58_F[] = {"ret_val", "timeout_p"};

// 59: InterruptMask EXIT
F X59_F[] = {"mask_level", "empty"};

// 61: InterruptChar EXIT
F X61_F[] = {"ret_val", "old"};

// 65: ClockTime EXIT
F X65_F[] = {"ret_val", "old_sec"};
F X65_W[] = {"ret_val", "old_sec", "old_nsec"};

// 66: ClockAdjust EXIT
F X66_F[] = {"ret_val", "old_tick_count"};
F X66_W[] = {"ret_val", "old_tick_count", "old_tick_nsec_inc"};

// 67: ClockPeriod EXIT
F X67_F[] = {"ret_val", "old_nsec"};
F X67_W[] = {"ret_val", "old_nsec", "old_fract"};

// 70: TimerCreate EXIT
F X70_F[] = {"timer_id", "empty"};

// 72: TimerSettime EXIT
F X72_F[] = {"ret_val", "oitime_sec"};
F X72_W[] = {"ret_val", "oitime_sec", "oitime_nsec",
             "oitime_interval_sec", "oitime_interval_nsec"};

// 73: TimerInfo EXIT
F X73_F[] = {"prev_id", "itime_nsec"};
F X73_W[] = {"prev_id", "itime_nsec", "itime_interval_nsec",
             "otime_nsec", "otime_interval_nsec", "flags", "tid", "notify",
             "clockid", "overruns", "sigev_notify", "sigev_func_p",
             "sigev_value", "sigev_attr_p"};

// 74: TimerAlarm EXIT
F X74_F[] = {"ret_val", "otime_sec"};
F X74_W[] = {"ret_val", "otime_sec", "otime_nsec", "interval_sec", "interval_nsec"};

// 75: TimerTimeout EXIT
F X75_F[] = {"prev_timeout_flags", "otime_sec"};
F X75_W[] = {"prev_timeout_flags", "otime_sec", "otime_nsec"};

// 88: SchedGet EXIT
F X88_F[] = {"ret_val", "sched_priority"};
F X88_W[] = {"ret_val", "sched_priority", "sched_curpriority",
             "ss_low_prio", "ss_max_repl", "ss_repl_sec", "ss_repl_nsec",
             "ss_budget_sec", "ss_budget_nsec"};

// 91: SchedInfo EXIT
F X91_F[] = {"ret_val", "priority_max"};
F X91_W[] = {"ret_val", "priority_min", "priority_max",
             "interval_sec", "interval_nsec", "priority_priv"};

#undef F

static const KercallFieldDef EXIT_TABLE[] = {
    /* 0  NOP              */ SAME(X_RV),
    /* 1  TraceEvent       */ SAME(X_RV),
    /* 2  Ring0            */ SAME(X_RV),
    /* 3  CacheFlush       */ SAME(X_RV),
    /* 4  (unused)         */ {nullptr, 0, nullptr, 0},
    /* 5  MsgRegisterEvent */ {nullptr, 0, nullptr, 0},
    /* 6  MsgSendPulsePtr  */ SAME(X_ST),
    /* 7  SysCpupageGet    */ SAME(X_RV),
    /* 8  SysCpupageSet    */ SAME(X_RV),
    /* 9  MsgPause         */ {nullptr, 0, nullptr, 0},
    /* 10 MsgCurrent       */ {nullptr, 0, nullptr, 0},
    /* 11 MsgSendv         */ PAIR(X11_F, X11_W),
    /* 12 MsgSendvnc       */ PAIR(X12_F, X12_W),
    /* 13 MsgError         */ SAME(X_RV),
    /* 14 MsgReceivev      */ PAIR(X14_F, X14_W),
    /* 15 MsgReplyv        */ SAME(X_RV),
    /* 16 MsgReadv         */ PAIR(X16_F, X16_W),
    /* 17 MsgWritev        */ SAME(X17_F),
    /* 18 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 19 MsgInfo          */ PAIR(X19_F, X19_W),
    /* 20 MsgSendPulse     */ SAME(X_ST),
    /* 21 MsgDeliverEvent  */ SAME(X21_F),
    /* 22 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 23 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 24 MsgReceivePulsev */ PAIR(X14_F, X14_W),
    /* 25 MsgVerifyEvent   */ SAME(X_RV),
    /* 26 SignalKill       */ SAME(X_RV),
    /* 27 SignalReturn     */ SAME(X_RV),
    /* 28 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 29 SignalAction     */ PAIR(X29_F, X29_W),
    /* 30 SignalProcmask   */ PAIR(X30_F, X30_W),
    /* 31 SignalSuspend    */ SAME(X31_F),
    /* 32 SignalWaitinfo   */ PAIR(X32_F, X32_W),
    /* 33 SignalKillSigval */ SAME(X_RV),
    /* 34 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 35 ChannelCreate    */ SAME(X35_F),
    /* 36 ChannelDestroy   */ SAME(X_RV),
    /* 37 ChannelCtl       */ SAME(X_RV),
    /* 38 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 39 ConnectAttach    */ SAME(X39_F),
    /* 40 ConnectDetach    */ SAME(X_RV),
    /* 41 ConnectServerInfo*/ PAIR(X41_F, X41_W),
    /* 42 ConnectClientInfo*/ PAIR(X42_F, X42_W),
    /* 43 ConnectFlags     */ SAME(X43_F),
    /* 44 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 45 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 46 ThreadCreate     */ SAME(X46_F),
    /* 47 ThreadDestroy    */ SAME(X_RV),
    /* 48 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 49 ThreadDetach     */ SAME(X_RV),
    /* 50 ThreadJoin       */ SAME(X50_F),
    /* 51 ThreadCancel     */ SAME(X_RV),
    /* 52 ThreadCtl        */ SAME(X_RV),
    /* 53 ThreadCtlExt     */ SAME(X_RV),
    /* 54 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 55 InterruptAttach  */ SAME(X55_F),
    /* 56 InterruptQuery   */ SAME(X_RV),
    /* 57 InterruptDetach  */ SAME(X_RV),
    /* 58 InterruptWait    */ SAME(X58_F),
    /* 59 InterruptMask    */ SAME(X59_F),
    /* 60 InterruptUnmask  */ SAME(X59_F),
    /* 61 InterruptChar    */ SAME(X61_F),
    /* 62 InterruptCtl     */ SAME(X_RV),
    /* 63 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 64 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 65 ClockTime        */ PAIR(X65_F, X65_W),
    /* 66 ClockAdjust      */ PAIR(X66_F, X66_W),
    /* 67 ClockPeriod      */ PAIR(X67_F, X67_W),
    /* 68 ClockId          */ SAME(X_RV),
    /* 69 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 70 TimerCreate      */ SAME(X70_F),
    /* 71 TimerDestroy     */ SAME(X_RV),
    /* 72 TimerSettime     */ PAIR(X72_F, X72_W),
    /* 73 TimerInfo        */ PAIR(X73_F, X73_W),
    /* 74 TimerAlarm       */ PAIR(X74_F, X74_W),
    /* 75 TimerTimeout     */ PAIR(X75_F, X75_W),
    /* 76 TimerDelegate    */ SAME(X_RV),
    /* 77 SyncRwlock       */ SAME(X_RV),
    /* 78 SyncCreate       */ SAME(X_RV),
    /* 79 SyncDestroy      */ SAME(X_RV),
    /* 80 SyncMutexLock    */ SAME(X_RV),
    /* 81 SyncMutexUnlock  */ SAME(X_RV),
    /* 82 SyncCondvarWait  */ SAME(X_RV),
    /* 83 SyncCondvarSignal*/ SAME(X_RV),
    /* 84 SyncSemPost      */ SAME(X_RV),
    /* 85 SyncSemWait      */ SAME(X_RV),
    /* 86 SyncCtl          */ SAME(X_RV),
    /* 87 SyncMutexRevive  */ SAME(X_RV),
    /* 88 SchedGet         */ PAIR(X88_F, X88_W),
    /* 89 SchedSet         */ SAME(X_RV),
    /* 90 SchedYield       */ SAME(X_RV),
    /* 91 SchedInfo        */ PAIR(X91_F, X91_W),
    /* 92 SchedCtl         */ SAME(X_RV),
    /* 93 (unused)         */ SAME(X_RV),
    /* 94 (unused)         */ SAME(X_RV),
    /* 95 (unused)         */ {nullptr, 0, nullptr, 0},
    /* 96 NetInfoscoid     */ SAME(X_RV),
};

static constexpr int EXIT_TABLE_SIZE = (int)(sizeof(EXIT_TABLE) / sizeof(EXIT_TABLE[0]));

#undef SZ
#undef SAME
#undef PAIR
#undef DEF

} // anonymous namespace

const KercallFieldDef& get_kercall_enter_fields(int call_num) {
    if (call_num >= 0 && call_num < ENTER_TABLE_SIZE && ENTER_TABLE[call_num].fast)
        return ENTER_TABLE[call_num];
    return DEFAULT_DEF;
}

const KercallFieldDef& get_kercall_exit_fields(int call_num) {
    if (call_num >= 0 && call_num < EXIT_TABLE_SIZE && EXIT_TABLE[call_num].fast)
        return EXIT_TABLE[call_num];
    return DEFAULT_DEF;
}

} // namespace kev
