#ifndef QNX_COMPAT_H
#define QNX_COMPAT_H

#include <stdint.h>
#include <stdio.h>

typedef uint32_t _Uint32t;
typedef uintptr_t _Uintptrt;

typedef _Uint32t __traceentry;

typedef struct traceevent {
    __traceentry header;
    __traceentry data[3];
} traceevent_t;

#define _NTO_TRACE_GETCPU(h)        (((h) & 0x3f000000u) >> 24)
#define _NTO_TRACE_GETEVENT(h)      ((unsigned)(h) & 0x3ffu)
#define _NTO_TRACE_GETEVENT_C(h)    ((h) & (0x1fu << 10))

#define _TRACE_STRUCT_S     ((uint32_t)0x00000000u)
#define _TRACE_STRUCT_CB    ((uint32_t)0x00000001u << 30)
#define _TRACE_STRUCT_CC    ((uint32_t)0x00000002u << 30)
#define _TRACE_STRUCT_CE    ((uint32_t)0x00000003u << 30)
#define _TRACE_GET_STRUCT(c) ((c) & ((uint32_t)0x3u << 30))

/* External classes */
enum {
    _NTO_TRACE_EMPTY,
    _NTO_TRACE_CONTROL,
    _NTO_TRACE_KERCALL,
    _NTO_TRACE_KERCALLENTER,
    _NTO_TRACE_KERCALLEXIT,
    _NTO_TRACE_KERCALLINT,
    _NTO_TRACE_INT,
    _NTO_TRACE_RESERVEDCLASS1,
    _NTO_TRACE_RESERVEDCLASS2,
    _NTO_TRACE_PROCESS,
    _NTO_TRACE_THREAD,
    _NTO_TRACE_RESERVEDCLASS3,
    _NTO_TRACE_USER,
    _NTO_TRACE_SYSTEM,
    _NTO_TRACE_COMM,
};

#define _NTO_TRACE_CONTROLTIME      ((uint32_t)0x00000001u << 0)
#define _NTO_TRACE_CONTROLBUFFER    ((uint32_t)0x00000001u << 1)

#define _NTO_TRACE_KERCALLFIRST     (0x00000000u)
#define _NTO_TRACE_KERCALLLAST      (128u)
#define _NTO_TRACE_KERCALL64        ((uint32_t)0x00000001u << 9)

#define _NTO_TRACE_INTFIRST         (0x00000000u)
#define _NTO_TRACE_INTLAST          (0xffffffffu)

/* Thread states (from sys/states.h) */
enum {
    KEV_STATE_DEAD        = 0,
    KEV_STATE_RUNNING     = 1,
    KEV_STATE_READY       = 2,
    KEV_STATE_STOPPED     = 3,
    KEV_STATE_SEND        = 4,
    KEV_STATE_RECEIVE     = 5,
    KEV_STATE_REPLY       = 6,
    KEV_STATE_STACK       = 7,
    KEV_STATE_WAITTHREAD  = 8,
    KEV_STATE_WAITPAGE    = 9,
    KEV_STATE_SIGSUSPEND  = 10,
    KEV_STATE_SIGWAITINFO = 11,
    KEV_STATE_NANOSLEEP   = 12,
    KEV_STATE_MUTEX       = 13,
    KEV_STATE_CONDVAR     = 14,
    KEV_STATE_JOIN        = 15,
    KEV_STATE_INTR        = 16,
    KEV_STATE_SEM         = 17,
    KEV_STATE_WAITCTX     = 18,
    KEV_STATE_NET_SEND    = 19,
    KEV_STATE_NET_REPLY   = 20,
    KEV_STATE_MAX         = 24,
};

