/**
 * @file types.hpp
 * @brief QSchedTracer - 类型定义和 QST v4 文件格式
 *
 * QST v4 文件布局:
 *   QstFileHeader (64B) → SectionHeader (24B, DATA) → tracebuf_t[]
 *                       → SectionHeader (24B, PINF) → tracebuf_t[]
 *
 * 与 deep_os_tool_cpp/tracer 的输出格式完全对齐。
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <atomic>

#ifdef __QNX__
#include <sys/trace.h>
#endif

namespace qst {

#ifdef __QNX__
using QstEvent = ::traceevent_t;
#else
struct QstEvent {
    uint32_t header;
    uint32_t data[3];
};
#endif

namespace constants {

constexpr size_t EVENT_SIZE = sizeof(QstEvent);
constexpr uint32_t MAX_EVENTS_PER_KBUF = 1024;

}  // namespace constants

enum class State : int {
    Idle = 0,
    Running,
    Stopping,
    Dumping
};

namespace v4 {

constexpr uint32_t FILE_MAGIC    = 0x51535434;  // 'QST4'
constexpr uint32_t FILE_VERSION  = 4;
constexpr uint32_t DATA_MAGIC    = 0x44415441;  // 'DATA'
constexpr uint32_t PINF_MAGIC    = 0x50494E46;  // 'PINF'

struct __attribute__((packed)) QstFileHeader {
    uint32_t magic;              ///< 0x51535434 'QST4'
    uint32_t version;            ///< 4
    uint32_t header_size;        ///< sizeof(QstFileHeader) = 64
    uint32_t flags;              ///< bit0: little-endian=0, big-endian=1
    uint64_t clock_freq;         ///< ClockCycles() 频率 (cycles_per_sec)
    int64_t  capture_start_ns;   ///< 采集开始 wall clock (epoch nanoseconds)
    int64_t  capture_end_ns;     ///< 采集结束 wall clock (epoch nanoseconds)
    uint32_t num_cpus;           ///< CPU 数量
    uint32_t os_version;         ///< 710 (QNX 7.1) or 800 (QNX 8.0)
    uint32_t tracebuf_size;      ///< sizeof(tracebuf_t), typically 16384
    uint32_t data_offset;        ///< offsetof(tracebuf_t, data)
    uint8_t  reserved[8];        ///< reserved[0..3] = bufs_per_cpu (QNX 8.0), 0 for 7.1
};

static_assert(sizeof(QstFileHeader) == 64, "QstFileHeader must be 64 bytes");

struct __attribute__((packed)) SectionHeader {
    uint32_t magic;
    uint32_t version;
    uint64_t payload_size;
    uint32_t entry_count;
    uint32_t reserved;
};

static_assert(sizeof(SectionHeader) == 24, "SectionHeader must be 24 bytes");

}  // namespace v4

}  // namespace qst
