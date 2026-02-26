/**
 * @file types.hpp
 * @brief QSchedTracer - 类型定义和数据结构
 * 
 * @details
 * 本文件定义了调度追踪器使用的所有数据结构。
 * 
 * ## 核心数据结构
 * 
 * 1. **QstEvent** - QNX 内核 trace 事件 (16 字节)
 * 2. **FileHeader** - 文件头格式 (v3，64 字节)
 * 3. **ProcInfoHeader** - 进程/线程信息头 (16 字节)
 * 4. **MainDataHeader** - 主调度数据头 (16 字节)
 * 
 * ## 文件格式 (.qst v3)
 * 
 * ```
 * ┌──────────────────────────────────────────────────────────────┐
 * │ FileHeader (64 字节)                                         │
 * │   - magic: 'QST3'                                            │
 * │   - clock_freq: 时钟频率 (Hz)                                │
 * │   - wallclock_sec/nsec: 采集结束时的系统时间                  │
 * │   - sync_cycles: 最后一个事件的 cycles (32位)                │
 * ├──────────────────────────────────────────────────────────────┤
 * │ ProcInfoHeader (16 字节)                                     │
 * │   - magic: 'PINF'                                            │
 * │   - event_count: 事件数量                                    │
 * ├──────────────────────────────────────────────────────────────┤
 * │ QstEvent[0..M-1] - 进程/线程信息                             │
 * │   - 仅提取 PROCDESTROY 获取名称，忽略调度事件                │
 * ├──────────────────────────────────────────────────────────────┤
 * │ MainDataHeader (16 字节)                                     │
 * │   - magic: 'MAIN'                                            │
 * │   - event_count: 事件数量                                    │
 * ├──────────────────────────────────────────────────────────────┤
 * │ QstEvent[0..N-1] - 主调度数据 (按时间顺序写入)               │
 * │   - 最后一个事件的 data[0] == sync_cycles                    │
 * └──────────────────────────────────────────────────────────────┘
 * ```
 * 
 * @note 本文件仅支持 QNX Neutrino RTOS 平台
 * 
 * @author QSchedTracer Team
 * @date 2026-01-22
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <atomic>

// ============================================================================
// 平台相关头文件 (必须在 namespace 外部 include！)
// ============================================================================

#include <sys/trace.h>

namespace qst {

// ============================================================================
// 类型定义
// ============================================================================

// 使用系统提供的 traceevent_t
// 注意：不能命名为 TraceEvent，因为与 QNX 系统函数 TraceEvent() 冲突
using QstEvent = ::traceevent_t;

// ============================================================================
// 常量定义
// ============================================================================

namespace constants {

/** 文件魔数 "QST3" (版本 3) */
constexpr uint32_t MAGIC = 0x51535433;

/** 文件格式版本 */
constexpr uint32_t VERSION = 3;

/** 单个事件大小 (16 bytes) */
constexpr size_t EVENT_SIZE = sizeof(QstEvent);

/** 默认缓冲区大小 (10MB) */
constexpr size_t DEFAULT_BUFFER_SIZE = 10 * 1024 * 1024;

/** 内核 trace buffer 数量 (推荐值)，从16-->32修改为tracelogger的默认值 */
constexpr int KERNEL_BUFFER_COUNT = 32;

/** 进程/线程信息缓冲区大小 (3MB) */
constexpr size_t PROCINFO_BUFFER_SIZE = 3 * 1024 * 1024;

/** 进程/线程信息头魔数 "PINF" */
constexpr uint32_t PROCINFO_MAGIC = 0x50494E46;

/** 主调度数据头魔数 "MAIN" */
constexpr uint32_t MAINDATA_MAGIC = 0x4D41494E;

/** Pulse 消息代码 (1-127 为用户定义，负数保留给系统) */
constexpr int8_t PULSE_CODE_BUFFER_READY = 1;  ///< 内核 buffer 已复制到用户缓冲区，实际这里没有作用，只是保留做扩展的
constexpr int8_t PULSE_CODE_TRIGGER      = 2;  ///< 触发器触发，需要落盘
constexpr int8_t PULSE_CODE_STOP         = 3;  ///< 请求停止采集

/** 触发源标识 (通过 pulse.value.sival_int 传递) */
constexpr int32_t TRIGGER_SOURCE_KERNEL_EVENT = 1;  ///< 内核事件触发
constexpr int32_t TRIGGER_SOURCE_TOPIC        = 2;  ///< Topic 订阅触发 (预留)
constexpr int32_t TRIGGER_SOURCE_SIGNAL       = 3;  ///< 信号触发 (优雅退出)

} // namespace constants

// ============================================================================
// 采集器状态
// ============================================================================

enum class State : int {
    Idle = 0,       ///< 空闲
    Running,        ///< 采集中
    Stopping,       ///< 停止中
    Dumping         ///< 落盘中
};

// ============================================================================
// 文件格式数据结构 (简化版 v3)
// ============================================================================

/**
 * @brief .qst 文件头格式 (v3，64 字节)
 * 
 * 包含真实时间同步点信息：
 * - wallclock_sec/nsec: 采集结束时的系统时间 (UNIX 时间戳)
 * - sync_cycles: 采集结束时最后一个事件的 32 位 cycles 值
 * 
 * 解析器从最后一个事件向前推算每个事件的真实时间：
 *   event_time = wallclock - (sync_cycles - event_cycles) / clock_freq
 */
struct __attribute__((packed)) FileHeader {
    uint32_t magic;             ///< 文件魔数 'QST3' (0x51535433)
    uint32_t version;           ///< 文件格式版本 (3)
    uint64_t clock_freq;        ///< 时钟频率 (Hz)
    int64_t  wallclock_sec;     ///< 采集结束的系统时间 (秒，UNIX 时间戳)
    int64_t  wallclock_nsec;    ///< 采集结束的系统时间 (纳秒部分，0-999999999)
    uint32_t sync_cycles;       ///< 最后一条事件的 cycles (32位，与 wallclock 同步)
    uint32_t reserved[7];       ///< 保留扩展 (填充到 64 字节)
};

static_assert(sizeof(FileHeader) == 64, "FileHeader must be 64 bytes");

/**
 * @brief 进程/线程信息头 (16 字节)
 * 
 * 在 FileHeader 之后，存储进程/线程名称信息。
 * 这些数据是在落盘前通过 _NTO_TRACE_START 获取的。
 * 解析时仅提取 PROCDESTROY 事件获取名称，忽略调度事件。
 */
struct __attribute__((packed)) ProcInfoHeader {
    uint32_t magic;             ///< 魔数 'PINF' (0x50494E46)
    uint32_t event_count;       ///< 事件数量
    uint64_t reserved;          ///< 保留
};

static_assert(sizeof(ProcInfoHeader) == 16, "ProcInfoHeader must be 16 bytes");

/**
 * @brief 主调度数据头 (16 字节)
 * 
 * 在进程/线程信息之后，存储主调度事件数据。
 * 数据已按时间顺序写入，最后一个事件的 data[0] == sync_cycles。
 */
struct __attribute__((packed)) MainDataHeader {
    uint32_t magic;             ///< 魔数 'MAIN' (0x4D41494E)
    uint32_t event_count;       ///< 事件数量
    uint64_t reserved;          ///< 保留
};

static_assert(sizeof(MainDataHeader) == 16, "MainDataHeader must be 16 bytes");

} // namespace qst

