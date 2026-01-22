/**
 * @file ring_buffer.cpp
 * @brief QSchedTracer - 环形缓冲区实现
 * 
 * @details
 * 单一环形缓冲区实现，针对中断上下文优化。
 * 
 * ## 设计原则
 * 
 * 1. **简单高效**: 单一连续缓冲区，无 Per-CPU 分离
 * 2. **中断安全**: 写入操作使用 memcpy，无锁设计
 * 3. **环形覆盖**: 缓冲区满时自动覆盖最旧数据
 * 
 * ## 内存布局
 * 
 * ```
 * buffer[0]              write_pos          buffer_size-1
 *    │                      │                    │
 *    ▼                      ▼                    ▼
 *  ┌────┬────┬────┬────┬────┬────┬────┬────┬────┐
 *  │ E0 │ E1 │ E2 │ E3 │    │    │    │    │    │
 *  └────┴────┴────┴────┴────┴────┴────┴────┴────┘
 *    ↑                    ↑
 *  最旧数据            下次写入
 * 
 * 当写满后 (wrap_count > 0):
 *  ┌────┬────┬────┬────┬────┬────┬────┬────┬────┐
 *  │ E8 │ E9 │ E2 │ E3 │ E4 │ E5 │ E6 │ E7 │    │
 *  └────┴────┴────┴────┴────┴────┴────┴────┴────┘
 *          ↑                                   ↑
 *       write_pos                          最旧数据
 * ```
 * 
 * @author QSchedTracer Team
 * @date 2026-01-21
 */

#include "qst/ring_buffer.hpp"
#include "qst/log.hpp"
#include <cstdlib>
#include <cstring>
#include <new>

namespace qst {

// ============================================================================
// 内部函数
// ============================================================================

namespace {

/**
 * @brief 分配对齐内存
 * 
 * 使用 64 字节对齐以优化缓存性能。
 */
void* alignedAlloc(size_t alignment, size_t size) {
    void* ptr = nullptr;
    
#ifdef __QNXNTO__
    // QNX: 使用 posix_memalign
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return nullptr;
    }
#else
    // Linux/其他: 使用 aligned_alloc
    ptr = std::aligned_alloc(alignment, size);
#endif
    
    return ptr;
}

} // anonymous namespace

// ============================================================================
// RingBuffer 实现
// ============================================================================

RingBuffer::RingBuffer(size_t buffer_size) {
    // 确保 buffer_size 是事件大小的整数倍
    buffer_size = (buffer_size / constants::EVENT_SIZE) * constants::EVENT_SIZE;
    if (buffer_size < constants::EVENT_SIZE) {
        LOG_ERROR("缓冲区太小: {} 字节 (最小需要 {})", 
                  buffer_size, constants::EVENT_SIZE);
        throw std::bad_alloc();
    }
    
    LOG_INFO("初始化环形缓冲区: 大小={} 字节 ({} 个事件)", 
             buffer_size, buffer_size / constants::EVENT_SIZE);
    
    // 分配对齐内存 (64 字节对齐)
    buffer_ = static_cast<uint8_t*>(alignedAlloc(64, buffer_size));
    if (buffer_ == nullptr) {
        LOG_ERROR("内存分配失败: {} 字节", buffer_size);
        throw std::bad_alloc();
    }
    
    // 初始化字段
    buffer_size_ = buffer_size;
    capacity_ = buffer_size / constants::EVENT_SIZE;
    
    LOG_INFO("初始化成功: buffer={}, 容量={} 个事件", 
             static_cast<void*>(buffer_), capacity_);
}

RingBuffer::~RingBuffer() {
    LOG_INFO("销毁环形缓冲区: 共处理 {} 个事件, {} 次环绕", 
             static_cast<size_t>(event_count_), static_cast<size_t>(wrap_count_));
    
    if (buffer_ != nullptr) {
        std::free(buffer_);
        buffer_ = nullptr;
    }
}

RingBuffer::RingBuffer(RingBuffer&& other) noexcept
    : state_(other.state_.load())
    , buffer_(other.buffer_)
    , buffer_size_(other.buffer_size_)
    , capacity_(other.capacity_)
    , write_pos_(other.write_pos_)
    , event_count_(other.event_count_)
    , wrap_count_(other.wrap_count_)
    , start_time_(other.start_time_)
    , end_time_(other.end_time_)
    , clock_freq_(other.clock_freq_)
    , wallclock_sec_(other.wallclock_sec_)
    , wallclock_nsec_(other.wallclock_nsec_)
    , sync_cycles_(other.sync_cycles_)
    , total_bytes_(other.total_bytes_)
    , buffers_received_(other.buffers_received_)
{
    other.buffer_ = nullptr;
    other.buffer_size_ = 0;
}

