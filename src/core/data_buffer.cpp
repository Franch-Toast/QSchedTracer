/**
 * @file core/data_buffer.cpp
 * @brief QSchedTracer - 数据缓冲区实现
 * 
 * @details
 * 环形数据缓冲区实现，针对中断上下文优化。
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
 * @date 2026-01-26
 */

#include "qst/core/data_buffer.hpp"
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
// DataBuffer 实现
// ============================================================================

DataBuffer::DataBuffer(size_t requested_size) {
    // 确保 requested_size 是事件大小的整数倍
    size_t aligned_size = (requested_size / constants::EVENT_SIZE) * constants::EVENT_SIZE;
    if (aligned_size < constants::EVENT_SIZE) {
        LOG_ERROR("缓冲区太小: {} 字节 (最小需要 {})", 
                  requested_size, constants::EVENT_SIZE);
        throw std::bad_alloc();
    }
    
    LOG_INFO("初始化数据缓冲区: 大小={} 字节 ({} 个事件)", 
             aligned_size, aligned_size / constants::EVENT_SIZE);
    
    // 分配对齐内存 (64 字节对齐)
    buffer_ = static_cast<uint8_t*>(alignedAlloc(64, aligned_size));
    if (buffer_ == nullptr) {
        LOG_ERROR("内存分配失败: {} 字节", aligned_size);
        throw std::bad_alloc();
    }
    
    // 初始化字段
    buffer_size_ = aligned_size;
    capacity_ = aligned_size / constants::EVENT_SIZE;
    
    LOG_INFO("初始化成功: buffer={}, 容量={} 个事件", 
             static_cast<void*>(buffer_), capacity_);
}

DataBuffer::~DataBuffer() {
    LOG_DEBUG("销毁数据缓冲区: {} 次环绕", static_cast<size_t>(wrap_count_));
    
    if (buffer_ != nullptr) {
        std::free(buffer_);
        buffer_ = nullptr;
    }
}

DataBuffer::DataBuffer(DataBuffer&& other) noexcept
    : state_(other.state_.load())
    , buffer_(other.buffer_)
    , buffer_size_(other.buffer_size_)
    , capacity_(other.capacity_)
    , write_pos_(other.write_pos_)
    , wrap_count_(other.wrap_count_)
    , clock_freq_(other.clock_freq_)
    , wallclock_sec_(other.wallclock_sec_)
    , wallclock_nsec_(other.wallclock_nsec_)
{
    other.buffer_ = nullptr;
    other.buffer_size_ = 0;
}

DataBuffer& DataBuffer::operator=(DataBuffer&& other) noexcept {
    if (this != &other) {
        if (buffer_ != nullptr) {
            std::free(buffer_);
        }
        
        state_.store(other.state_.load());
        buffer_ = other.buffer_;
        buffer_size_ = other.buffer_size_;
        capacity_ = other.capacity_;
        write_pos_ = other.write_pos_;
        wrap_count_ = other.wrap_count_;
        clock_freq_ = other.clock_freq_;
        wallclock_sec_ = other.wallclock_sec_;
        wallclock_nsec_ = other.wallclock_nsec_;
        
        other.buffer_ = nullptr;
        other.buffer_size_ = 0;
    }
    return *this;
}

void DataBuffer::write(const void* data, size_t nbytes) {
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
}

