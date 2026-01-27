/**
 * @file core/data_buffer.hpp
 * @brief QSchedTracer - 数据缓冲区类
 * 
 * @details
 * 提供环形数据缓冲区的操作接口，设计用于中断上下文中高效写入。
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
 * qst::DataBuffer db(4 * 1024 * 1024);  // 4MB 缓冲区
 * 
 * // 在中断回调中写入整块数据
 * db.write(kernel_buffer->data, nbytes);
 * 
 * // 获取数据用于落盘
 * auto [data, size, count] = db.getData();
 * write(fd, data, size);
 * ```
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#pragma once

#include "qst/types.hpp"
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <tuple>
#include <memory>

namespace qst {

/**
 * @brief 数据缓冲区类 (环形 buffer，无 Per-CPU 分离)
 * 
 * 设计特点：
 * - 不按 CPU 分离，中断回调直接 memcpy 整块数据
 * - CPU 信息保留在事件 header 中，由解析器提取
 * - 最大程度减少中断处理时间
 * - 支持真实时间戳：记录 wallclock + sync_cycles 同步点
 */
class DataBuffer {
public:
    /**
     * @brief 构造函数
     * @param buffer_size 缓冲区大小 (字节)
     */
    explicit DataBuffer(size_t buffer_size = constants::DEFAULT_BUFFER_SIZE);
    
    /**
     * @brief 析构函数
     */
    ~DataBuffer();
    
    // 禁止拷贝
    DataBuffer(const DataBuffer&) = delete;
    DataBuffer& operator=(const DataBuffer&) = delete;
    
    // 允许移动
    DataBuffer(DataBuffer&& other) noexcept;
    DataBuffer& operator=(DataBuffer&& other) noexcept;
    
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
     * @brief 获取缓冲区原始数据 (可能不按时间顺序)
     * 
     * @return tuple<数据指针, 数据大小, 事件数量>
     * 
     * @note 如果发生过环绕，数据不是按时间顺序的
     * @see getOrderedData() 获取按时间顺序的数据
     */
    std::tuple<void*, size_t, size_t> getData() const;
    
    /**
     * @brief 获取按时间顺序排列的缓冲区数据
     * 
     * 如果缓冲区发生过环绕，会重组数据：
     * - 原始: [新数据][旧数据] (write_pos 之前是新，之后是旧)
     * - 输出: [旧数据][新数据] (按时间顺序)
     * 
     * @param[out] out_size 输出数据大小 (字节)
     * @param[out] out_event_count 输出事件数量
     * @param[out] out_sync_cycles 输出最后一个事件的 cycles (用于时间同步)
     * 
     * @return unique_ptr 管理的缓冲区，无数据时返回 nullptr
     * 
     * @note 只需一次内存分配和复制，高效且安全
     */
    std::unique_ptr<uint8_t[]> getOrderedData(size_t* out_size, 
                                               size_t* out_event_count,
                                               uint32_t* out_sync_cycles) const;
    
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
    size_t wrapCount() const { return wrap_count_; }
    
    State state() const { return state_.load(); }
    void setState(State s) { state_.store(s); }
    
    // === 时间信息 ===
    
    uint64_t clockFreq() const { return clock_freq_; }
    void setClockFreq(uint64_t f) { clock_freq_ = f; }
    
    int64_t wallclockSec() const { return wallclock_sec_; }
    void setWallclockSec(int64_t s) { wallclock_sec_ = s; }
    
    int64_t wallclockNsec() const { return wallclock_nsec_; }
    void setWallclockNsec(int64_t ns) { wallclock_nsec_ = ns; }

private:
    // 缓冲区状态
    std::atomic<State> state_{State::Idle};
    
    // 缓冲区指针和大小
    uint8_t* buffer_{nullptr};
    size_t buffer_size_{0};
    size_t capacity_{0};  // 最大事件数
    
    // 写入位置 (volatile 用于中断上下文安全)
    volatile size_t write_pos_{0};
    volatile size_t wrap_count_{0};
    
    // 时间信息
    uint64_t clock_freq_{0};        ///< 时钟频率 (cycles per second)
    int64_t wallclock_sec_{0};      ///< 真实时间 (秒)
    int64_t wallclock_nsec_{0};     ///< 真实时间 (纳秒部分)
};

} // namespace qst

