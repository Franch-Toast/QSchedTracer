#pragma once

#include <cstdint>

namespace qst {

constexpr uint32_t QST_MAGIC = 0x51535434;    // 'QST4'
constexpr int QST_VERSION = 4;
constexpr int QST_EVENT_SIZE = 16;

constexpr int FILE_HEADER_SIZE = 64;
constexpr int SECTION_HEADER_SIZE = 24;

constexpr uint32_t DATA_MAGIC = 0x44415441;   // 'DATA'
constexpr uint32_t PINF_MAGIC = 0x50494E46;   // 'PINF'

constexpr int TRACEBUF_OFFSET_FLAGS      = 16;
constexpr int TRACEBUF_OFFSET_NUM_EVENTS = 20;
constexpr int TRACEBUF_OFFSET_SEQ        = 24;

constexpr uint32_t MAX_UINT32 = 0xFFFFFFFF;

constexpr int _TRACE_MAX_KER_CALL_NUM = 128;
constexpr int KERCALL_64 = 0x200;

enum InternalClass {
    IC_EMPTY    = 0,
    IC_CONTROL  = 1,
    IC_KER_CALL = 2,
    IC_INT      = 3,
    IC_PR_TH    = 4,
    IC_SYSTEM   = 5,
    IC_USER     = 6,
    IC_COMM     = 7,
};

enum StructType {
    ST_SIMPLE        = 0,
    ST_COMBINE_BEGIN = 1,
    ST_COMBINE_CONT  = 2,
    ST_COMBINE_END   = 3,
};

enum IntEventType {
    INT_ENTRY            = 1,
    INT_EXIT             = 2,
    INT_HANDLER_ENTRY    = 3,
    INT_HANDLER_EXIT     = 4,
    INT_ENTRY_64         = 5,
    INT_HANDLER_ENTRY_64 = 6,
};

constexpr int MAX_TH_STATE_NUM = 26;

static const char* const THREAD_STATES[] = {
    "DEAD",         // 0
    "RUNNING",      // 1
    "READY",        // 2
    "STOPPED",      // 3
    "SEND",         // 4
    "RECEIVE",      // 5
    "REPLY",        // 6
    "STACK",        // 7
    "WAITTHREAD",   // 8
    "WAITPAGE",     // 9
    "SIGSUSPEND",   // 10
    "SIGWAITINFO",  // 11
    "NANOSLEEP",    // 12
    "MUTEX",        // 13
    "CONDVAR",      // 14
    "JOIN",         // 15
    "INTR",         // 16
    "SEM",          // 17
    "WAITCTX",      // 18
    "NET_SEND",     // 19
    "NET_REPLY",    // 20
    nullptr,        // 21
    nullptr,        // 22
    nullptr,        // 23
    "CREATE",       // 24
    "DESTROY",      // 25
};
constexpr int THREAD_STATES_COUNT = 26;

static const char* const POLICY_NAMES[] = {
    "UNKNOWN",        // 0
    "SCHED_FIFO",     // 1
    "SCHED_RR",       // 2
    "SCHED_OTHER",    // 3
    "SCHED_SPORADIC", // 4
};
constexpr int POLICY_NAMES_COUNT = 5;

inline const char* get_thread_state(int code) {
    if (code >= 0 && code < THREAD_STATES_COUNT && THREAD_STATES[code])
        return THREAD_STATES[code];
    return nullptr;
}

inline const char* format_policy(int policy) {
    if (policy >= 0 && policy < POLICY_NAMES_COUNT)
        return POLICY_NAMES[policy];
    return "UNKNOWN";
}

constexpr int CPU_PID = 999000;
inline int cpu_main_tid(int cpu_id) { return cpu_id * 10 + 1; }
inline int cpu_irq_tid(int cpu_id) { return cpu_id * 10 + 2; }

}  // namespace qst
