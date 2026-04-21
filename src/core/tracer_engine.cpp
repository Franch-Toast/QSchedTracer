/**
 * @file core/tracer_engine.cpp
 * @brief TracerEngine - 核心生命周期（构造/初始化/运行循环/清理）
 */

#include "qst/core/tracer_engine.hpp"
#include "qst/log.hpp"

#include <unistd.h>
#include <chrono>

#ifdef __QNX__
#include <sys/syspage.h>
#include <sys/trace.h>
#include <sys/mman.h>
#endif

namespace qst {
namespace core {

TracerEngine::TracerEngine(const config::TracerConfig& config)
    : config_(config)
{
    kernel_buffer_count_ = config_.computeKernelBufferCount(getCpuCount());
}

TracerEngine::~TracerEngine() {
    cleanup();
}

int TracerEngine::getCpuCount() {
#ifdef __QNX__
    if (_syspage_ptr) {
        return _syspage_ptr->num_cpu;
    }
#endif
    return static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
}

uint64_t TracerEngine::getClockFreq() {
#ifdef __QNX__
    if (_syspage_ptr && SYSPAGE_ENTRY(qtime)) {
        return SYSPAGE_ENTRY(qtime)->cycles_per_sec;
    }
#endif
    return 1000000000ULL;
}

int TracerEngine::run() {
    LOG_INFO("========================================");
    LOG_INFO("QSchedTracer - Ring Mode (QST v4)");
    LOG_INFO("========================================");
    LOG_INFO("Event filter: {}",
             config_.event_filter == config::EventFilter::Scheduling
                 ? "scheduling (filtered)" : "all (full capture)");
    LOG_INFO("Buffer: {} MB", config_.buffer_size_mb);
    LOG_INFO("Kernel buffers (-k): {} (from {} MB, {} CPUs)",
             kernel_buffer_count_, config_.buffer_size_mb, getCpuCount());

    if (initialize() != 0) {
        LOG_ERROR("Initialization failed");
        return 1;
    }

#ifdef __QNX__
    if (setupSelfManagedMode() != 0) {
        LOG_ERROR("Setup failed");
        return 1;
    }
#endif

    runLoop();

    printStats();
    cleanup();

    LOG_INFO("Exit");
    return 0;
}

void TracerEngine::requestStop() {
    stop_requested_.store(true);
}

void TracerEngine::requestDump() {
    dump_requested_.store(true);
}

int TracerEngine::initialize() {
    LOG_INFO("Initializing...");

    data_manager_ = std::make_unique<data::DataManager>(
        data_buffer_, config_.output_dir, config_.file_prefix);

    data_buffer_.setClockFreq(getClockFreq());
    LOG_INFO("Clock freq: {} Hz", data_buffer_.clockFreq());

    return 0;
}

void TracerEngine::runLoop() {
    LOG_INFO("Starting Ring Mode trace...");
    LOG_INFO("SIGINT (Ctrl+C) = dump + exit, SIGUSR1 = dump + restart");

#ifdef __QNX__
    trace_start_time_ = std::chrono::system_clock::now();
    TraceEvent(_NTO_TRACE_STARTNOSTATE);

    while (!stop_requested_.load()) {
        usleep(100000);  // 100ms poll
        if (dump_requested_.exchange(false)) {
            LOG_INFO("SIGUSR1 received, dumping and restarting...");
            dumpSelfManaged(0, true);
        }
    }

    dumpSelfManaged(0, false);
    LOG_INFO("Trace stopped");
#else
    LOG_WARN("Non-QNX platform, trace not available");
#endif
}

void TracerEngine::cleanup() {
    LOG_INFO("Cleaning up...");

#ifdef __QNX__
    TraceEvent(_NTO_TRACE_STOP);

    if (kernel_buffers_ != nullptr && kernel_buffers_ != MAP_FAILED) {
#if !defined(LP8797)
        size_t total_size = total_kernel_buffers_ * sizeof(tracebuf_t);
        munmap(kernel_buffers_, total_size);
#else
        if (!logger_attached_) {
            size_t total_size = total_kernel_buffers_ * sizeof(tracebuf_t);
            munmap(kernel_buffers_, total_size);
        }
#endif
        kernel_buffers_ = nullptr;
    }

    TraceEvent(_NTO_TRACE_DEALLOCBUFFER);

#if defined(LP8797)
    if (logger_attached_) {
        TraceEvent(_NTO_TRACE_LOGGER_DETACH);
        logger_attached_ = false;
    }
#endif
#endif

    LOG_INFO("Cleanup complete");
}

void TracerEngine::printStats() const {
    LOG_INFO("=== Statistics ===");
    LOG_INFO("Kernel buffer count (-k): {}", kernel_buffer_count_);
    LOG_INFO("Dumps completed: {}", static_cast<uint64_t>(buffers_processed_));
}

}  // namespace core
}  // namespace qst