enum {
    KEV_TRACE_THREAD_CREATE  = KEV_STATE_MAX,     /* 24 */
    KEV_TRACE_THREAD_DESTROY = KEV_STATE_MAX + 1, /* 25 */
#if KEV_QNX_VERSION >= 80
    KEV_STATE_MUON_MUTEX     = 26,
    KEV_STATE_TRACEBUFFER    = 27,
    KEV_STATE_INTR_ATTACH_EV = 28,
    KEV_STATE_TIMER_DELEGATE = 29,
    KEV_TRACE_MAX_TH_STATE_NUM = 30
#else
    KEV_TRACE_MAX_TH_STATE_NUM                    /* 26 */
#endif
};

#if KEV_QNX_VERSION >= 80
/* QNX 8.0: _NTO_TRACE_TH* are direct state values */
#define _NTO_TRACE_THDEAD           KEV_STATE_DEAD
#define _NTO_TRACE_THRUNNING        KEV_STATE_RUNNING
#define _NTO_TRACE_THREADY          KEV_STATE_READY
#define _NTO_TRACE_THSTOPPED        KEV_STATE_STOPPED
#define _NTO_TRACE_THSEND           KEV_STATE_SEND
#define _NTO_TRACE_THRECEIVE        KEV_STATE_RECEIVE
#define _NTO_TRACE_THREPLY          KEV_STATE_REPLY
#define _NTO_TRACE_THWAITPAGE       9u   /* STATE_WAITPAGE */
#define _NTO_TRACE_THSIGSUSPEND     10u  /* STATE_SIGSUSPEND */
#define _NTO_TRACE_THSIGWAITINFO    11u  /* STATE_SIGWAITINFO */
#define _NTO_TRACE_THNANOSLEEP      12u  /* STATE_NANOSLEEP */
#define _NTO_TRACE_THMUTEX          13u  /* STATE_MUTEX */
#define _NTO_TRACE_THCONDVAR        14u  /* STATE_CONDVAR */
#define _NTO_TRACE_THJOIN           15u  /* STATE_JOIN */
#define _NTO_TRACE_THINTR           16u  /* STATE_INTR */
#define _NTO_TRACE_THSEM            17u  /* STATE_SEM */
#define _NTO_TRACE_THWAITCTX        18u  /* STATE_WAITCTX */
#define _NTO_TRACE_THBARRIER        21u  /* STATE_BARRIER */
#define _NTO_TRACE_THCREATE         24u  /* STATE_CREATE */
#define _NTO_TRACE_THDESTROY        25u  /* STATE_DESTROY */
#define _NTO_TRACE_THMUON_MUTEX     26u  /* STATE_MUON_MUTEX */
#define _NTO_TRACE_THTRACEBUFFER    27u  /* STATE_TRACEBUFFER */
#define _NTO_TRACE_THINTR_ATTACH_EV 28u  /* STATE_INTR_ATTACH_EV */
#define _NTO_TRACE_THTIMER_DELEGATE 29u  /* STATE_TIMER_DELEGATE */
/* Aliases for removed states (unused on 8.0, kept for compile compat) */
#define _NTO_TRACE_THSTACK          7u   /* STATE_MQ_SEND on 8.0 */
#define _NTO_TRACE_THWAITTHREAD     8u   /* STATE_MQ_RECEIVE on 8.0 */
#define _NTO_TRACE_THNET_SEND       19u  /* STATE_RWLOCK_READ on 8.0 */
#define _NTO_TRACE_THNET_REPLY      20u  /* STATE_RWLOCK_WRITE on 8.0 */
#else
/* QNX 7.1: _NTO_TRACE_TH* are bitmasks (1 << state_index) */
#define _NTO_TRACE_THDEAD           (0x00000001u << KEV_STATE_DEAD)
#define _NTO_TRACE_THRUNNING        (0x00000001u << KEV_STATE_RUNNING)
#define _NTO_TRACE_THREADY          (0x00000001u << KEV_STATE_READY)
#define _NTO_TRACE_THSTOPPED        (0x00000001u << KEV_STATE_STOPPED)
#define _NTO_TRACE_THSEND           (0x00000001u << KEV_STATE_SEND)
#define _NTO_TRACE_THRECEIVE        (0x00000001u << KEV_STATE_RECEIVE)
#define _NTO_TRACE_THREPLY          (0x00000001u << KEV_STATE_REPLY)
#define _NTO_TRACE_THSTACK          (0x00000001u << KEV_STATE_STACK)
#define _NTO_TRACE_THWAITTHREAD     (0x00000001u << KEV_STATE_WAITTHREAD)
#define _NTO_TRACE_THWAITPAGE       (0x00000001u << KEV_STATE_WAITPAGE)
#define _NTO_TRACE_THSIGSUSPEND     (0x00000001u << KEV_STATE_SIGSUSPEND)
#define _NTO_TRACE_THSIGWAITINFO    (0x00000001u << KEV_STATE_SIGWAITINFO)
#define _NTO_TRACE_THNANOSLEEP      (0x00000001u << KEV_STATE_NANOSLEEP)
#define _NTO_TRACE_THMUTEX          (0x00000001u << KEV_STATE_MUTEX)
#define _NTO_TRACE_THCONDVAR        (0x00000001u << KEV_STATE_CONDVAR)
#define _NTO_TRACE_THJOIN           (0x00000001u << KEV_STATE_JOIN)
#define _NTO_TRACE_THINTR           (0x00000001u << KEV_STATE_INTR)
#define _NTO_TRACE_THSEM            (0x00000001u << KEV_STATE_SEM)
#define _NTO_TRACE_THWAITCTX        (0x00000001u << KEV_STATE_WAITCTX)
#define _NTO_TRACE_THNET_SEND       (0x00000001u << KEV_STATE_NET_SEND)
#define _NTO_TRACE_THNET_REPLY      (0x00000001u << KEV_STATE_NET_REPLY)
#define _NTO_TRACE_THCREATE         (0x00000001u << KEV_TRACE_THREAD_CREATE)
#define _NTO_TRACE_THDESTROY        (0x00000001u << KEV_TRACE_THREAD_DESTROY)
#endif

