/**
 * @file ring_buffer.hpp
 * @brief QSchedTracer - 环形缓冲区类
 * 
 * @details
 * 提供单一环形缓冲区的操作接口，设计用于中断上下文中高效写入。
 * 
 * ## 设计特点
 * 
 * 1. **单一缓冲区**: 不按 CPU 分离，简化实现
 * 2. **memcpy 整块复制**: 中断回调中使用 memcpy 复制整个内核 buffer
 * 3. **CPU 信息保留在事件中**: 由解析器从事件 header 提取 CPU ID
 * 
 * ## 使用示例
 * 
 * ```cpp
 * // 初始化
 * qst::RingBuffer rb(4 * 1024 * 1024);  // 4MB 缓冲区
 * 
 * // 在中断回调中写入整块数据
 * rb.write(kernel_buffer->data, nbytes);
 * 
 * // 获取数据用于落盘
 * auto [data, size, count] = rb.getData();
 * write(fd, data, size);
 * ```
 * 
 * @author QSchedTracer Team
 * @date 2026-01-21
 */

#pragma once

#include "types.hpp"
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <tuple>

namespace qst {

/**
 * @brief 环形缓冲区类 (单一 buffer，无 Per-CPU 分离)
 * 
 * 设计特点：
 * - 不按 CPU 分离，中断回调直接 memcpy 整块数据
 * - CPU 信息保留在事件 header 中，由解析器提取
 * - 最大程度减少中断处理时间
 * - 支持真实时间戳：记录 wallclock + sync_cycles 同步点
 */
class RingBuffer {
public:
    /**
     * @brief 构造函数
     * @param buffer_size 缓冲区大小 (字节)
     */
    explicit RingBuffer(size_t buffer_size = constants::DEFAULT_BUFFER_SIZE);
    
    /**
     * @brief 析构函数
     */
    ~RingBuffer();
    
    // 禁止拷贝
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    
    // 允许移动
    RingBuffer(RingBuffer&& other) noexcept;
    RingBuffer& operator=(RingBuffer&& other) noexcept;
    
    /**
     * @brief 写入数据到缓冲区 (整块复制)
     * 
     * 设计用于中断上下文，使用 memcpy 高效复制。
     * 支持环形覆盖：当缓冲区满时自动覆盖最旧的数据。
     * 
     * @param data 源数据指针 (TraceEvent 数组)
     * @param nbytes 数据大小 (字节)
     * 
     * @note 此函数可在中断上下文中安全调用
     */
    void write(const void* data, size_t nbytes);
    
    /**
     * @brief 获取缓冲区数据用于落盘
     * 
     * @return tuple<数据指针, 数据大小, 事件数量>
     * 
     * @note 返回的数据可能不是从缓冲区开头开始 (如果发生过环绕)
     */
    std::tuple<void*, size_t, size_t> getData() const;
    
    /**
     * @brief 重置缓冲区
     */
    void reset();
    
    /**
     * @brief 打印缓冲区状态 (调试用)
     */
    void dumpState() const;
    
    // === Getters ===
    
    uint8_t* buffer() const { return buffer_; }
    size_t bufferSize() const { return buffer_size_; }
    size_t capacity() const { return capacity_; }
    size_t writePos() const { return write_pos_; }
    size_t eventCount() const { return event_count_; }
    size_t wrapCount() const { return wrap_count_; }
    uint64_t totalBytes() const { return total_bytes_; }
    uint64_t buffersReceived() const { return buffers_received_; }
    
    State state() const { return state_.load(); }
    void setState(State s) { state_.store(s); }
    
    // === 时间信息 ===
    
    uint64_t startTime() const { return start_time_; }
    void setStartTime(uint64_t t) { start_time_ = t; }
    
    uint64_t endTime() const { return end_time_; }
    void setEndTime(uint64_t t) { end_time_ = t; }
    
    uint64_t clockFreq() const { return clock_freq_; }
    void setClockFreq(uint64_t f) { clock_freq_ = f; }
    
    // === v3: 真实时间同步点 ===
    
    int64_t wallclockSec() const { return wallclock_sec_; }
    void setWallclockSec(int64_t s) { wallclock_sec_ = s; }
    
    int64_t wallclockNsec() const { return wallclock_nsec_; }
    void setWallclockNsec(int64_t ns) { wallclock_nsec_ = ns; }
    
    uint64_t syncCycles() const { return sync_cycles_; }
    void setSyncCycles(uint64_t c) { sync_cycles_ = c; }

private:
    // 缓冲区状态
    std::atomic<State> state_{State::Idle};
    
    // 缓冲区指针和大小
    uint8_t* buffer_{nullptr};
    size_t buffer_size_{0};
    size_t capacity_{0};  // 最大事件数
    
    // 写入位置
    volatile size_t write_pos_{0};
    volatile size_t event_count_{0};
    volatile size_t wrap_count_{0};
    
    // 时间信息
    uint64_t start_time_{0};
    uint64_t end_time_{0};
    uint64_t clock_freq_{0};
    
    // v3: 真实时间同步点 (采集结束时)
    int64_t wallclock_sec_{0};
    int64_t wallclock_nsec_{0};
    uint64_t sync_cycles_{0};
    
    // 统计信息
    volatile uint64_t total_bytes_{0};
    volatile uint64_t buffers_received_{0};
};

} // namespace qst

