/**
 * @file core/tracer_event_config.cpp
 * @brief TracerEngine - 内核追踪事件类配置（QNX only）
 *
 * 包含 initKernelTraceBase、killExistingTracelogger、
 * setupSchedulingEvents、configureEventClasses 等事件配置逻辑。
 */

#include "qst/core/tracer_engine.hpp"
#include "qst/log.hpp"

#ifdef __QNX__

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/trace.h>
#include <sys/kercalls.h>
#include <sys/wait.h>
#include <sys/procmgr.h>

namespace qst {
namespace core {

int TracerEngine::initKernelTraceBase() {
    int ret = ThreadCtl(_NTO_TCTL_IO, 0);
    if (ret == -1) {
        LOG_ERROR("Failed to get I/O privilege: {}", strerror(errno));
        return -1;
    }

    ret = procmgr_ability(0,
        PROCMGR_ADN_ROOT | PROCMGR_AOP_ALLOW | PROCMGR_AID_TRACE,
        PROCMGR_ADN_ROOT | PROCMGR_AOP_ALLOW | PROCMGR_AID_MEM_PHYS,
        PROCMGR_AID_EOL);
    if (ret == -1) {
        LOG_WARN("procmgr_ability(TRACE|MEM_PHYS) failed: {} (continuing anyway)",
                 strerror(errno));
    } else {
        LOG_INFO("PROCMGR_AID_TRACE + MEM_PHYS abilities granted");
    }

    int trace_support = TraceEvent(_NTO_TRACE_QUERYSUPPORT);
    if (trace_support == _NTO_TRACE_NOINSTRSUPP) {
        LOG_ERROR("Kernel instrumentation not enabled (procnto not instrumented)");
        return -1;
    }
    LOG_INFO("Kernel instrumentation: supported");

    killExistingTracelogger();
    return 0;
}

void TracerEngine::killExistingTracelogger() {
    LOG_INFO("Cleaning up residual trace state...");

    system("slay -f tracelogger 2>/dev/null");
    usleep(300000);
    system("slay -f qst_tracer 2>/dev/null");
    usleep(200000);

    TraceEvent(_NTO_TRACE_STOP);
    TraceEvent(_NTO_TRACE_DEALLOCBUFFER);
    usleep(100000);
}

int TracerEngine::setupSchedulingEvents() {
    LOG_INFO("Setting up scheduling events (all Wide mode)...");

    TraceEvent(_NTO_TRACE_DELALLCLASSES);

    TraceEvent(_NTO_TRACE_CLRCLASSPID, _NTO_TRACE_KERCALL);
    TraceEvent(_NTO_TRACE_CLRCLASSTID, _NTO_TRACE_KERCALL);
    TraceEvent(_NTO_TRACE_CLRCLASSPID, _NTO_TRACE_KERCALLENTER);
    TraceEvent(_NTO_TRACE_CLRCLASSTID, _NTO_TRACE_KERCALLENTER);
    TraceEvent(_NTO_TRACE_CLRCLASSPID, _NTO_TRACE_KERCALLEXIT);
    TraceEvent(_NTO_TRACE_CLRCLASSTID, _NTO_TRACE_KERCALLEXIT);

    LOG_INFO("  [1/8] Thread state events (Wide mode)...");
    TraceEvent(_NTO_TRACE_SETCLASSWIDE, _NTO_TRACE_THREAD);
    TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_THREAD);

#if !defined(QNX_800) && defined(_NTO_TRACE_VTHREAD)
    LOG_INFO("  [2/8] VThread state events (Wide mode)...");
    TraceEvent(_NTO_TRACE_SETCLASSWIDE, _NTO_TRACE_VTHREAD);
    TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_VTHREAD);
#else
    LOG_INFO("  [2/8] VThread - skipped (not available on QNX 8.0)");
#endif