RingBuffer& RingBuffer::operator=(RingBuffer&& other) noexcept {
    if (this != &other) {
        if (buffer_ != nullptr) {
            std::free(buffer_);
        }
        
        state_.store(other.state_.load());
        buffer_ = other.buffer_;
        buffer_size_ = other.buffer_size_;
        capacity_ = other.capacity_;
        write_pos_ = other.write_pos_;
        event_count_ = other.event_count_;
        wrap_count_ = other.wrap_count_;
        start_time_ = other.start_time_;
        end_time_ = other.end_time_;
        clock_freq_ = other.clock_freq_;
        wallclock_sec_ = other.wallclock_sec_;
        wallclock_nsec_ = other.wallclock_nsec_;
        sync_cycles_ = other.sync_cycles_;
        total_bytes_ = other.total_bytes_;
        buffers_received_ = other.buffers_received_;
        
        other.buffer_ = nullptr;
        other.buffer_size_ = 0;
    }
    return *this;
}

void RingBuffer::write(const void* data, size_t nbytes) {
    if (buffer_ == nullptr || data == nullptr || nbytes == 0) {
        return;
    }
    
    // 限制写入大小不超过缓冲区大小
    if (nbytes > buffer_size_) {
        // 如果数据大于缓冲区，只保留最后一部分
        size_t skip = nbytes - buffer_size_;
        data = static_cast<const uint8_t*>(data) + skip;
        nbytes = buffer_size_;
    }
    
    size_t pos = write_pos_;
    size_t buf_size = buffer_size_;
    
    // 检查是否需要环绕
    if (pos + nbytes <= buf_size) {
        // 不需要环绕，直接 memcpy
        std::memcpy(buffer_ + pos, data, nbytes);
        write_pos_ = pos + nbytes;
    } else {
        // 需要环绕，分两次 memcpy
        size_t first_part = buf_size - pos;
        size_t second_part = nbytes - first_part;
        
        // 复制到缓冲区末尾
        std::memcpy(buffer_ + pos, data, first_part);
        
        // 复制到缓冲区开头
        std::memcpy(buffer_, static_cast<const uint8_t*>(data) + first_part, second_part);
        
        write_pos_ = second_part;
        wrap_count_++;
    }
    
    // 更新统计
    total_bytes_ += nbytes;
    event_count_ += nbytes / constants::EVENT_SIZE;
    buffers_received_++;
}

std::tuple<void*, size_t, size_t> RingBuffer::getData() const {
    if (buffer_ == nullptr) {
        return {nullptr, 0, 0};
    }
    
    // 计算实际数据大小
    size_t actual_size;
    size_t actual_events;
    
    if (wrap_count_ > 0) {
        // 发生过环绕，缓冲区是满的
        actual_size = buffer_size_;
        actual_events = capacity_;
    } else {
        // 未环绕，只有部分数据
        actual_size = write_pos_;
        actual_events = write_pos_ / constants::EVENT_SIZE;
    }
    
    return {buffer_, actual_size, actual_events};
}

void RingBuffer::reset() {
    write_pos_ = 0;
    event_count_ = 0;
    wrap_count_ = 0;
    total_bytes_ = 0;
    buffers_received_ = 0;
    start_time_ = 0;
    end_time_ = 0;
    
    // v3: 真实时间同步点
    wallclock_sec_ = 0;
    wallclock_nsec_ = 0;
    sync_cycles_ = 0;
    
    LOG_INFO("缓冲区已重置");
}

void RingBuffer::dumpState() const {
    std::fprintf(stderr, "\n");
    std::fprintf(stderr, "╔══════════════════════════════════════════════════╗\n");
    std::fprintf(stderr, "║              环形缓冲区状态                      ║\n");
    std::fprintf(stderr, "╠══════════════════════════════════════════════════╣\n");
    std::fprintf(stderr, "║ 缓冲区地址:    %-20p           ║\n", static_cast<void*>(buffer_));
    std::fprintf(stderr, "║ 缓冲区大小:    %-15zu 字节        ║\n", buffer_size_);
    std::fprintf(stderr, "║ 事件容量:      %-15zu 个          ║\n", capacity_);
    std::fprintf(stderr, "║ 写入位置:      %-15zu 字节        ║\n", static_cast<size_t>(write_pos_));
    std::fprintf(stderr, "║ 事件数量:      %-15zu 个          ║\n", static_cast<size_t>(event_count_));
    std::fprintf(stderr, "║ 环绕次数:      %-15zu 次          ║\n", static_cast<size_t>(wrap_count_));
    std::fprintf(stderr, "║ 总写入字节:    %-15zu 字节        ║\n", static_cast<size_t>(total_bytes_));
    std::fprintf(stderr, "║ 收到 Buffer:   %-15zu 个          ║\n", static_cast<size_t>(buffers_received_));
    std::fprintf(stderr, "║ 状态:          %-15d               ║\n", static_cast<int>(state_.load()));
    std::fprintf(stderr, "╠══════════════════════════════════════════════════╣\n");
    std::fprintf(stderr, "║ 真实时间同步点 (v3)                              ║\n");
    std::fprintf(stderr, "║ Wallclock:     %lld.%09lld                ║\n", 
            static_cast<long long>(wallclock_sec_), static_cast<long long>(wallclock_nsec_));
    std::fprintf(stderr, "║ Sync Cycles:   %-20llu           ║\n", 
            static_cast<unsigned long long>(sync_cycles_));
    std::fprintf(stderr, "╚══════════════════════════════════════════════════╝\n");
    std::fprintf(stderr, "\n");
}

} // namespace qst

