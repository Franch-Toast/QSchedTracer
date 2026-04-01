/**
 * @file core/tracer_engine.cpp
 * @brief QSchedTracer - Ring Mode 追踪引擎实现
 *
 * 从 deep_os_tool_cpp/tracer/tracer_engine.cc 对齐移植。
 * 使用内核 Ring Mode + mmap 零拷贝，输出 QST v4 文件。
 * 支持 QNX 7.1 (mmap 物理地址) 和 QNX 8.0 (LOGGER_ATTACH + per-CPU)。
 */

#include "qst/core/tracer_engine.hpp"
#include "qst/log.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <algorithm>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>

#ifdef __QNX__
#include <sys/types.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <sys/trace.h>
#include <sys/mman.h>
#include <sys/kercalls.h>
#include <signal.h>
#include <sys/wait.h>
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

int TracerEngine::initialize() {
    LOG_INFO("Initializing...");

    data_manager_ = std::make_unique<data::DataManager>(
        data_buffer_, config_.output_dir, config_.file_prefix);

    data_buffer_.setClockFreq(getClockFreq());
    LOG_INFO("Clock freq: {} Hz", data_buffer_.clockFreq());

    return 0;
}

// ============================================================================
// Kernel trace setup (QNX only)
// ============================================================================

#ifdef __QNX__

int TracerEngine::initKernelTraceBase() {
    int ret = ThreadCtl(_NTO_TCTL_IO, 0);
    if (ret == -1) {
        LOG_ERROR("Failed to get I/O privilege: {}", strerror(errno));
        return -1;
    }

    killExistingTracelogger();
    return 0;
}

void TracerEngine::killExistingTracelogger() {
    LOG_INFO("Cleaning up residual trace state...");

    TraceEvent(_NTO_TRACE_STOP);
    usleep(200000);

    int ret = system("slay -f tracelogger 2>/dev/null");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
        LOG_WARN("Killed existing tracelogger process(es)");
        usleep(500000);
    }

    TraceEvent(_NTO_TRACE_STOP);
    TraceEvent(_NTO_TRACE_DEALLOCBUFFER);
    usleep(100000);
}

// ============================================================================
// Event filtering
// ============================================================================

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

#if !defined(LP8797)
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
#if !defined(LP8797)
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

// ============================================================================
// Self-Managed Ring Mode setup
// ============================================================================

int TracerEngine::setupSelfManagedMode() {
    LOG_INFO("Setting up Self-Managed Ring Mode...");

    if (initKernelTraceBase() != 0) {
        return -1;
    }

#if defined(LP8797)
    // QNX 8.0: LOGGER_ATTACH + per-CPU ALLOCBUFFER
    int ret = TraceEvent(_NTO_TRACE_LOGGER_ATTACH);
    if (ret == -1) {
        LOG_ERROR("_NTO_TRACE_LOGGER_ATTACH failed: {}", strerror(errno));
        return -1;
    }
    logger_attached_ = true;

    int bufs_per_cpu = kernel_buffer_count_;
    tracebuf_t* buffers_base = nullptr;
    ret = TraceEvent(_NTO_TRACE_ALLOCBUFFER, bufs_per_cpu, &buffers_base);
    if (ret == -1) {
        LOG_ERROR("Failed to allocate {} kernel buffers per CPU: {}",
                  bufs_per_cpu, strerror(errno));
        TraceEvent(_NTO_TRACE_LOGGER_DETACH);
        logger_attached_ = false;
        return -1;
    }

    kernel_buffers_ = buffers_base;
    bufs_per_cpu_ = bufs_per_cpu;
    total_kernel_buffers_ = bufs_per_cpu * getCpuCount();

    int num_cpus = getCpuCount();
    cpu_write_start_.resize(num_cpus, 0);

    size_t total_size = total_kernel_buffers_ * sizeof(tracebuf_t);
    LOG_INFO("[QNX 8.0] Allocated {} buffers/CPU x {} CPUs = {} total ({} KB)",
             bufs_per_cpu, num_cpus, total_kernel_buffers_, total_size / 1024);

#else
    // QNX 7.1: ALLOCBUFFER returns physical address, needs mmap
    total_kernel_buffers_ = kernel_buffer_count_;
    int ret = TraceEvent(_NTO_TRACE_ALLOCBUFFER, total_kernel_buffers_, &kernel_paddr_);
    if (ret == -1) {
        LOG_ERROR("Failed to allocate {} kernel buffers: {}",
                  total_kernel_buffers_, strerror(errno));
        return -1;
    }

    size_t total_size = total_kernel_buffers_ * sizeof(tracebuf_t);
    kernel_buffers_ = static_cast<tracebuf_t*>(
        mmap(nullptr, total_size, PROT_READ, MAP_SHARED | MAP_PHYS, NOFD, kernel_paddr_));

    if (kernel_buffers_ == MAP_FAILED) {
        LOG_ERROR("Failed to mmap kernel buffer: {}", strerror(errno));
        TraceEvent(_NTO_TRACE_DEALLOCBUFFER);
        return -1;
    }

    LOG_INFO("[QNX 7.1] Allocated {} kernel buffers ({} KB) at paddr 0x{:x}",
             total_kernel_buffers_, total_size / 1024, kernel_paddr_);
#endif

    ret = TraceEvent(_NTO_TRACE_SETRINGMODE);
    if (ret == -1) {
        LOG_ERROR("Failed to set ring mode: {}", strerror(errno));
        return -1;
    }

#if !defined(LP8797)
    write_start_index_ = 0;
#endif

    if (config_.verbose) {
        debugDumpBufferStates("AFTER_SETRINGMODE");
    }

    configureEventClasses();

    LOG_INFO("Self-Managed Ring Mode ready");
    return 0;
}