#define _NTO_TRACE_PROCCREATE       ((uint32_t)0x00000001u << 0)
#define _NTO_TRACE_PROCDESTROY      ((uint32_t)0x00000001u << 1)
#define _NTO_TRACE_PROCCREATE_NAME  ((uint32_t)0x00000001u << 2)
#define _NTO_TRACE_PROCDESTROY_NAME ((uint32_t)0x00000001u << 3)
#define _NTO_TRACE_PROCTHREAD_NAME  ((uint32_t)0x00000001u << 4)

#define _NTO_TRACE_COMM_SMSG        (0x00000000u)
#define _NTO_TRACE_COMM_LAST        (0x0000000bu)

#define _NTO_TRACE_SYS_RESERVED     (0x00000001u)
#define _NTO_TRACE_SYS_LAST         (0x0000001fu)

/* Internal classes */
#define _TRACE_CONTROL_C            ((uint32_t)0x00000001u << 10)
#define _TRACE_KER_CALL_C           ((uint32_t)0x00000002u << 10)
#define _TRACE_INT_C                ((uint32_t)0x00000003u << 10)
#define _TRACE_PR_TH_C              ((uint32_t)0x00000004u << 10)
#define _TRACE_SYSTEM_C             ((uint32_t)0x00000005u << 10)
#define _TRACE_USER_C               ((uint32_t)0x00000006u << 10)
#define _TRACE_COMM_C               ((uint32_t)0x00000007u << 10)

#define _TRACE_MAX_KER_CALL_NUM     (128u)
#if KEV_QNX_VERSION >= 80
#define _TRACE_MAX_TH_STATE_NUM     30u
#else
#define _TRACE_MAX_TH_STATE_NUM     26u
#endif

