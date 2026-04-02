/**
 * @file core/tracer_ring_dump.cpp
 * @brief TracerEngine - Ring Buffer 管理与数据转储
 *
 * 包含 setupSelfManagedMode、scanValidRange、
 * debugDumpBufferStates、dumpSelfManaged 等 Ring Mode 核心操作。
 */

#include "qst/core/tracer_engine.hpp"
#include "qst/log.hpp"

#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <vector>
#include <chrono>

#ifdef __QNX__
#include <sys/neutrino.h>
#include <sys/trace.h>
#include <sys/mman.h>
#endif

namespace qst {
namespace core {

// ============================================================================
// Self-Managed Ring Mode setup (QNX only)
// ============================================================================

#ifdef __QNX__

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
                                    bpc) != 0) {
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

}  // namespace core
}  // namespace qst