// ============================================================================
// Debug: dump buffer states
// ============================================================================

void TracerEngine::debugDumpBufferStates(const char* label) {
    if (!kernel_buffers_ || total_kernel_buffers_ <= 0) return;

    int active_count = 0;
    uint32_t total_events = 0;

    for (int i = 0; i < total_kernel_buffers_; ++i) {
        uint32_t n = kernel_buffers_[i].h.num_events;
        if (n > 0 && n <= constants::MAX_EVENTS_PER_KBUF) {
            ++active_count;
            total_events += n;
        }
    }

    LOG_DEBUG("[DBG-BUF] {} active={}/{} total_events={}",
             label, active_count, total_kernel_buffers_, total_events);
}

// ============================================================================
// scanValidRange — scan ring buffer for contiguous valid buffers
// ============================================================================

TracerEngine::ScanResult
TracerEngine::scanValidRange(const tracebuf_t* kb, int N,
                             int write_start, uint32_t min_seq) {
    ScanResult result;
    result.start = write_start;

    for (int j = 0; j < N; j++) {
        int idx = (write_start + j) % N;
        uint32_t ne = kb[idx].h.num_events;
        if (ne == 0 || ne > constants::MAX_EVENTS_PER_KBUF) {
            break;
        }
        uint32_t seq = kb[idx].h.seq_buff_num;
        if (min_seq > 0 && seq <= min_seq) {
            break;
        }
        if (seq > result.max_seq) result.max_seq = seq;
        result.count++;
    }

    return result;
}

#endif  // __QNX__

// ============================================================================
// Run loop: START → wait for SIGINT → STOP → dump → exit
// ============================================================================

void TracerEngine::runLoop() {
    LOG_INFO("Starting Ring Mode trace...");
    LOG_INFO("Press Ctrl+C to stop and dump.");

#ifdef __QNX__
    trace_start_time_ = std::chrono::system_clock::now();
    TraceEvent(_NTO_TRACE_STARTNOSTATE);

    while (!stop_requested_.load()) {
        usleep(100000);  // 100ms poll
    }

    dumpSelfManaged(0, false);
    LOG_INFO("Trace stopped");
#else
    LOG_WARN("Non-QNX platform, trace not available");
#endif
}

// ============================================================================
// dumpSelfManaged: STOP → FLUSH → scan → write QST v4 → (optional) restart
// ============================================================================

