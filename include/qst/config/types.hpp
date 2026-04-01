/**
 * @file config/types.hpp
 * @brief QSchedTracer - 配置数据结构
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace qst {
namespace config {

enum class EventFilter {
    Scheduling,  ///< 仅调度相关事件 (Thread/Comm/KerCall 等, Wide Mode)
    All          ///< 全量采集: ADDALLCLASSES + SETCLASSWIDE
};

struct TracerConfig {
    uint32_t buffer_size_mb = 10;
    EventFilter event_filter = EventFilter::Scheduling;
    std::string output_dir = ".";
    std::string file_prefix = "tracer";
    bool verbose = false;

    /**
     * @brief 根据 buffer_size_mb 和 CPU 数量计算内核 buffer 数量
     *
     * QNX 7.1: 总 buffer 数
     * QNX 8.0 (LP8797): 每 CPU 的 buffer 数
     * 每个 kernel buffer 约 16KB (sizeof(tracebuf_t))
     */
    int computeKernelBufferCount(int num_cpus) const {
        int total_buffers = static_cast<int>(buffer_size_mb) * 1024 / 16;
#if defined(LP8797)
        return (num_cpus > 0) ? (total_buffers / num_cpus) : total_buffers;
#else
        (void)num_cpus;
        return total_buffers;
#endif
    }
};

}  // namespace config
}  // namespace qst