/* traceparser info modes */
typedef enum {
    _TRACEPARSER_INFO_HEADER_BEGIN = 0,
    _TRACEPARSER_INFO_FILE_NAME,
    _TRACEPARSER_INFO_DATE,
    _TRACEPARSER_INFO_VER_MAJOR,
    _TRACEPARSER_INFO_VER_MINOR,
    _TRACEPARSER_INFO_LITTLE_ENDIAN,
    _TRACEPARSER_INFO_BIG_ENDIAN,
    _TRACEPARSER_INFO_MIDDLE_ENDIAN,
    _TRACEPARSER_INFO_ENCODING,
    _TRACEPARSER_INFO_BOOT_DATE,
    _TRACEPARSER_INFO_CYCLES_PER_SEC,
    _TRACEPARSER_INFO_CPU_NUM,
    _TRACEPARSER_INFO_SYSNAME,
    _TRACEPARSER_INFO_NODENAME,
    _TRACEPARSER_INFO_SYS_RELEASE,
    _TRACEPARSER_INFO_SYS_VERSION,
    _TRACEPARSER_INFO_MACHINE,
    _TRACEPARSER_INFO_SYSPAGE_LEN,
    _TRACEPARSER_INFO_NORMALIZEDN,
    _TRACEPARSER_INFO_HEADER_END,

    _TRACEPARSER_INFO_SYSPAGE              = -1,
    _TRACEPARSER_INFO_ENDIAN_CONV          = -2,
    _TRACEPARSER_INFO_NOW_CALLBACK_CLASS   = -3,
    _TRACEPARSER_INFO_NOW_CALLBACK_EVENT   = -4,
    _TRACEPARSER_INFO_PREV_CALLBACK_CLASS  = -5,
    _TRACEPARSER_INFO_PREV_CALLBACK_EVENT  = -6,
    _TRACEPARSER_INFO_PREV_CALLBACK_RETURN = -7,
    _TRACEPARSER_INFO_DEBUG                = -8,
    _TRACEPARSER_INFO_ERROR                = -9,
    _TRACEPARSER_INFO_ERROR_STR            = -10,
    _TRACEPARSER_INFO_STATS                = -11,
    _TRACEPARSER_INFO_EVENT_ID             = -12,
    _TRACEPARSER_INFO_CLK_MSB              = -13,
    _TRACEPARSER_INFO_CLK_INTIAL           = -14,
    _TRACEPARSER_INFO_CLK                  = -15,
    _TRACEPARSER_INFO_CLK_DELTA            = -16,
    _TRACEPARSER_INFO_TIME_DELTA           = -17,
} info_modes_t;

/* traceparser debug modes */
#define _TRACEPARSER_DEBUG_ALL      (0xffffffffu)
#define _TRACEPARSER_DEBUG_NONE     (0x00000000u)
#define _TRACEPARSER_DEBUG_ERRORS   (0x00000001u << 0)
#define _TRACEPARSER_DEBUG_HEADER   (0x00000001u << 1)

/* Sort modes */
#define _TRACEPARSER_NOSORT 0U
#define _TRACEPARSER_SORT   1U

#ifdef __cplusplus
extern "C" {
#endif

struct traceparser_state;
typedef int (*tracep_callb_func_t)(struct traceparser_state*, void*,
                                   unsigned, unsigned, unsigned*, unsigned);

extern struct traceparser_state* traceparser_init(struct traceparser_state*);
extern void  traceparser_destroy(struct traceparser_state**);
extern const void* traceparser_get_info(struct traceparser_state*,
                                        info_modes_t, unsigned*);
extern int   traceparser_debug(struct traceparser_state*, FILE*, unsigned);
extern void  traceparser_sort(struct traceparser_state*, unsigned);
extern int   traceparser_cs(struct traceparser_state*, void*,
                            tracep_callb_func_t, unsigned, unsigned);
extern int   traceparser_cs_range(struct traceparser_state*, void*,
                                  tracep_callb_func_t, unsigned,
                                  unsigned, unsigned);
extern int   traceparser(struct traceparser_state*, void*, const char*);
extern int   traceparser_one_event(struct traceparser_state*, traceevent_t*);

#ifdef __cplusplus
}
#endif

#endif /* QNX_COMPAT_H */