std::tuple<void*, size_t, size_t> DataBuffer::getData() const {
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

std::unique_ptr<uint8_t[]> DataBuffer::getOrderedData(size_t* out_size, 
                                                       size_t* out_event_count,
                                                       uint32_t* out_sync_cycles) const {
    // 初始化输出参数
    if (out_size) *out_size = 0;
    if (out_event_count) *out_event_count = 0;
    if (out_sync_cycles) *out_sync_cycles = 0;
    
    if (buffer_ == nullptr) {
        return nullptr;
    }
    
    size_t pos = write_pos_;
    size_t wraps = wrap_count_;
    
    if (wraps == 0) {
        // 未环绕：数据已经按顺序 [0, write_pos)
        size_t data_size = pos;
        
        if (data_size == 0) {
            return nullptr;
        }
        
        size_t event_count = data_size / constants::EVENT_SIZE;
        
        // 直接分配 unique_ptr 管理的内存，一次复制
        auto result = std::make_unique<uint8_t[]>(data_size);
        std::memcpy(result.get(), buffer_, data_size);
        
        // 最后一个事件的 cycles
        uint32_t sync_cycles = 0;
        if (event_count > 0) {
            const QstEvent* last_event = reinterpret_cast<const QstEvent*>(
                buffer_ + data_size - constants::EVENT_SIZE);
            sync_cycles = last_event->data[0];
        }
        
        // 设置输出参数
        if (out_size) *out_size = data_size;
        if (out_event_count) *out_event_count = event_count;
        if (out_sync_cycles) *out_sync_cycles = sync_cycles;
        
        return result;
    } else {
        // 已环绕：需要重组
        // 原始: [新数据 0..pos) [旧数据 pos..buffer_size)
        // 输出: [旧数据] [新数据] (按时间顺序)
        
        size_t data_size = buffer_size_;
        size_t old_data_size = buffer_size_ - pos;  // 旧数据 (较早)
        size_t new_data_size = pos;                  // 新数据 (较晚)
        size_t event_count = capacity_;
        
        // 直接分配 unique_ptr 管理的内存，一次复制 (分两段)
        auto result = std::make_unique<uint8_t[]>(data_size);
        
        // 先复制旧数据 (从 pos 到末尾)
        if (old_data_size > 0) {
            std::memcpy(result.get(), buffer_ + pos, old_data_size);
        }
        
        // 再复制新数据 (从开头到 pos)
        if (new_data_size > 0) {
            std::memcpy(result.get() + old_data_size, buffer_, new_data_size);
        }
        
        // 最后一个事件的 cycles
        uint32_t sync_cycles = 0;
        if (event_count > 0) {
            // 最后一个事件在新数据的末尾，即原始缓冲区的 pos - EVENT_SIZE
            size_t last_event_offset;
            if (pos > 0) {
                last_event_offset = pos - constants::EVENT_SIZE;
            } else {
                last_event_offset = buffer_size_ - constants::EVENT_SIZE;
            }
            const QstEvent* last_event = reinterpret_cast<const QstEvent*>(
                buffer_ + last_event_offset);
            sync_cycles = last_event->data[0];
        }
        
        // 设置输出参数
        if (out_size) *out_size = data_size;
        if (out_event_count) *out_event_count = event_count;
        if (out_sync_cycles) *out_sync_cycles = sync_cycles;
        
        return result;
    }
}

void DataBuffer::reset() {
    write_pos_ = 0;
    wrap_count_ = 0;
    wallclock_sec_ = 0;
    wallclock_nsec_ = 0;
    
    LOG_INFO("缓冲区已重置");
}

void DataBuffer::dumpState() const {
    std::fprintf(stderr, "\n");
    std::fprintf(stderr, "╔══════════════════════════════════════════════════╗\n");
    std::fprintf(stderr, "║              数据缓冲区状态                      ║\n");
    std::fprintf(stderr, "╠══════════════════════════════════════════════════╣\n");
    std::fprintf(stderr, "║ 缓冲区地址:    %-20p           ║\n", static_cast<void*>(buffer_));
    std::fprintf(stderr, "║ 缓冲区大小:    %-15zu 字节        ║\n", buffer_size_);
    std::fprintf(stderr, "║ 事件容量:      %-15zu 个          ║\n", capacity_);
    std::fprintf(stderr, "║ 写入位置:      %-15zu 字节        ║\n", static_cast<size_t>(write_pos_));
    std::fprintf(stderr, "║ 环绕次数:      %-15zu 次          ║\n", static_cast<size_t>(wrap_count_));
    std::fprintf(stderr, "║ 状态:          %-15d               ║\n", static_cast<int>(state_.load()));
    std::fprintf(stderr, "╠══════════════════════════════════════════════════╣\n");
    std::fprintf(stderr, "║ 时间同步点                                       ║\n");
    std::fprintf(stderr, "║ Wallclock:     %lld.%09lld                ║\n", 
            static_cast<long long>(wallclock_sec_), static_cast<long long>(wallclock_nsec_));
    std::fprintf(stderr, "║ Clock Freq:    %-20llu Hz        ║\n", 
            static_cast<unsigned long long>(clock_freq_));
    std::fprintf(stderr, "╚══════════════════════════════════════════════════╝\n");
    std::fprintf(stderr, "\n");
}

} // namespace qst

