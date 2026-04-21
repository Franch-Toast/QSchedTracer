#include "qst/event_names.h"

#include <cstdint>
#include <cstring>

namespace qst {

static const char* KERCALL_NAMES_71_DATA[kKercallTableSize] = {
  "Nop",
  "TraceEvent",
  "Ring0",
  "CacheFlush",
  "SysSrandom",
  "MsgRegisterEvent",
  "MsgSendPulsePtr",
  "SysCpupageGet",
  "SysCpupageSet",
  "MsgPause",
  "MsgCurrent",
  "MsgSendv",
  "MsgSendvnc",
  "MsgError",
  "MsgReceivev",
  "MsgReplyv",
  "MsgReadv",
  "MsgWritev",
  "MsgReadwritev",
  "MsgInfo",
  "MsgSendPulse",
  "MsgDeliverEvent",
  "MsgKeyData",
  "MsgReadiov",
  "MsgReceivePulsev",
  "MsgVerifyEvent",
  "SignalKill",
  "SignalReturn",
  "SignalFault",
  "SignalAction",
  "SignalProcmask",
  "SignalSuspend",
  "SignalWaitinfo",
  "SignalKillSigval",
  nullptr,
  "ChannelCreate",
  "ChannelDestroy",
  "ChanconAttr",
  nullptr,
  "ConnectAttach",
  "ConnectDetach",
  "ConnectServerInfo",
  "ConnectClientInfo",
  "ConnectFlags",
  nullptr,
  nullptr,
  "ThreadCreate",
  "ThreadDestroy",
  "ThreadDestroyAll",
  "ThreadDetach",
  "ThreadJoin",
  "ThreadCancel",
  "ThreadCtl",
  "ThreadCtlExt",
  nullptr,
  "InterruptAttach",
  "InterruptDetachFunc",
  "InterruptDetach",
  "InterruptWait",
  "InterruptMask",
  "InterruptUnmask",
  "InterruptCharacteristic",
  nullptr,
  nullptr,
  nullptr,
  "ClockTime",
  "ClockAdjust",
  "ClockPeriod",
  "ClockId",
  nullptr,
  "TimerCreate",
  "TimerDestroy",
  "TimerSettime",
  "TimerInfo",
  "TimerAlarm",
  "TimerTimeout",
  nullptr,
  nullptr,
  "SyncCreate",
  "SyncDestroy",
  "SyncMutexLock",
  "SyncMutexUnlock",
  "SyncCondvarWait",
  "SyncCondvarSignal",
  "SyncSemPost",
  "SyncSemWait",
  "SyncCtl",
  "SyncMutexRevive",
  "SchedGet",
  "SchedSet",
  "SchedYield",
  "SchedInfo",
  "SchedCtl",
  "NetCred",
  "NetVtid",
  "NetUnblock",
  "NetInfoscoid",
  "NetSignalKill",
  nullptr,
  nullptr,
  "PowerParameter",
  "PowerActive",
  "SchedWaypoint",
  nullptr,
  nullptr,
  nullptr,
  "SysCustom",
  "Bad",
};

static const char* KERCALL_NAMES_80_DATA[kKercallTableSize] = {
  "Nop",
  "TraceEvent",
  "Ring0",
  "CacheControl",
  "SysSrandom",
  "MsgRegisterEvent",
  "MsgSendPulsePtr",
  "SysCpupageGet",
  "SysCpupageSet",
  "MsgPause",
  "MsgCurrent",
  "MsgSendv",
  "MsgSendvnc",
  "MsgError",
  "MsgReceivev",
  "MsgReplyv",
  "MsgReadv",
  "MsgWritev",
  nullptr,
  "MsgInfo",
  "MsgSendPulse",
  "MsgDeliverEvent",
  nullptr,
  nullptr,
  "MsgReceivePulsev",
  "MsgVerifyEvent",
  "SignalKill",
  "SignalReturn",
  nullptr,
  "SignalAction",
  "SignalProcmask",
  "SignalSuspend",
  "SignalWaitinfo",
  "SignalKillSigval",
  nullptr,
  "ChannelCreate",
  "ChannelDestroy",
  "ChannelCtl",
  nullptr,
  "ConnectAttach",
  "ConnectDetach",
  "ConnectServerInfo",
  "ConnectClientInfo",
  "ConnectFlags",
  "ConnectDupfd",
  nullptr,
  "ThreadCreate",
  "ThreadDestroy",
  nullptr,
  "ThreadDetach",
  "ThreadJoin",
  "ThreadCancel",
  "ThreadCtl",
  "ThreadCtlExt",
  nullptr,
  "InterruptAttach",
  "InterruptQuery",
  "InterruptDetach",
  "InterruptWait",
  "InterruptMask",
  "InterruptUnmask",
  "InterruptUnblock",
  "InterruptCtl",
  nullptr,
  nullptr,
  "ClockTime",
  "ClockAdjust",
  "ClockPeriod",
  "ClockId",
  nullptr,
  "TimerCreate",
  "TimerDestroy",
  "TimerSettime",
  "TimerInfo",
  "TimerAlarm",
  "TimerTimeout",
  "TimerDelegate",
  "SyncRwlock",
  "SyncCreate",
  "SyncDestroy",
  "SyncMutexLock",
  "SyncMutexUnlock",
  "SyncCondvarWait",
  "SyncCondvarSignal",
  "SyncSemPost",
  "SyncSemWait",
  "SyncCtl",
  "SyncBarrierWait",
  "SchedGet",
  "SchedSet",
  "SchedYield",
  "SchedInfo",
  "SchedCtl",
  "MsgQueueOpen",
  "MsgQueueSend",
  "MsgQueueReceive",
  "MsgQueueClose",
  "MsgQueueCtl",
  "PipeOpen",
  "PipeCtl",
  "PipeClose",
  nullptr,
  nullptr,
  nullptr,
  "SyncTypeDestroy",
  "Test",
  "SysCustom",
  "Bad",
};

static const char* K_DEFAULT_FAST[] = { "d1", "d2" };
static const char* K_DEFAULT_WIDE[] = { "d1", "d2" };

static const char* K_ENTER_0_FAST[] = {
  "dummy",
  "empty",
};
static const char* K_ENTER_0_WIDE[] = {
  "dummy",
  "empty",
};
static const char* K_ENTER_1_FAST[] = {
  "mode",
  "class",
};
static const char* K_ENTER_1_WIDE[] = {
  "mode",
  "class",
  "event",
  "data_1",
  "data_2",
};
static const char* K_ENTER_2_FAST[] = {
  "func_p",
  "arg_p",
};
static const char* K_ENTER_2_WIDE[] = {
  "func_p",
  "arg_p",
};
static const char* K_ENTER_3_FAST[] = {
  "addr",
  "len",
};
static const char* K_ENTER_3_WIDE[] = {
  "addr",
  "len",
  "flags",
};
static const char* K_ENTER_5_FAST[] = {
  "empty",
  "empty",
};
static const char* K_ENTER_5_WIDE[] = {
  "empty",
  "empty",
};
static const char* K_ENTER_6_FAST[] = {
  "coid",
  "code",
};
static const char* K_ENTER_6_WIDE[] = {
  "coid",
  "priority",
  "code",
  "value",
};
static const char* K_ENTER_7_FAST[] = {
  "index",
  "empty",
};
static const char* K_ENTER_7_WIDE[] = {
  "index",
  "empty",
};
static const char* K_ENTER_8_FAST[] = {
  "index",
  "value",
};
static const char* K_ENTER_8_WIDE[] = {
  "index",
  "value",
};
static const char* K_ENTER_9_FAST[] = {
  "rcvid",
  "cookie",
};
static const char* K_ENTER_9_WIDE[] = {
  "rcvid",
  "cookie",
};
static const char* K_ENTER_10_FAST[] = {
  "rcvid",
  "empty",
};
static const char* K_ENTER_10_WIDE[] = {
  "rcvid",
  "empty",
};
static const char* K_ENTER_11_FAST[] = {
  "coid",
  "msg[0]",
};
static const char* K_ENTER_11_WIDE[] = {
  "coid",
  "sparts",
  "rparts",
  "msg[0]",
  "msg[1]",
  "msg[2]",
};
static const char* K_ENTER_12_FAST[] = {
  "coid",
  "msg[0]",
};
static const char* K_ENTER_12_WIDE[] = {
  "coid",
  "sparts",
  "rparts",
  "msg[0]",
  "msg[1]",
  "msg[2]",
};
static const char* K_ENTER_13_FAST[] = {
  "rcvid",
  "err",
};
static const char* K_ENTER_13_WIDE[] = {
  "rcvid",
  "err",
};
static const char* K_ENTER_14_FAST[] = {
  "chid",
  "rparts",
};
static const char* K_ENTER_14_WIDE[] = {
  "chid",
  "rparts",
};
static const char* K_ENTER_15_FAST[] = {
  "rcvid",
  "status",
};
static const char* K_ENTER_15_WIDE[] = {
  "rcvid",
  "sparts",
  "status",
  "smsg[0]",
  "smsg[1]",
  "smsg[2]",
};
static const char* K_ENTER_16_FAST[] = {
  "rcvid",
  "offset",
};
static const char* K_ENTER_16_WIDE[] = {
  "rcvid",
  "rmsg_p",
  "rparts",
  "offset",
};
static const char* K_ENTER_17_FAST[] = {
  "rcvid",
  "offset",
};
static const char* K_ENTER_17_WIDE[] = {
  "rcvid",
  "sparts",
  "offset",
  "msg[0]",
  "msg[1]",
  "msg[2]",
};
static const char* K_ENTER_19_FAST[] = {
  "rcvid",
  "info_p",
};
static const char* K_ENTER_19_WIDE[] = {
  "rcvid",
  "info_p",
};
static const char* K_ENTER_20_FAST[] = {
  "coid",
  "code",
};
static const char* K_ENTER_20_WIDE[] = {
  "coid",
  "priority",
  "code",
  "value",
};
static const char* K_ENTER_21_FAST[] = {
  "rcvid",
  "sigev_notify",
};
static const char* K_ENTER_21_WIDE[] = {
  "rcvid",
  "sigev_notify",
  "sigev_func_p",
  "sigev_value",
  "sigev_attr_p",
};
static const char* K_ENTER_24_FAST[] = {
  "chid",
  "rparts",
};
static const char* K_ENTER_24_WIDE[] = {
  "chid",
  "rparts",
};
static const char* K_ENTER_25_FAST[] = {
  "rcvid",
  "sigev_notify",
};
static const char* K_ENTER_25_WIDE[] = {
  "rcvid",
  "sigev_notify",
  "sigev_func_p",
  "sigev_value",
};
static const char* K_ENTER_26_FAST[] = {
  "pid",
  "signo",
};
static const char* K_ENTER_26_WIDE[] = {
  "nd",
  "pid",
  "tid",
  "signo",
  "code",
  "value",
};
static const char* K_ENTER_27_FAST[] = {
  "signal_p",
  "empty",
};
static const char* K_ENTER_27_WIDE[] = {
  "signal_p",
  "empty",
};
static const char* K_ENTER_29_FAST[] = {
  "signo",
  "handler_p",
};
static const char* K_ENTER_29_WIDE[] = {
  "pid",
  "sigstub_p",
  "signo",
  "handler_p",
  "sa_flags",
  "sa_mask_0",
  "sa_mask_1",
};
static const char* K_ENTER_30_FAST[] = {
  "pid",
  "tid",
};
static const char* K_ENTER_30_WIDE[] = {
  "pid",
  "tid",
  "how",
  "sig_blocked_0",
  "sig_blocked_1",
};
static const char* K_ENTER_31_FAST[] = {
  "sig_blocked_0",
  "sig_blocked_1",
};
static const char* K_ENTER_31_WIDE[] = {
  "sig_blocked_0",
  "sig_blocked_1",
};
static const char* K_ENTER_32_FAST[] = {
  "sig_wait_0",
  "sig_wait_1",
};
static const char* K_ENTER_32_WIDE[] = {
  "sig_wait_0",
  "sig_wait_1",
};
static const char* K_ENTER_33_FAST[] = {
  "pid",
  "signo",
};
static const char* K_ENTER_33_WIDE[] = {
  "nd",
  "pid",
  "tid",
  "signo",
  "code",
  "value",
};
static const char* K_ENTER_35_FAST[] = {
  "flags",
  "empty",
};
static const char* K_ENTER_35_WIDE[] = {
  "flags",
  "empty",
};
static const char* K_ENTER_36_FAST[] = {
  "chid",
  "empty",
};
static const char* K_ENTER_36_WIDE[] = {
  "chid",
  "empty",
};
static const char* K_ENTER_37_FAST[] = {
  "chid",
  "cmd",
};
static const char* K_ENTER_37_WIDE[] = {
  "chid",
  "cmd",
  "new_attr",
};
static const char* K_ENTER_39_FAST[] = {
  "empty",
  "pid",
};
static const char* K_ENTER_39_WIDE[] = {
  "empty",
  "pid",
  "chid",
  "index",
  "flags",
};
static const char* K_ENTER_40_FAST[] = {
  "coid",
  "empty",
};
static const char* K_ENTER_40_WIDE[] = {
  "coid",
  "empty",
};
static const char* K_ENTER_41_FAST[] = {
  "pid",
  "coid",
};
static const char* K_ENTER_41_WIDE[] = {
  "pid",
  "coid",
};
static const char* K_ENTER_42_FAST[] = {
  "scoid",
  "ngroups",
};
static const char* K_ENTER_42_WIDE[] = {
  "scoid",
  "ngroups",
};
static const char* K_ENTER_43_FAST[] = {
  "coid",
  "bits",
};
static const char* K_ENTER_43_WIDE[] = {
  "pid",
  "coid",
  "masks",
  "bits",
};
static const char* K_ENTER_46_FAST[] = {
  "func_p",
  "arg_p",
};
static const char* K_ENTER_46_WIDE[] = {
  "pid",
  "func_p",
  "arg_p",
  "flags",
  "stacksize",
  "stackaddr_p",
  "exitfunc_p",
  "policy",
  "priority",
  "curpriority",
  "ss_low_prio",
  "ss_max_repl",
  "ss_repl_sec",
  "ss_repl_nsec",
  "ss_budget_sec",
  "ss_budget_nsec",
  "guardsize",
};
static const char* K_ENTER_47_FAST[] = {
  "tid",
  "status_p",
};
static const char* K_ENTER_47_WIDE[] = {
  "tid",
  "priority",
  "status_p",
};
static const char* K_ENTER_49_FAST[] = {
  "tid",
  "empty",
};
static const char* K_ENTER_49_WIDE[] = {
  "tid",
  "empty",
};
static const char* K_ENTER_50_FAST[] = {
  "tid",
  "status_p",
};
static const char* K_ENTER_50_WIDE[] = {
  "tid",
  "status_p",
};
static const char* K_ENTER_51_FAST[] = {
  "tid",
  "canstub_p",
};
static const char* K_ENTER_51_WIDE[] = {
  "tid",
  "canstub_p",
};
static const char* K_ENTER_52_FAST[] = {
  "cmd",
  "data_p",
};
static const char* K_ENTER_52_WIDE[] = {
  "cmd",
  "data_p",
};
static const char* K_ENTER_53_FAST[] = {
  "pid",
  "tid",
};
static const char* K_ENTER_53_WIDE[] = {
  "pid",
  "tid",
  "cmd",
  "data_p",
};
static const char* K_ENTER_55_FAST[] = {
  "intr",
  "flags",
};
static const char* K_ENTER_55_WIDE[] = {
  "intr",
  "handler_p",
  "area_p",
  "areasize",
  "flags",
};
static const char* K_ENTER_56_FAST[] = {
  "type_or_intr",
  "id_or_handler_p",
};
static const char* K_ENTER_56_WIDE[] = {
  "type_or_intr",
  "id_or_handler_p",
};
static const char* K_ENTER_57_FAST[] = {
  "id",
  "empty",
};
static const char* K_ENTER_57_WIDE[] = {
  "id",
  "empty",
};
static const char* K_ENTER_58_FAST[] = {
  "flags",
  "timeout_sec",
};
static const char* K_ENTER_58_WIDE[] = {
  "flags",
  "timeout_sec",
  "timeout_nsec",
};
static const char* K_ENTER_59_FAST[] = {
  "intr",
  "id",
};
static const char* K_ENTER_59_WIDE[] = {
  "intr",
  "id",
};
static const char* K_ENTER_60_FAST[] = {
  "intr",
  "id",
};
static const char* K_ENTER_60_WIDE[] = {
  "intr",
  "id",
};
static const char* K_ENTER_61_FAST[] = {
  "id",
  "type",
};
static const char* K_ENTER_61_WIDE[] = {
  "id",
  "type",
  "new",
};
static const char* K_ENTER_62_FAST[] = {
  "cmd",
  "data_p",
};
static const char* K_ENTER_62_WIDE[] = {
  "cmd",
  "data_p",
};
static const char* K_ENTER_65_FAST[] = {
  "id",
  "new_sec",
};
static const char* K_ENTER_65_WIDE[] = {
  "id",
  "new_sec",
  "new_nsec",
};
static const char* K_ENTER_66_FAST[] = {
  "id",
  "tick_count",
};
static const char* K_ENTER_66_WIDE[] = {
  "id",
  "tick_count",
  "tick_nsec_inc",
};
static const char* K_ENTER_67_FAST[] = {
  "id",
  "new_nsec",
};
static const char* K_ENTER_67_WIDE[] = {
  "id",
  "new_nsec",
  "new_fract",
};
static const char* K_ENTER_68_FAST[] = {
  "pid",
  "tid",
};
static const char* K_ENTER_68_WIDE[] = {
  "pid",
  "tid",
};
static const char* K_ENTER_70_FAST[] = {
  "timer_id",
  "sigev_notify",
};
static const char* K_ENTER_70_WIDE[] = {
  "timer_id",
  "sigev_notify",
  "sigev_func_p",
  "sigev_value",
  "sigev_attr_p",
};
static const char* K_ENTER_71_FAST[] = {
  "id",
  "empty",
};
static const char* K_ENTER_71_WIDE[] = {
  "id",
  "empty",
};
static const char* K_ENTER_72_FAST[] = {
  "clock_id",
  "itime_nsec_sec",
};
static const char* K_ENTER_72_WIDE[] = {
  "clock_id",
  "flags",
  "itime_nsec_sec",
  "itime_nsec_nsec",
  "itime_interval_sec",
  "itime_interval_nsec",
};
static const char* K_ENTER_73_FAST[] = {
  "pid",
  "id",
};
static const char* K_ENTER_73_WIDE[] = {
  "pid",
  "id",
  "flags",
  "info_p",
};
static const char* K_ENTER_74_FAST[] = {
  "clock_id",
  "itime_sec",
};
static const char* K_ENTER_74_WIDE[] = {
  "clock_id",
  "itime_sec",
  "itime_nsec",
  "interval_sec",
  "interval_nsec",
};
static const char* K_ENTER_75_FAST[] = {
  "clock_id",
  "timeout_flags",
};
static const char* K_ENTER_75_WIDE[] = {
  "clock_id",
  "timeout_flags",
  "ntime_sec",
  "ntime_nsec",
  "sigev_notify",
  "sigev_func_p",
  "sigev_value",
  "sigev_attr_p",
};
static const char* K_ENTER_76_FAST[] = {
  "action",
  "delegate_cpu",
};
static const char* K_ENTER_76_WIDE[] = {
  "action",
  "delegate_cpu",
};
static const char* K_ENTER_77_FAST[] = {
  "sync_p_lo",
  "sync_p_hi",
};
static const char* K_ENTER_77_WIDE[] = {
  "sync_p_lo",
  "sync_p_hi",
  "op",
  "count",
  "owner",
};
static const char* K_ENTER_78_FAST[] = {
  "type",
  "sync_p",
};
static const char* K_ENTER_78_WIDE[] = {
  "type",
  "sync_p",
  "count",
  "owner",
  "protocol",
  "flags",
  "prioceiling",
  "clockid",
};
static const char* K_ENTER_79_FAST[] = {
  "sync_p",
  "owner",
};
static const char* K_ENTER_79_WIDE[] = {
  "sync_p",
  "count",
  "owner",
};
static const char* K_ENTER_80_FAST[] = {
  "sync_p",
  "owner",
};
static const char* K_ENTER_80_WIDE[] = {
  "sync_p",
  "count",
  "owner",
};
static const char* K_ENTER_81_FAST[] = {
  "sync_p",
  "owner",
};
static const char* K_ENTER_81_WIDE[] = {
  "sync_p",
  "count",
  "owner",
};
static const char* K_ENTER_82_FAST[] = {
  "sync_p",
  "mutex_p",
};
static const char* K_ENTER_82_WIDE[] = {
  "sync_p",
  "mutex_p",
  "sync_count",
  "sync_owner",
  "mutex_count",
  "mutex_owner",
};
static const char* K_ENTER_83_FAST[] = {
  "sync_p",
  "all",
};
static const char* K_ENTER_83_WIDE[] = {
  "sync_p",
  "all",
  "sync_count",
  "sync_owner",
};
static const char* K_ENTER_84_FAST[] = {
  "sync_p",
  "count",
};
static const char* K_ENTER_84_WIDE[] = {
  "sync_p",
  "count",
  "owner",
};
static const char* K_ENTER_85_FAST[] = {
  "sync_p",
  "count",
};
static const char* K_ENTER_85_WIDE[] = {
  "sync_p",
  "try",
  "count",
  "owner",
};
static const char* K_ENTER_86_FAST[] = {
  "cmd",
  "sync_p",
};
static const char* K_ENTER_86_WIDE[] = {
  "cmd",
  "sync_p",
  "data_p",
  "count",
  "owner",
};
static const char* K_ENTER_87_FAST[] = {
  "sync_p",
  "owner",
};
static const char* K_ENTER_87_WIDE[] = {
  "sync_p",
  "count",
  "owner",
};
static const char* K_ENTER_88_FAST[] = {
  "pid",
  "tid",
};
static const char* K_ENTER_88_WIDE[] = {
  "pid",
  "tid",
};
static const char* K_ENTER_89_FAST[] = {
  "pid",
  "sched_priority",
};
static const char* K_ENTER_89_WIDE[] = {
  "pid",
  "tid",
  "policy",
  "sched_priority",
  "sched_curpriority",
  "ss_low_prio",
  "ss_max_repl",
  "ss_repl_sec",
  "ss_repl_nsec",
  "ss_budget_sec",
  "ss_budget_nsec",
};
static const char* K_ENTER_90_FAST[] = {
  "empty",
  "empty",
};
static const char* K_ENTER_90_WIDE[] = {
  "empty",
  "empty",
};
static const char* K_ENTER_91_FAST[] = {
  "pid",
  "policy",
};
static const char* K_ENTER_91_WIDE[] = {
  "pid",
  "policy",
};
static const char* K_ENTER_92_FAST[] = {
  "cmd",
  "data_p",
};
static const char* K_ENTER_92_WIDE[] = {
  "cmd",
  "data_p",
};
static const char* K_ENTER_93_FAST[] = {
  "d1",
  "d2",
};
static const char* K_ENTER_93_WIDE[] = {
  "d1",
  "d2",
};
static const char* K_ENTER_94_FAST[] = {
  "fd",
  "msg[0]",
};
static const char* K_ENTER_94_WIDE[] = {
  "fd",
  "msg[0]",
  "msglen_lo",
  "msglen_hi",
  "priority",
};
static const char* K_ENTER_95_FAST[] = {
  "fd",
  "empty",
};
static const char* K_ENTER_95_WIDE[] = {
  "fd",
  "msglen_lo",
  "msglen_hi",
  "msg_lo",
  "msg_hi",
  "prio_lo",
  "prio_hi",
};
static const char* K_ENTER_96_FAST[] = {
  "rcvid_lo",
  "rcvid_hi",
};
static const char* K_ENTER_96_WIDE[] = {
  "rcvid_lo",
  "rcvid_hi",
  "queueid",
};
static const char* K_ENTER_97_FAST[] = {
  "fd",
  "cmd",
};
static const char* K_ENTER_97_WIDE[] = {
  "fd",
  "cmd",
};
static const char* K_ENTER_100_FAST[] = {
  "d1",
  "d2",
};
static const char* K_ENTER_100_WIDE[] = {
  "d1",
  "d2",
};
static const char* K_ENTER_104_FAST[] = {
  "sync_p",
  "owner",
};
static const char* K_ENTER_104_WIDE[] = {
  "sync_p",
  "count",
  "owner",
};
static const char* K_ENTER_106_FAST[] = {
  "empty",
  "empty",
};
static const char* K_ENTER_106_WIDE[] = {
  "empty",
  "empty",
};
static const char* K_ENTER_107_FAST[] = {
  "empty",
  "empty",
};
static const char* K_ENTER_107_WIDE[] = {
  "empty",
  "empty",
};
static const char* K_EXIT_0_FAST[] = {
  "empty",
  "empty",
};
static const char* K_EXIT_0_WIDE[] = {
  "empty",
  "empty",
};
static const char* K_EXIT_1_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_1_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_2_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_2_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_3_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_3_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_5_FAST[] = {
  "empty",
  "empty",
};
static const char* K_EXIT_5_WIDE[] = {
  "empty",
  "empty",
};
static const char* K_EXIT_6_FAST[] = {
  "status",
  "empty",
};
static const char* K_EXIT_6_WIDE[] = {
  "status",
  "empty",
};
static const char* K_EXIT_7_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_7_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_8_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_8_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_9_FAST[] = {
  "empty",
  "empty",
};
static const char* K_EXIT_9_WIDE[] = {
  "empty",
  "empty",
};
static const char* K_EXIT_10_FAST[] = {
  "empty",
  "empty",
};
static const char* K_EXIT_10_WIDE[] = {
  "empty",
  "empty",
};
static const char* K_EXIT_11_FAST[] = {
  "status",
  "rmsg[0]",
};
static const char* K_EXIT_11_WIDE[] = {
  "status",
  "rmsg[0]",
  "rmsg[1]",
  "rmsg[2]",
};
static const char* K_EXIT_12_FAST[] = {
  "ret_val",
  "rmsg[0]",
};
static const char* K_EXIT_12_WIDE[] = {
  "ret_val",
  "rmsg[0]",
  "rmsg[1]",
  "rmsg[2]",
};
static const char* K_EXIT_13_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_13_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_14_FAST[] = {
  "rcvid",
  "rmsg[0]",
};
static const char* K_EXIT_14_WIDE[] = {
  "rcvid",
  "rmsg[0]",
  "rmsg[1]",
  "rmsg[2]",
  "empty",
  "empty",
  "info_pid",
  "info_tid",
  "info_chid",
  "info_scoid",
  "info_coid",
  "info_msglen",
  "info_srcmsglen",
  "info_dstmsglen",
  "info_priority",
  "info_flags",
  "info_reserved",
};
static const char* K_EXIT_15_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_15_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_16_FAST[] = {
  "rbytes",
  "rmsg[0]",
};
static const char* K_EXIT_16_WIDE[] = {
  "rbytes",
  "rmsg[0]",
  "rmsg[1]",
  "rmsg[2]",
};
static const char* K_EXIT_17_FAST[] = {
  "wbytes",
  "empty",
};
static const char* K_EXIT_17_WIDE[] = {
  "wbytes",
  "empty",
};
static const char* K_EXIT_19_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_19_WIDE[] = {
  "ret_val",
  "empty",
  "empty",
  "info_pid",
  "info_tid",
  "info_chid",
  "info_scoid",
  "info_coid",
  "info_msglen",
  "info_srcmsglen",
  "info_dstmsglen",
  "info_priority",
  "info_flags",
  "info_reserved",
};
static const char* K_EXIT_20_FAST[] = {
  "status",
  "empty",
};
static const char* K_EXIT_20_WIDE[] = {
  "status",
  "empty",
};
static const char* K_EXIT_21_FAST[] = {
  "ret_val",
  "eventp",
};
static const char* K_EXIT_21_WIDE[] = {
  "ret_val",
  "eventp",
};
static const char* K_EXIT_24_FAST[] = {
  "rcvid",
  "rmsg[0]",
};
static const char* K_EXIT_24_WIDE[] = {
  "rcvid",
  "rmsg[0]",
  "rmsg[1]",
  "rmsg[2]",
  "empty",
  "empty",
  "info_pid",
  "info_tid",
  "info_chid",
  "info_scoid",
  "info_coid",
  "info_msglen",
  "info_srcmsglen",
  "info_dstmsglen",
  "info_priority",
  "info_flags",
  "info_reserved",
};
static const char* K_EXIT_26_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_26_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_27_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_27_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_29_FAST[] = {
  "ret_val",
  "handler_p",
};
static const char* K_EXIT_29_WIDE[] = {
  "ret_val",
  "handler_p",
  "sa_flags",
  "sa_mask_0",
  "sa_mask_1",
};
static const char* K_EXIT_30_FAST[] = {
  "ret_val",
  "sig_blocked_0",
};
static const char* K_EXIT_30_WIDE[] = {
  "ret_val",
  "sig_blocked_0",
  "sig_blocked_1",
};
static const char* K_EXIT_31_FAST[] = {
  "ret_val",
  "sig_blocked_p",
};
static const char* K_EXIT_31_WIDE[] = {
  "ret_val",
  "sig_blocked_p",
};
static const char* K_EXIT_32_FAST[] = {
  "sig_num",
  "si_code",
};
static const char* K_EXIT_32_WIDE[] = {
  "sig_num",
  "si_signo",
  "si_code",
  "si_errno",
  "p[0]",
  "p[1]",
  "p[2]",
  "p[3]",
  "p[4]",
  "p[5]",
  "p[6]",
};
static const char* K_EXIT_33_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_33_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_35_FAST[] = {
  "chid",
  "empty",
};
static const char* K_EXIT_35_WIDE[] = {
  "chid",
  "empty",
};
static const char* K_EXIT_36_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_36_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_37_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_37_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_39_FAST[] = {
  "coid",
  "empty",
};
static const char* K_EXIT_39_WIDE[] = {
  "coid",
  "empty",
};
static const char* K_EXIT_40_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_40_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_41_FAST[] = {
  "coid",
  "empty",
};
static const char* K_EXIT_41_WIDE[] = {
  "coid",
  "empty",
  "empty",
  "info_pid",
  "info_tid",
  "info_chid",
  "info_scoid",
  "info_coid",
  "info_msglen",
  "info_srcmsglen",
  "info_dstmsglen",
  "info_priority",
  "info_flags",
  "info_reserved",
};
static const char* K_EXIT_42_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_42_WIDE[] = {
  "ret_val",
  "empty",
  "info_pid",
  "info_sid",
  "flags",
  "info_ruid",
  "info_euid",
  "info_suid",
  "info_rgid",
  "info_egid",
  "info_sgid",
  "info_ngroups",
  "group[0]",
  "group[1]",
  "group[2]",
  "group[3]",
  "group[4]",
  "group[5]",
  "group[6]",
  "group[7]",
};
static const char* K_EXIT_43_FAST[] = {
  "old_flags",
  "empty",
};
static const char* K_EXIT_43_WIDE[] = {
  "old_flags",
  "empty",
};
static const char* K_EXIT_46_FAST[] = {
  "thread_id",
  "owner",
};
static const char* K_EXIT_46_WIDE[] = {
  "thread_id",
  "owner",
};
static const char* K_EXIT_47_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_47_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_49_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_49_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_50_FAST[] = {
  "ret_val",
  "status_p",
};
static const char* K_EXIT_50_WIDE[] = {
  "ret_val",
  "status_p",
};
static const char* K_EXIT_51_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_51_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_52_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_52_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_53_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_53_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_55_FAST[] = {
  "int_fun_id",
  "empty",
};
static const char* K_EXIT_55_WIDE[] = {
  "int_fun_id",
  "empty",
};
static const char* K_EXIT_56_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_56_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_57_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_57_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_58_FAST[] = {
  "ret_val",
  "timeout_p",
};
static const char* K_EXIT_58_WIDE[] = {
  "ret_val",
  "timeout_p",
};
static const char* K_EXIT_59_FAST[] = {
  "mask_level",
  "empty",
};
static const char* K_EXIT_59_WIDE[] = {
  "mask_level",
  "empty",
};
static const char* K_EXIT_60_FAST[] = {
  "mask_level",
  "empty",
};
static const char* K_EXIT_60_WIDE[] = {
  "mask_level",
  "empty",
};
static const char* K_EXIT_61_FAST[] = {
  "ret_val",
  "old",
};
static const char* K_EXIT_61_WIDE[] = {
  "ret_val",
  "old",
};
static const char* K_EXIT_62_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_62_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_65_FAST[] = {
  "ret_val",
  "old_sec",
};
static const char* K_EXIT_65_WIDE[] = {
  "ret_val",
  "old_sec",
  "old_nsec",
};
static const char* K_EXIT_66_FAST[] = {
  "ret_val",
  "old_tick_count",
};
static const char* K_EXIT_66_WIDE[] = {
  "ret_val",
  "old_tick_count",
  "old_tick_nsec_inc",
};
static const char* K_EXIT_67_FAST[] = {
  "ret_val",
  "old_nsec",
};
static const char* K_EXIT_67_WIDE[] = {
  "ret_val",
  "old_nsec",
  "old_fract",
};
static const char* K_EXIT_68_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_68_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_70_FAST[] = {
  "timer_id",
  "empty",
};
static const char* K_EXIT_70_WIDE[] = {
  "timer_id",
  "empty",
};
static const char* K_EXIT_71_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_71_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_72_FAST[] = {
  "ret_val",
  "oitime_sec",
};
static const char* K_EXIT_72_WIDE[] = {
  "ret_val",
  "oitime_sec",
  "oitime_nsec",
  "oitime_interval_sec",
  "oitime_interval_nsec",
};
static const char* K_EXIT_73_FAST[] = {
  "prev_id",
  "itime_nsec",
};
static const char* K_EXIT_73_WIDE[] = {
  "prev_id",
  "itime_nsec",
  "itime_interval_nsec",
  "otime_nsec",
  "otime_interval_nsec",
  "flags",
  "tid",
  "notify",
  "clockid",
  "overruns",
  "sigev_notify",
  "sigev_func_p",
  "sigev_value",
  "sigev_attr_p",
};
static const char* K_EXIT_74_FAST[] = {
  "ret_val",
  "otime_sec",
};
static const char* K_EXIT_74_WIDE[] = {
  "ret_val",
  "otime_sec",
  "otime_nsec",
  "interval_sec",
  "interval_nsec",
};
static const char* K_EXIT_75_FAST[] = {
  "prev_timeout_flags",
  "otime_sec",
};
static const char* K_EXIT_75_WIDE[] = {
  "prev_timeout_flags",
  "otime_sec",
  "otime_nsec",
};
static const char* K_EXIT_76_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_76_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_77_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_77_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_78_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_78_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_79_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_79_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_80_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_80_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_81_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_81_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_82_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_82_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_83_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_83_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_84_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_84_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_85_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_85_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_86_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_86_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_87_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_87_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_88_FAST[] = {
  "ret_val",
  "sched_priority",
};
static const char* K_EXIT_88_WIDE[] = {
  "ret_val",
  "sched_priority",
  "sched_curpriority",
  "ss_low_prio",
  "ss_max_repl",
  "ss_repl_sec",
  "ss_repl_nsec",
  "ss_budget_sec",
  "ss_budget_nsec",
};
static const char* K_EXIT_89_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_89_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_90_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_90_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_91_FAST[] = {
  "ret_val",
  "priority_max",
};
static const char* K_EXIT_91_WIDE[] = {
  "ret_val",
  "priority_min",
  "priority_max",
  "interval_sec",
  "interval_nsec",
  "priority_priv",
};
static const char* K_EXIT_92_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_92_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_93_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_93_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_94_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_94_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_95_FAST[] = {
  "msglen",
  "msg[0]",
};
static const char* K_EXIT_95_WIDE[] = {
  "msglen",
  "msg[0]",
  "priority",
};
static const char* K_EXIT_96_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_96_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_97_FAST[] = {
  "ret_val",
  "cmd",
};
static const char* K_EXIT_97_WIDE[] = {
  "ret_val",
  "cmd",
};
static const char* K_EXIT_100_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_100_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_104_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_104_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_106_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_106_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_107_FAST[] = {
  "ret_val",
  "empty",
};
static const char* K_EXIT_107_WIDE[] = {
  "ret_val",
  "empty",
};
static const char* K_ENTER64_3_WIDE[] = {
  "addr_lo",
  "addr_hi",
  "nlines_lo",
  "nlines_hi",
  "flags",
  "index",
};
static const char* K_ENTER64_6_WIDE[] = {
  "coid",
  "priority",
  "code",
  "value_lo",
  "value_hi",
};
static const char* K_ENTER64_11_WIDE[] = {
  "coid",
  "sparts_lo",
  "sparts_hi",
  "rparts_lo",
  "rparts_hi",
  "msg[0]",
  "msg[1]",
  "msg[2]",
};
static const char* K_ENTER64_12_WIDE[] = {
  "coid",
  "sparts_lo",
  "sparts_hi",
  "rparts_lo",
  "rparts_hi",
  "msg[0]",
  "msg[1]",
  "msg[2]",
};
static const char* K_ENTER64_14_WIDE[] = {
  "chid",
  "rparts_lo",
  "rparts_hi",
};
static const char* K_ENTER64_15_WIDE[] = {
  "rcvid",
  "sparts_lo",
  "sparts_hi",
  "status_lo",
  "status_hi",
  "smsg[0]",
  "smsg[1]",
  "smsg[2]",
};
static const char* K_ENTER64_16_WIDE[] = {
  "rcvid",
  "rmsg_p_lo",
  "rmsg_p_hi",
  "rparts_lo",
  "rparts_hi",
  "offset_lo",
  "offset_hi",
};
static const char* K_ENTER64_17_WIDE[] = {
  "rcvid",
  "sparts_lo",
  "sparts_hi",
  "offset_lo",
  "offset_hi",
  "msg[0]",
  "msg[1]",
  "msg[2]",
};
static const char* K_ENTER64_20_WIDE[] = {
  "coid",
  "priority",
  "code",
  "value_lo",
  "value_hi",
};
static const char* K_ENTER64_21_WIDE[] = {
  "rcvid",
  "sigev_notify",
  "pad",
  "sigev_func_p_lo",
  "sigev_func_p_hi",
  "sigev_value_lo",
  "sigev_value_hi",
  "sigev_attr_p_lo",
  "sigev_attr_p_hi",
};
static const char* K_ENTER64_24_WIDE[] = {
  "chid",
  "rparts_lo",
  "rparts_hi",
};
static const char* K_ENTER64_26_WIDE[] = {
  "nd",
  "pid",
  "tid",
  "signo",
  "code",
  "value_lo",
  "value_hi",
};
static const char* K_ENTER64_27_WIDE[] = {
  "signal_p_lo",
  "signal_p_hi",
};
static const char* K_ENTER64_29_WIDE[] = {
  "pid",
  "sigstub_p_lo",
  "sigstub_p_hi",
  "signo",
  "handler_p_lo",
  "handler_p_hi",
  "sa_flags",
  "sa_mask_0",
  "sa_mask_1",
};
static const char* K_ENTER64_33_WIDE[] = {
  "nd",
  "pid",
  "tid",
  "signo",
  "code",
  "value_lo",
  "value_hi",
};
static const char* K_ENTER64_46_WIDE[] = {
  "pid",
  "func_p_lo",
  "func_p_hi",
  "arg_p_lo",
  "arg_p_hi",
  "flags",
  "stacksize",
  "stackaddr_p_lo",
  "stackaddr_p_hi",
  "exitfunc_p_lo",
  "exitfunc_p_hi",
  "policy",
  "priority",
  "curpriority",
  "ss_low_prio",
  "ss_max_repl",
  "ss_repl_sec",
  "ss_repl_nsec",
  "ss_budget_sec",
  "ss_budget_nsec",
  "guardsize",
};
static const char* K_ENTER64_47_WIDE[] = {
  "tid",
  "priority",
  "status_p_lo",
  "status_p_hi",
};
static const char* K_ENTER64_51_WIDE[] = {
  "tid",
  "canstub_p_lo",
  "canstub_p_hi",
};
static const char* K_ENTER64_52_WIDE[] = {
  "cmd",
  "data_p_lo",
  "data_p_hi",
};
static const char* K_ENTER64_53_WIDE[] = {
  "pid",
  "tid",
  "cmd",
  "data_p_lo",
  "data_p_hi",
};
static const char* K_ENTER64_55_WIDE[] = {
  "intr",
  "handler_p_lo",
  "handler_p_hi",
  "area_p_lo",
  "area_p_hi",
  "areasize",
  "flags",
};
static const char* K_ENTER64_70_WIDE[] = {
  "timer_id",
  "sigev_notify",
  "pad",
  "sigev_func_p_lo",
  "sigev_func_p_hi",
  "sigev_value_lo",
  "sigev_value_hi",
  "sigev_attr_p_lo",
  "sigev_attr_p_hi",
};
static const char* K_ENTER64_73_WIDE[] = {
  "pid",
  "id",
  "flags",
  "info_p_lo",
  "info_p_hi",
};
static const char* K_ENTER64_75_WIDE[] = {
  "clock_id",
  "timeout_flags",
  "ntime_sec",
  "ntime_nsec",
  "sigev_notify",
  "pad",
  "sigev_func_p_lo",
  "sigev_func_p_hi",
  "sigev_value_lo",
  "sigev_value_hi",
  "sigev_attr_p_lo",
  "sigev_attr_p_hi",
};
static const char* K_ENTER64_78_WIDE[] = {
  "type",
  "sync_p_lo",
  "sync_p_hi",
  "count",
  "owner",
  "protocol",
  "flags",
  "prioceiling",
  "clockid",
};
static const char* K_ENTER64_79_WIDE[] = {
  "sync_p_lo",
  "sync_p_hi",
  "count",
  "owner",
};
static const char* K_ENTER64_80_WIDE[] = {
  "sync_p_lo",
  "sync_p_hi",
  "count",
  "owner",
};
static const char* K_ENTER64_81_WIDE[] = {
  "sync_p_lo",
  "sync_p_hi",
  "count",
  "owner",
};
static const char* K_ENTER64_82_WIDE[] = {
  "sync_p_lo",
  "sync_p_hi",
  "mutex_p_lo",
  "mutex_p_hi",
  "sync_count",
  "sync_owner",
  "mutex_count",
  "mutex_owner",
};
static const char* K_ENTER64_83_WIDE[] = {
  "sync_p_lo",
  "sync_p_hi",
  "all",
  "sync_count",
  "sync_owner",
};
static const char* K_ENTER64_84_WIDE[] = {
  "sync_p_lo",
  "sync_p_hi",
  "count",
  "owner",
};
static const char* K_ENTER64_85_WIDE[] = {
  "sync_p_lo",
  "sync_p_hi",
  "try",
  "count",
  "owner",
};
static const char* K_ENTER64_86_WIDE[] = {
  "cmd",
  "sync_p_lo",
  "sync_p_hi",
  "data_p_lo",
  "data_p_hi",
  "count",
  "owner",
};
static const char* K_ENTER64_92_WIDE[] = {
  "cmd",
  "data_p_lo",
  "data_p_hi",
};
static const char* K_EXIT64_11_WIDE[] = {
  "status_lo",
  "status_hi",
  "rmsg[0]",
  "rmsg[1]",
  "rmsg[2]",
};
static const char* K_EXIT64_12_WIDE[] = {
  "ret_val_lo",
  "ret_val_hi",
  "rmsg[0]",
  "rmsg[1]",
  "rmsg[2]",
};
static const char* K_EXIT64_14_WIDE[] = {
  "rcvid",
  "rmsg[0]",
  "rmsg[1]",
  "rmsg[2]",
  "info_nd",
  "info_srcnd",
  "info_pid",
  "info_tid",
  "info_chid",
  "info_scoid",
  "info_coid",
  "info_msglen_lo",
  "info_msglen_hi",
  "info_srcmsglen_lo",
  "info_srcmsglen_hi",
  "info_dstmsglen_lo",
  "info_dstmsglen_hi",
  "info_priority",
  "info_flags",
  "info_reserved_0",
  "info_reserved_1",
};
static const char* K_EXIT64_16_WIDE[] = {
  "rbytes_lo",
  "rbytes_hi",
  "rmsg[0]",
  "rmsg[1]",
  "rmsg[2]",
};
static const char* K_EXIT64_17_WIDE[] = {
  "wbytes_lo",
  "wbytes_hi",
};
static const char* K_EXIT64_21_WIDE[] = {
  "ret_val",
  "eventp_lo",
  "eventp_hi",
};
static const char* K_EXIT64_24_WIDE[] = {
  "rcvid",
  "rmsg[0]",
  "rmsg[1]",
  "rmsg[2]",
  "info_nd",
  "info_srcnd",
  "info_pid",
  "info_tid",
  "info_chid",
  "info_scoid",
  "info_coid",
  "info_msglen_lo",
  "info_msglen_hi",
  "info_srcmsglen_lo",
  "info_srcmsglen_hi",
  "info_dstmsglen_lo",
  "info_dstmsglen_hi",
  "info_priority",
  "info_flags",
  "info_reserved_0",
  "info_reserved_1",
};
static const char* K_EXIT64_29_WIDE[] = {
  "ret_val",
  "handler_p_lo",
  "handler_p_hi",
  "sa_flags",
  "sa_mask_0",
  "sa_mask_1",
};
static const char* K_EXIT64_50_WIDE[] = {
  "ret_val",
  "status_p_lo",
  "status_p_hi",
};
static const char* K_EXIT64_58_WIDE[] = {
  "ret_val",
  "timeout_p_lo",
  "timeout_p_hi",
};
static const KercallFields KERCALL_ENTER_LOOKUP[kKercallTableSize] = {
  { K_ENTER_0_FAST, 2, K_ENTER_0_WIDE, 2 },
  { K_ENTER_1_FAST, 2, K_ENTER_1_WIDE, 5 },
  { K_ENTER_2_FAST, 2, K_ENTER_2_WIDE, 2 },
  { K_ENTER_3_FAST, 2, K_ENTER_3_WIDE, 3 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_5_FAST, 2, K_ENTER_5_WIDE, 2 },
  { K_ENTER_6_FAST, 2, K_ENTER_6_WIDE, 4 },
  { K_ENTER_7_FAST, 2, K_ENTER_7_WIDE, 2 },
  { K_ENTER_8_FAST, 2, K_ENTER_8_WIDE, 2 },
  { K_ENTER_9_FAST, 2, K_ENTER_9_WIDE, 2 },
  { K_ENTER_10_FAST, 2, K_ENTER_10_WIDE, 2 },
  { K_ENTER_11_FAST, 2, K_ENTER_11_WIDE, 6 },
  { K_ENTER_12_FAST, 2, K_ENTER_12_WIDE, 6 },
  { K_ENTER_13_FAST, 2, K_ENTER_13_WIDE, 2 },
  { K_ENTER_14_FAST, 2, K_ENTER_14_WIDE, 2 },
  { K_ENTER_15_FAST, 2, K_ENTER_15_WIDE, 6 },
  { K_ENTER_16_FAST, 2, K_ENTER_16_WIDE, 4 },
  { K_ENTER_17_FAST, 2, K_ENTER_17_WIDE, 6 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_19_FAST, 2, K_ENTER_19_WIDE, 2 },
  { K_ENTER_20_FAST, 2, K_ENTER_20_WIDE, 4 },
  { K_ENTER_21_FAST, 2, K_ENTER_21_WIDE, 5 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_24_FAST, 2, K_ENTER_24_WIDE, 2 },
  { K_ENTER_25_FAST, 2, K_ENTER_25_WIDE, 4 },
  { K_ENTER_26_FAST, 2, K_ENTER_26_WIDE, 6 },
  { K_ENTER_27_FAST, 2, K_ENTER_27_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_29_FAST, 2, K_ENTER_29_WIDE, 7 },
  { K_ENTER_30_FAST, 2, K_ENTER_30_WIDE, 5 },
  { K_ENTER_31_FAST, 2, K_ENTER_31_WIDE, 2 },
  { K_ENTER_32_FAST, 2, K_ENTER_32_WIDE, 2 },
  { K_ENTER_33_FAST, 2, K_ENTER_33_WIDE, 6 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_35_FAST, 2, K_ENTER_35_WIDE, 2 },
  { K_ENTER_36_FAST, 2, K_ENTER_36_WIDE, 2 },
  { K_ENTER_37_FAST, 2, K_ENTER_37_WIDE, 3 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_39_FAST, 2, K_ENTER_39_WIDE, 5 },
  { K_ENTER_40_FAST, 2, K_ENTER_40_WIDE, 2 },
  { K_ENTER_41_FAST, 2, K_ENTER_41_WIDE, 2 },
  { K_ENTER_42_FAST, 2, K_ENTER_42_WIDE, 2 },
  { K_ENTER_43_FAST, 2, K_ENTER_43_WIDE, 4 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_46_FAST, 2, K_ENTER_46_WIDE, 17 },
  { K_ENTER_47_FAST, 2, K_ENTER_47_WIDE, 3 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_49_FAST, 2, K_ENTER_49_WIDE, 2 },
  { K_ENTER_50_FAST, 2, K_ENTER_50_WIDE, 2 },
  { K_ENTER_51_FAST, 2, K_ENTER_51_WIDE, 2 },
  { K_ENTER_52_FAST, 2, K_ENTER_52_WIDE, 2 },
  { K_ENTER_53_FAST, 2, K_ENTER_53_WIDE, 4 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_55_FAST, 2, K_ENTER_55_WIDE, 5 },
  { K_ENTER_56_FAST, 2, K_ENTER_56_WIDE, 2 },
  { K_ENTER_57_FAST, 2, K_ENTER_57_WIDE, 2 },
  { K_ENTER_58_FAST, 2, K_ENTER_58_WIDE, 3 },
  { K_ENTER_59_FAST, 2, K_ENTER_59_WIDE, 2 },
  { K_ENTER_60_FAST, 2, K_ENTER_60_WIDE, 2 },
  { K_ENTER_61_FAST, 2, K_ENTER_61_WIDE, 3 },
  { K_ENTER_62_FAST, 2, K_ENTER_62_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_65_FAST, 2, K_ENTER_65_WIDE, 3 },
  { K_ENTER_66_FAST, 2, K_ENTER_66_WIDE, 3 },
  { K_ENTER_67_FAST, 2, K_ENTER_67_WIDE, 3 },
  { K_ENTER_68_FAST, 2, K_ENTER_68_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_70_FAST, 2, K_ENTER_70_WIDE, 5 },
  { K_ENTER_71_FAST, 2, K_ENTER_71_WIDE, 2 },
  { K_ENTER_72_FAST, 2, K_ENTER_72_WIDE, 6 },
  { K_ENTER_73_FAST, 2, K_ENTER_73_WIDE, 4 },
  { K_ENTER_74_FAST, 2, K_ENTER_74_WIDE, 5 },
  { K_ENTER_75_FAST, 2, K_ENTER_75_WIDE, 8 },
  { K_ENTER_76_FAST, 2, K_ENTER_76_WIDE, 2 },
  { K_ENTER_77_FAST, 2, K_ENTER_77_WIDE, 5 },
  { K_ENTER_78_FAST, 2, K_ENTER_78_WIDE, 8 },
  { K_ENTER_79_FAST, 2, K_ENTER_79_WIDE, 3 },
  { K_ENTER_80_FAST, 2, K_ENTER_80_WIDE, 3 },
  { K_ENTER_81_FAST, 2, K_ENTER_81_WIDE, 3 },
  { K_ENTER_82_FAST, 2, K_ENTER_82_WIDE, 6 },
  { K_ENTER_83_FAST, 2, K_ENTER_83_WIDE, 4 },
  { K_ENTER_84_FAST, 2, K_ENTER_84_WIDE, 3 },
  { K_ENTER_85_FAST, 2, K_ENTER_85_WIDE, 4 },
  { K_ENTER_86_FAST, 2, K_ENTER_86_WIDE, 5 },
  { K_ENTER_87_FAST, 2, K_ENTER_87_WIDE, 3 },
  { K_ENTER_88_FAST, 2, K_ENTER_88_WIDE, 2 },
  { K_ENTER_89_FAST, 2, K_ENTER_89_WIDE, 11 },
  { K_ENTER_90_FAST, 2, K_ENTER_90_WIDE, 2 },
  { K_ENTER_91_FAST, 2, K_ENTER_91_WIDE, 2 },
  { K_ENTER_92_FAST, 2, K_ENTER_92_WIDE, 2 },
  { K_ENTER_93_FAST, 2, K_ENTER_93_WIDE, 2 },
  { K_ENTER_94_FAST, 2, K_ENTER_94_WIDE, 5 },
  { K_ENTER_95_FAST, 2, K_ENTER_95_WIDE, 7 },
  { K_ENTER_96_FAST, 2, K_ENTER_96_WIDE, 3 },
  { K_ENTER_97_FAST, 2, K_ENTER_97_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_100_FAST, 2, K_ENTER_100_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_104_FAST, 2, K_ENTER_104_WIDE, 3 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_ENTER_106_FAST, 2, K_ENTER_106_WIDE, 2 },
  { K_ENTER_107_FAST, 2, K_ENTER_107_WIDE, 2 },
};

static const KercallFields KERCALL_EXIT_LOOKUP[kKercallTableSize] = {
  { K_EXIT_0_FAST, 2, K_EXIT_0_WIDE, 2 },
  { K_EXIT_1_FAST, 2, K_EXIT_1_WIDE, 2 },
  { K_EXIT_2_FAST, 2, K_EXIT_2_WIDE, 2 },
  { K_EXIT_3_FAST, 2, K_EXIT_3_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_5_FAST, 2, K_EXIT_5_WIDE, 2 },
  { K_EXIT_6_FAST, 2, K_EXIT_6_WIDE, 2 },
  { K_EXIT_7_FAST, 2, K_EXIT_7_WIDE, 2 },
  { K_EXIT_8_FAST, 2, K_EXIT_8_WIDE, 2 },
  { K_EXIT_9_FAST, 2, K_EXIT_9_WIDE, 2 },
  { K_EXIT_10_FAST, 2, K_EXIT_10_WIDE, 2 },
  { K_EXIT_11_FAST, 2, K_EXIT_11_WIDE, 4 },
  { K_EXIT_12_FAST, 2, K_EXIT_12_WIDE, 4 },
  { K_EXIT_13_FAST, 2, K_EXIT_13_WIDE, 2 },
  { K_EXIT_14_FAST, 2, K_EXIT_14_WIDE, 17 },
  { K_EXIT_15_FAST, 2, K_EXIT_15_WIDE, 2 },
  { K_EXIT_16_FAST, 2, K_EXIT_16_WIDE, 4 },
  { K_EXIT_17_FAST, 2, K_EXIT_17_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_19_FAST, 2, K_EXIT_19_WIDE, 14 },
  { K_EXIT_20_FAST, 2, K_EXIT_20_WIDE, 2 },
  { K_EXIT_21_FAST, 2, K_EXIT_21_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_24_FAST, 2, K_EXIT_24_WIDE, 17 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_26_FAST, 2, K_EXIT_26_WIDE, 2 },
  { K_EXIT_27_FAST, 2, K_EXIT_27_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_29_FAST, 2, K_EXIT_29_WIDE, 5 },
  { K_EXIT_30_FAST, 2, K_EXIT_30_WIDE, 3 },
  { K_EXIT_31_FAST, 2, K_EXIT_31_WIDE, 2 },
  { K_EXIT_32_FAST, 2, K_EXIT_32_WIDE, 11 },
  { K_EXIT_33_FAST, 2, K_EXIT_33_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_35_FAST, 2, K_EXIT_35_WIDE, 2 },
  { K_EXIT_36_FAST, 2, K_EXIT_36_WIDE, 2 },
  { K_EXIT_37_FAST, 2, K_EXIT_37_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_39_FAST, 2, K_EXIT_39_WIDE, 2 },
  { K_EXIT_40_FAST, 2, K_EXIT_40_WIDE, 2 },
  { K_EXIT_41_FAST, 2, K_EXIT_41_WIDE, 14 },
  { K_EXIT_42_FAST, 2, K_EXIT_42_WIDE, 20 },
  { K_EXIT_43_FAST, 2, K_EXIT_43_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_46_FAST, 2, K_EXIT_46_WIDE, 2 },
  { K_EXIT_47_FAST, 2, K_EXIT_47_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_49_FAST, 2, K_EXIT_49_WIDE, 2 },
  { K_EXIT_50_FAST, 2, K_EXIT_50_WIDE, 2 },
  { K_EXIT_51_FAST, 2, K_EXIT_51_WIDE, 2 },
  { K_EXIT_52_FAST, 2, K_EXIT_52_WIDE, 2 },
  { K_EXIT_53_FAST, 2, K_EXIT_53_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_55_FAST, 2, K_EXIT_55_WIDE, 2 },
  { K_EXIT_56_FAST, 2, K_EXIT_56_WIDE, 2 },
  { K_EXIT_57_FAST, 2, K_EXIT_57_WIDE, 2 },
  { K_EXIT_58_FAST, 2, K_EXIT_58_WIDE, 2 },
  { K_EXIT_59_FAST, 2, K_EXIT_59_WIDE, 2 },
  { K_EXIT_60_FAST, 2, K_EXIT_60_WIDE, 2 },
  { K_EXIT_61_FAST, 2, K_EXIT_61_WIDE, 2 },
  { K_EXIT_62_FAST, 2, K_EXIT_62_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_65_FAST, 2, K_EXIT_65_WIDE, 3 },
  { K_EXIT_66_FAST, 2, K_EXIT_66_WIDE, 3 },
  { K_EXIT_67_FAST, 2, K_EXIT_67_WIDE, 3 },
  { K_EXIT_68_FAST, 2, K_EXIT_68_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_70_FAST, 2, K_EXIT_70_WIDE, 2 },
  { K_EXIT_71_FAST, 2, K_EXIT_71_WIDE, 2 },
  { K_EXIT_72_FAST, 2, K_EXIT_72_WIDE, 5 },
  { K_EXIT_73_FAST, 2, K_EXIT_73_WIDE, 14 },
  { K_EXIT_74_FAST, 2, K_EXIT_74_WIDE, 5 },
  { K_EXIT_75_FAST, 2, K_EXIT_75_WIDE, 3 },
  { K_EXIT_76_FAST, 2, K_EXIT_76_WIDE, 2 },
  { K_EXIT_77_FAST, 2, K_EXIT_77_WIDE, 2 },
  { K_EXIT_78_FAST, 2, K_EXIT_78_WIDE, 2 },
  { K_EXIT_79_FAST, 2, K_EXIT_79_WIDE, 2 },
  { K_EXIT_80_FAST, 2, K_EXIT_80_WIDE, 2 },
  { K_EXIT_81_FAST, 2, K_EXIT_81_WIDE, 2 },
  { K_EXIT_82_FAST, 2, K_EXIT_82_WIDE, 2 },
  { K_EXIT_83_FAST, 2, K_EXIT_83_WIDE, 2 },
  { K_EXIT_84_FAST, 2, K_EXIT_84_WIDE, 2 },
  { K_EXIT_85_FAST, 2, K_EXIT_85_WIDE, 2 },
  { K_EXIT_86_FAST, 2, K_EXIT_86_WIDE, 2 },
  { K_EXIT_87_FAST, 2, K_EXIT_87_WIDE, 2 },
  { K_EXIT_88_FAST, 2, K_EXIT_88_WIDE, 9 },
  { K_EXIT_89_FAST, 2, K_EXIT_89_WIDE, 2 },
  { K_EXIT_90_FAST, 2, K_EXIT_90_WIDE, 2 },
  { K_EXIT_91_FAST, 2, K_EXIT_91_WIDE, 6 },
  { K_EXIT_92_FAST, 2, K_EXIT_92_WIDE, 2 },
  { K_EXIT_93_FAST, 2, K_EXIT_93_WIDE, 2 },
  { K_EXIT_94_FAST, 2, K_EXIT_94_WIDE, 2 },
  { K_EXIT_95_FAST, 2, K_EXIT_95_WIDE, 3 },
  { K_EXIT_96_FAST, 2, K_EXIT_96_WIDE, 2 },
  { K_EXIT_97_FAST, 2, K_EXIT_97_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_100_FAST, 2, K_EXIT_100_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_104_FAST, 2, K_EXIT_104_WIDE, 2 },
  { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 },
  { K_EXIT_106_FAST, 2, K_EXIT_106_WIDE, 2 },
  { K_EXIT_107_FAST, 2, K_EXIT_107_WIDE, 2 },
};

static const char** KERCALL_ENTER_WIDE64_PTR[kKercallTableSize] = {
  nullptr,
  nullptr,
  nullptr,
  K_ENTER64_3_WIDE,
  nullptr,
  nullptr,
  K_ENTER64_6_WIDE,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  K_ENTER64_11_WIDE,
  K_ENTER64_12_WIDE,
  nullptr,
  K_ENTER64_14_WIDE,
  K_ENTER64_15_WIDE,
  K_ENTER64_16_WIDE,
  K_ENTER64_17_WIDE,
  nullptr,
  nullptr,
  K_ENTER64_20_WIDE,
  K_ENTER64_21_WIDE,
  nullptr,
  nullptr,
  K_ENTER64_24_WIDE,
  nullptr,
  K_ENTER64_26_WIDE,
  K_ENTER64_27_WIDE,
  nullptr,
  K_ENTER64_29_WIDE,
  nullptr,
  nullptr,
  nullptr,
  K_ENTER64_33_WIDE,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  K_ENTER64_46_WIDE,
  K_ENTER64_47_WIDE,
  nullptr,
  nullptr,
  nullptr,
  K_ENTER64_51_WIDE,
  K_ENTER64_52_WIDE,
  K_ENTER64_53_WIDE,
  nullptr,
  K_ENTER64_55_WIDE,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  K_ENTER64_70_WIDE,
  nullptr,
  nullptr,
  K_ENTER64_73_WIDE,
  nullptr,
  K_ENTER64_75_WIDE,
  nullptr,
  nullptr,
  K_ENTER64_78_WIDE,
  K_ENTER64_79_WIDE,
  K_ENTER64_80_WIDE,
  K_ENTER64_81_WIDE,
  K_ENTER64_82_WIDE,
  K_ENTER64_83_WIDE,
  K_ENTER64_84_WIDE,
  K_ENTER64_85_WIDE,
  K_ENTER64_86_WIDE,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  K_ENTER64_92_WIDE,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
};
static const int KERCALL_ENTER_WIDE64_COUNT[kKercallTableSize] = {
  0,
  0,
  0,
  6,
  0,
  0,
  5,
  0,
  0,
  0,
  0,
  8,
  8,
  0,
  3,
  8,
  7,
  8,
  0,
  0,
  5,
  9,
  0,
  0,
  3,
  0,
  7,
  2,
  0,
  9,
  0,
  0,
  0,
  7,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  21,
  4,
  0,
  0,
  0,
  3,
  3,
  5,
  0,
  7,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  9,
  0,
  0,
  5,
  0,
  12,
  0,
  0,
  9,
  4,
  4,
  4,
  8,
  5,
  4,
  5,
  7,
  0,
  0,
  0,
  0,
  0,
  3,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
};

static const char** KERCALL_EXIT_WIDE64_PTR[kKercallTableSize] = {
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  K_EXIT64_11_WIDE,
  K_EXIT64_12_WIDE,
  nullptr,
  K_EXIT64_14_WIDE,
  nullptr,
  K_EXIT64_16_WIDE,
  K_EXIT64_17_WIDE,
  nullptr,
  nullptr,
  nullptr,
  K_EXIT64_21_WIDE,
  nullptr,
  nullptr,
  K_EXIT64_24_WIDE,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  K_EXIT64_29_WIDE,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  K_EXIT64_50_WIDE,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  K_EXIT64_58_WIDE,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
};
static const int KERCALL_EXIT_WIDE64_COUNT[kKercallTableSize] = {
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  5,
  5,
  0,
  21,
  0,
  5,
  2,
  0,
  0,
  0,
  3,
  0,
  0,
  21,
  0,
  0,
  0,
  0,
  6,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  3,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  3,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
};


const char* get_kercall_name(int call_num, int os_version) {
  if (call_num < 0 || call_num >= kKercallTableSize) return nullptr;
  const char** tbl = (os_version >= 800) ? KERCALL_NAMES_80_DATA : KERCALL_NAMES_71_DATA;
  return tbl[call_num];
}

KercallFields get_kercall_enter_fields(int call_num, bool is_64) {
  if (call_num < 0 || call_num >= kKercallTableSize) {
    return { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 };
  }
  const KercallFields base = KERCALL_ENTER_LOOKUP[call_num];
  if (is_64) {
    const char** w64 = KERCALL_ENTER_WIDE64_PTR[call_num];
    if (w64) {
      return { base.fast, base.fast_count, w64, KERCALL_ENTER_WIDE64_COUNT[call_num] };
    }
  }
  return base;
}

KercallFields get_kercall_exit_fields(int call_num, bool is_64) {
  if (call_num < 0 || call_num >= kKercallTableSize) {
    return { K_DEFAULT_FAST, 2, K_DEFAULT_WIDE, 2 };
  }
  const KercallFields base = KERCALL_EXIT_LOOKUP[call_num];
  if (is_64) {
    const char** w64 = KERCALL_EXIT_WIDE64_PTR[call_num];
    if (w64) {
      return { base.fast, base.fast_count, w64, KERCALL_EXIT_WIDE64_COUNT[call_num] };
    }
  }
  return base;
}

}  // namespace qst