void TracerEngine::dumpSelfManaged(int event_type, bool restart) {
#ifdef __QNX__
    LOG_INFO("Dumping ring buffers (event={})...", event_type);

    if (config_.verbose) {
        debugDumpBufferStates("BEFORE_STOP");
    }

    TraceEvent(_NTO_TRACE_STOP);
    auto trace_end_time = std::chrono::system_clock::now();

    TraceEvent(_NTO_TRACE_FLUSHBUFFER);

    if (config_.verbose) {
        debugDumpBufferStates("AFTER_STOP_FLUSH");
    }

    // Phase 1: scan DATA valid range
    int total_data = 0;
    uint32_t data_max_seq = 0;

#if !defined(LP8797)
    auto data_range = scanValidRange(kernel_buffers_, total_kernel_buffers_,
                                     write_start_index_, 0);
    total_data = data_range.count;
    data_max_seq = data_range.max_seq;
    if (data_range.count > 0) {
        write_start_index_ = (data_range.start + data_range.count) % total_kernel_buffers_;
    }
    LOG_INFO("DATA scan: start={} count={} max_seq={}",
             data_range.start, data_range.count, data_max_seq);
#else
    int num_cpus = getCpuCount();
    std::vector<ScanResult> data_ranges(num_cpus);
    for (int cpu = 0; cpu < num_cpus; cpu++) {
        data_ranges[cpu] = scanValidRange(
            &kernel_buffers_[cpu * bufs_per_cpu_], bufs_per_cpu_,
            cpu_write_start_[cpu], 0);
        total_data += data_ranges[cpu].count;
        if (data_ranges[cpu].max_seq > data_max_seq) {
            data_max_seq = data_ranges[cpu].max_seq;
        }
        if (data_ranges[cpu].count > 0) {
            cpu_write_start_[cpu] =
                (data_ranges[cpu].start + data_ranges[cpu].count) % bufs_per_cpu_;
        }
    }
    LOG_INFO("DATA scan total: {} buffers, global_max_seq={}", total_data, data_max_seq);
#endif

    if (total_data == 0) {
        LOG_WARN("No valid data buffers found, skipping dump");
        if (restart) {
            TraceEvent(_NTO_TRACE_SETRINGMODE);
            configureEventClasses();
            trace_start_time_ = std::chrono::system_clock::now();
            TraceEvent(_NTO_TRACE_STARTNOSTATE);
        }
        return;
    }

    // Phase 2: open file, write FileHeader + DATA section
    uint32_t bpc = 0;
#if defined(LP8797)
    bpc = static_cast<uint32_t>(bufs_per_cpu_);
#endif

    if (data_manager_->openQstFile(data_buffer_.clockFreq(),
                                    trace_start_time_, trace_end_time,
                                    event_type, bpc) != 0) {
        LOG_ERROR("Failed to open QST file, aborting dump");
        if (restart) {
            TraceEvent(_NTO_TRACE_SETRINGMODE);
            configureEventClasses();
            trace_start_time_ = std::chrono::system_clock::now();
            TraceEvent(_NTO_TRACE_STARTNOSTATE);
        }
        return;
    }

    bool write_ok = true;

    if (data_manager_->writeSectionHeader(v4::DATA_MAGIC, total_data) != 0) {
        LOG_ERROR("Failed to write DATA section header");
        write_ok = false;
    }

    if (write_ok) {
#if !defined(LP8797)
        if (data_manager_->writeContiguousTracebufs(
                kernel_buffers_, total_kernel_buffers_,
                data_range.start, data_range.count) != 0) {
            LOG_ERROR("Failed to write DATA payload");
            write_ok = false;
        }
#else
        for (int cpu = 0; cpu < num_cpus; cpu++) {
            if (data_ranges[cpu].count == 0) continue;
            if (data_manager_->writeContiguousTracebufs(
                    &kernel_buffers_[cpu * bufs_per_cpu_], bufs_per_cpu_,
                    data_ranges[cpu].start, data_ranges[cpu].count) != 0) {
                LOG_ERROR("Failed to write DATA payload for CPU{}", cpu);
                write_ok = false;
                break;
            }
        }
#endif
    }

    if (!write_ok) {
        LOG_ERROR("DATA write failed, closing file");
        data_manager_->closeQstFile();
        if (restart) {
            TraceEvent(_NTO_TRACE_SETRINGMODE);
            configureEventClasses();
            trace_start_time_ = std::chrono::system_clock::now();
            TraceEvent(_NTO_TRACE_STARTNOSTATE);
        }
        return;
    }

    LOG_INFO("[v4] DATA section written ({} tracebuf_t blocks)", total_data);

    // Phase 3: collect process/thread info (START → usleep → STOP → FLUSH)
    if (config_.verbose) {
        debugDumpBufferStates("BEFORE_PROCINFO");
    }

    int pi_ret = TraceEvent(_NTO_TRACE_START);
    if (pi_ret != 0) {
        LOG_WARN("_NTO_TRACE_START for procinfo returned: {}", pi_ret);
    }
    usleep(2000);
    TraceEvent(_NTO_TRACE_STOP);
    TraceEvent(_NTO_TRACE_FLUSHBUFFER);

    if (config_.verbose) {
        debugDumpBufferStates("AFTER_PROCINFO_FLUSH");
    }

    // Phase 4: scan PINF + write PINF section
    int total_pinf = 0;

#if !defined(LP8797)
    auto pinf_range = scanValidRange(kernel_buffers_, total_kernel_buffers_,
                                     write_start_index_, data_max_seq);
    total_pinf = pinf_range.count;
    if (pinf_range.count > 0) {
        write_start_index_ = (pinf_range.start + pinf_range.count) % total_kernel_buffers_;
    }
    LOG_INFO("PINF scan: start={} count={}", pinf_range.start, pinf_range.count);
#else
    std::vector<ScanResult> pinf_ranges(num_cpus);
    for (int cpu = 0; cpu < num_cpus; cpu++) {
        pinf_ranges[cpu] = scanValidRange(
            &kernel_buffers_[cpu * bufs_per_cpu_], bufs_per_cpu_,
            cpu_write_start_[cpu], data_max_seq);
        total_pinf += pinf_ranges[cpu].count;
        if (pinf_ranges[cpu].count > 0) {
            cpu_write_start_[cpu] =
                (pinf_ranges[cpu].start + pinf_ranges[cpu].count) % bufs_per_cpu_;
        }
    }
    LOG_INFO("PINF scan total: {} buffers", total_pinf);
#endif

    data_manager_->writeSectionHeader(v4::PINF_MAGIC, total_pinf);

#if !defined(LP8797)
    if (total_pinf > 0) {
        data_manager_->writeContiguousTracebufs(
            kernel_buffers_, total_kernel_buffers_,
            pinf_range.start, pinf_range.count);
    }
#else
    for (int cpu = 0; cpu < num_cpus; cpu++) {
        if (pinf_ranges[cpu].count == 0) continue;
        data_manager_->writeContiguousTracebufs(
            &kernel_buffers_[cpu * bufs_per_cpu_], bufs_per_cpu_,
            pinf_ranges[cpu].start, pinf_ranges[cpu].count);
    }
#endif

    data_manager_->closeQstFile();
    ++buffers_processed_;
    LOG_INFO("Saved: {}", data_manager_->lastFilename());

    // Phase 5: optional restart
    if (restart) {
        TraceEvent(_NTO_TRACE_SETRINGMODE);
        configureEventClasses();
        trace_start_time_ = std::chrono::system_clock::now();
        TraceEvent(_NTO_TRACE_STARTNOSTATE);
        LOG_INFO("Trace resumed");
    }
#endif  // __QNX__
}

// ============================================================================
// Cleanup
// ============================================================================

void TracerEngine::cleanup() {
    LOG_INFO("Cleaning up...");

#ifdef __QNX__
    TraceEvent(_NTO_TRACE_STOP);

    if (kernel_buffers_ != nullptr && kernel_buffers_ != MAP_FAILED) {
#if !defined(LP8797)
        size_t total_size = total_kernel_buffers_ * sizeof(tracebuf_t);
        munmap(kernel_buffers_, total_size);
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
