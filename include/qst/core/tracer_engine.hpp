/**
 * @file core/tracer_engine.hpp
 * @brief QSchedTracer - 追踪引擎 (Ring Mode + mmap 零拷贝)
 *
 * 支持 QNX 7.1 (mmap 物理地址) 和 QNX 8.0 (LOGGER_ATTACH + per-CPU buffer)。
 * 采用内核 Ring Mode，STOP 时通过 mmap 零拷贝写入 QST v4 文件。
 */

#pragma once

#include "qst/config/types.hpp"
#include "qst/core/data_buffer.hpp"
#include "qst/data/data_manager.hpp"

#include <memory>
#include <atomic>
#include <chrono>
#include <vector>

#ifdef __QNX__
#include <sys/neutrino.h>
#include <sys/trace.h>
#endif

namespace qst {
namespace core {

class TracerEngine {
public:
    explicit TracerEngine(const config::TracerConfig& config);
    ~TracerEngine();

    TracerEngine(const TracerEngine&) = delete;
    TracerEngine& operator=(const TracerEngine&) = delete;
    TracerEngine(TracerEngine&&) = delete;
    TracerEngine& operator=(TracerEngine&&) = delete;

    int run();
    void requestStop();

    DataBuffer& dataBuffer() { return data_buffer_; }
    const config::TracerConfig& config() const { return config_; }

    static int getCpuCount();
    static uint64_t getClockFreq();

    struct ScanResult {
        int start{0};
        int count{0};
        uint32_t max_seq{0};
    };

private:
    int initialize();
    void cleanup();
    void printStats() const;

#ifdef __QNX__
    int initKernelTraceBase();
    int setupSchedulingEvents();
    void setupKerCallEvent(int kercall_id, const char* name);
    void setupKerCallEnterOnly(int kercall_id, const char* name);
    void configureEventClasses();
#endif

    int setupSelfManagedMode();
    void runLoop();

#ifdef __QNX__
    static ScanResult scanValidRange(const tracebuf_t* kb, int N,
                                     int write_start, uint32_t min_seq = 0);

    void debugDumpBufferStates(const char* label);
#endif

    void dumpSelfManaged(int event_type, bool restart);
    void killExistingTracelogger();

private:
    config::TracerConfig config_;
    DataBuffer data_buffer_;
    std::unique_ptr<data::DataManager> data_manager_;

    std::atomic<bool> stop_requested_{false};
    volatile uint64_t buffers_processed_{0};
    int kernel_buffer_count_{0};

#ifdef __QNX__
    tracebuf_t* kernel_buffers_{nullptr};
    int total_kernel_buffers_{0};
    int write_start_index_{0};

#if !defined(LP8797)
    paddr_t kernel_paddr_{0};
#else
    bool logger_attached_{false};
    int bufs_per_cpu_{0};
    std::vector<int> cpu_write_start_;
#endif

#endif  // __QNX__

    std::chrono::system_clock::time_point trace_start_time_;
};

}  // namespace core
}  // namespace qst