    LOG_INFO("  [3/8] Process events...");
    TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_PROCESS);

    LOG_INFO("  [4/8] Interrupt events - skipped");

    LOG_INFO("  [5/8] Communication events (Wide mode)...");
    TraceEvent(_NTO_TRACE_SETCLASSWIDE, _NTO_TRACE_COMM);
    TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_COMM);

    LOG_INFO("  [6/8] System events - skipped");

    LOG_INFO("  [7/8] Control events (CONTROLBUFFER only)...");
    TraceEvent(_NTO_TRACE_ADDEVENT, _NTO_TRACE_CONTROL, _NTO_TRACE_CONTROLBUFFER);

    LOG_INFO("  [8/8] KerCall events (sync + IPC + sched)...");
    setupKerCallEnterOnly(__KER_SYNC_MUTEX_LOCK, "SYNC_MUTEX_LOCK");
    setupKerCallEnterOnly(__KER_SYNC_MUTEX_UNLOCK, "SYNC_MUTEX_UNLOCK");
    setupKerCallEnterOnly(__KER_SYNC_SEM_WAIT, "SYNC_SEM_WAIT");
    setupKerCallEnterOnly(__KER_SYNC_SEM_POST, "SYNC_SEM_POST");
    setupKerCallEnterOnly(__KER_SYNC_CONDVAR_WAIT, "SYNC_CONDVAR_WAIT");
    setupKerCallEnterOnly(__KER_SYNC_CONDVAR_SIGNAL, "SYNC_CONDVAR_SIGNAL");
    setupKerCallEvent(__KER_MSG_SENDV, "MSG_SENDV");
    setupKerCallEvent(__KER_MSG_RECEIVEV, "MSG_RECEIVEV");
    setupKerCallEvent(__KER_MSG_REPLYV, "MSG_REPLYV");
    setupKerCallEnterOnly(__KER_SCHED_YIELD, "SCHED_YIELD");

    LOG_INFO("Scheduling events configured (all Wide mode)");
    return 0;
}

void TracerEngine::setupKerCallEvent(int kercall_id, const char* name) {
    TraceEvent(_NTO_TRACE_SETEVENTWIDE, _NTO_TRACE_KERCALLENTER, kercall_id);
    TraceEvent(_NTO_TRACE_SETEVENTWIDE, _NTO_TRACE_KERCALLEXIT, kercall_id);
    TraceEvent(_NTO_TRACE_ADDEVENT, _NTO_TRACE_KERCALLENTER, kercall_id);
    TraceEvent(_NTO_TRACE_ADDEVENT, _NTO_TRACE_KERCALLEXIT, kercall_id);
    LOG_DEBUG("    Added KerCall: {} (ENTER + EXIT, Wide mode)", name);
}

void TracerEngine::setupKerCallEnterOnly(int kercall_id, const char* name) {
    TraceEvent(_NTO_TRACE_SETEVENTWIDE, _NTO_TRACE_KERCALLENTER, kercall_id);
    TraceEvent(_NTO_TRACE_ADDEVENT, _NTO_TRACE_KERCALLENTER, kercall_id);
    LOG_DEBUG("    Added KerCall: {} (ENTER only, Wide mode)", name);
}

void TracerEngine::configureEventClasses() {
    bool need_classwide = true;

    if (config_.event_filter == config::EventFilter::Scheduling) {
        if (setupSchedulingEvents() == 0) {
            need_classwide = false;
        } else {
            LOG_WARN("Failed to setup scheduling events, falling back to full capture");
            TraceEvent(_NTO_TRACE_ADDALLCLASSES);
        }
    } else {
        TraceEvent(_NTO_TRACE_ADDALLCLASSES);
    }

    if (need_classwide) {
        TraceEvent(_NTO_TRACE_SETCLASSWIDE, _NTO_TRACE_THREAD);
#if !defined(QNX_800) && defined(_NTO_TRACE_VTHREAD)
        TraceEvent(_NTO_TRACE_SETCLASSWIDE, _NTO_TRACE_VTHREAD);
#endif
        TraceEvent(_NTO_TRACE_SETCLASSWIDE, _NTO_TRACE_COMM);
        TraceEvent(_NTO_TRACE_SETCLASSWIDE, _NTO_TRACE_SYSTEM);
        TraceEvent(_NTO_TRACE_SETCLASSWIDE, _NTO_TRACE_INT);
        TraceEvent(_NTO_TRACE_SETCLASSWIDE, _NTO_TRACE_KERCALLENTER);
        TraceEvent(_NTO_TRACE_SETCLASSWIDE, _NTO_TRACE_KERCALLEXIT);
    }

    LOG_INFO("Event classes configured: filter={}, wide mode enabled",
             config_.event_filter == config::EventFilter::Scheduling
                 ? "scheduling" : "all");
}

}  // namespace core
}  // namespace qst

#endif  // __QNX__
