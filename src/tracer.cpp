/**
 * @file tracer.cpp
 * @brief QSchedTracer - 追踪器主类实现
 * 
 * @details
 * 轻量级 QNX 调度事件追踪器，采用"飞行记录仪"模式。
 * 
 * @note 本文件仅支持 QNX Neutrino RTOS 平台
 * 
 * @author QSchedTracer Team
 * @date 2026-01-22
 */

#include "qst/tracer.hpp"
#include "qst/log.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <sys/trace.h>
#include <sys/mman.h>

namespace qst {

// 全局实例指针 (用于静态回调)
Tracer* Tracer::instance_ = nullptr;

// ============================================================================
// 构造和析构
// ============================================================================

Tracer::Tracer(const TracerConfig& config)
    : config_(config)
    , ring_buffer_(config.buffer_size)
    , active_buffer_(&ring_buffer_)  // 默认写入主缓冲区
{
    instance_ = this;
}

Tracer::~Tracer() {
    cleanupTrace();
    instance_ = nullptr;
}

// ============================================================================
// 静态辅助函数
// ============================================================================

int Tracer::getCpuCount() {
    if (_syspage_ptr) {
        return _syspage_ptr->num_cpu;
    }
    return static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
}

uint64_t Tracer::getClockFreq() {
    if (_syspage_ptr && SYSPAGE_ENTRY(qtime)) {
        return SYSPAGE_ENTRY(qtime)->cycles_per_sec;
    }
    return 1000000000ULL;
}

// ============================================================================
// 主运行函数
// ============================================================================

int Tracer::run() {
    // 打印 Banner
    LOG_INFO("╔══════════════════════════════════════════════════════╗");
    LOG_INFO("║   QSchedTracer - QNX 调度追踪器 (飞行记录仪模式)     ║");
    LOG_INFO("╠══════════════════════════════════════════════════════╣");
    LOG_INFO("║ 采集时长:    {} 秒 {}", config_.duration_sec, 
             config_.duration_sec == 0 ? "(无限)" : "");
    LOG_INFO("║ 输出文件:    {}", config_.output_file);
    LOG_INFO("║ 缓冲区:      {} MB", config_.buffer_size / (1024 * 1024));
    LOG_INFO("║ CPU 数量:    {}", getCpuCount());
    LOG_INFO("╚══════════════════════════════════════════════════════╝");
    
    // 配置内核 trace
    if (setupTrace() != 0) {
        LOG_ERROR("配置内核 trace 失败");
        return 1;
    }
    
    // 运行采集
    runCollection();
    
    // 打印统计
    printStats();
    ring_buffer_.dumpState();
    
    // 落盘前采集进程/线程信息
    LOG_INFO("准备落盘，采集进程/线程信息...");
    if (collectProcessInfo() != 0) {
        LOG_WARN("采集进程/线程信息失败，将使用未知名称");
    }
    
    // 保存数据
    int ret = saveToFile();
    
    // 清理
    cleanupTrace();
    
    LOG_INFO("程序结束 (退出码={})", ret);
    return ret;
}

void Tracer::requestStop() {
    stop_flag_.store(true);
}

// ============================================================================
// 中断回调
// ============================================================================

const struct sigevent* Tracer::bufferReadyHandler(int info) {
    if (instance_ == nullptr) {
        return nullptr;
    }
    
    // 提取 buffer 索引
    int idx = _TRACE_GET_BUFFNUM(info);
    
    if (idx < 0 || idx >= constants::KERNEL_BUFFER_COUNT || 
        instance_->kernel_buffers_ == nullptr) {
        return nullptr;
    }
    
    // 获取内核 buffer
    tracebuf_t* kbuf = &instance_->kernel_buffers_[idx];
    uint32_t num_events = kbuf->h.num_events;
    
    if (num_events == 0) {
        return nullptr;
    }
    
    // 限制事件数量到合理范围
    // 注意：不使用 _TRACELEMENTS 宏，因为它依赖嵌套类型 traceheader
    //       在 namespace 内部无法正确解析
    // _TRACEBUFSIZE = 16KB, 每个事件 16 bytes, 最多约 1000 个事件
    constexpr uint32_t MAX_TRACE_EVENTS = 1024;
    if (num_events > MAX_TRACE_EVENTS) {
        num_events = MAX_TRACE_EVENTS;
    }
    
    // 核心优化: 使用 memcpy 整块复制
    // 注意: 使用 active_buffer_ 支持动态切换写入目标
    size_t nbytes = num_events * sizeof(traceevent_t);
    instance_->active_buffer_->write(kbuf->data, nbytes);
    
    // 更新统计
    instance_->buffers_processed_++;
    
    // 发送 pulse 通知主线程
    instance_->pulse_event_.sigev_value.sival_int = idx;
    return &instance_->pulse_event_;
}

// ============================================================================
// 设置内核 trace
// ============================================================================

int Tracer::setupTrace() {
    int ret;
    
    LOG_INFO("正在配置内核 trace...");
    
    // 1. 获取 I/O 权限
    ret = ThreadCtl(_NTO_TCTL_IO, 0);
    if (ret == -1) {
        LOG_ERROR("获取 I/O 权限失败: {}", strerror(errno));
        return -1;
    }
    
    // 2. 清理现有资源
    TraceEvent(_NTO_TRACE_DEALLOCBUFFER);
    TraceEvent(_NTO_TRACE_STOP);
    usleep(10000);
    
    // 3. 分配内核 buffer
    LOG_DEBUG("分配 {} 个内核 buffer...", constants::KERNEL_BUFFER_COUNT);
    ret = TraceEvent(_NTO_TRACE_ALLOCBUFFER, constants::KERNEL_BUFFER_COUNT, &kernel_paddr_);
    if (ret == -1) {
        LOG_ERROR("分配内核 buffer 失败: {}", strerror(errno));
        return -1;
    }
    LOG_INFO("内核 buffer 物理地址: 0x{:x}", static_cast<unsigned long long>(kernel_paddr_));
    
    // 4. 映射物理内存
    size_t map_size = constants::KERNEL_BUFFER_COUNT * sizeof(tracebuf_t);
    kernel_buffers_ = static_cast<tracebuf_t*>(
        mmap(0, map_size, PROT_READ | PROT_WRITE,
             MAP_PHYS | MAP_SHARED, NOFD, kernel_paddr_));
    if (kernel_buffers_ == MAP_FAILED) {
        LOG_ERROR("内存映射失败: {}", strerror(errno));
        TraceEvent(_NTO_TRACE_DEALLOCBUFFER);
        return -1;
    }
    
    // 5. 创建消息通道
    channel_id_ = ChannelCreate(_NTO_CHF_PRIVATE);
    if (channel_id_ == -1) {
        LOG_ERROR("创建通道失败: {}", strerror(errno));
        munmap(kernel_buffers_, map_size);
        TraceEvent(_NTO_TRACE_DEALLOCBUFFER);
        return -1;
    }
    
    connection_id_ = ConnectAttach(0, 0, channel_id_, _NTO_SIDE_CHANNEL, 0);
    if (connection_id_ == -1) {
        LOG_ERROR("连接通道失败: {}", strerror(errno));
        ChannelDestroy(channel_id_);
        munmap(kernel_buffers_, map_size);
        TraceEvent(_NTO_TRACE_DEALLOCBUFFER);
        return -1;
    }
    
    // 6. 初始化 pulse
    SIGEV_PULSE_INIT(&pulse_event_, connection_id_, 15, constants::PULSE_CODE_BUFFER_READY, 0);
    
    // 7. 注册中断回调
    hook_id_ = InterruptHookTrace(bufferReadyHandler, 0);
    if (hook_id_ == -1) {
        LOG_ERROR("注册中断回调失败: {}", strerror(errno));
        ConnectDetach(connection_id_);
        ChannelDestroy(channel_id_);
        munmap(kernel_buffers_, map_size);
        TraceEvent(_NTO_TRACE_DEALLOCBUFFER);
        return -1;
    }
    LOG_INFO("InterruptHookTrace 注册成功 (id={})", hook_id_);
    
    // 8. 配置事件过滤器
    TraceEvent(_NTO_TRACE_DELALLCLASSES);
    TraceEvent(_NTO_TRACE_CLRCLASSPID, _NTO_TRACE_KERCALL);
    TraceEvent(_NTO_TRACE_CLRCLASSTID, _NTO_TRACE_KERCALL);
    TraceEvent(_NTO_TRACE_CLRCLASSPID, _NTO_TRACE_THREAD);
    TraceEvent(_NTO_TRACE_CLRCLASSTID, _NTO_TRACE_THREAD);
    
    TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_THREAD);
    TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_VTHREAD);
    TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_PROCESS);
    TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_CONTROL);
    
    // 9. 设置 FAST 模式
    TraceEvent(_NTO_TRACE_SETALLCLASSESFAST);
    
    // 10. 设置 Linear 模式 (buffer 满时通知)
    TraceEvent(_NTO_TRACE_SETLINEARMODE);
    
    LOG_INFO("内核 trace 配置完成");
    LOG_INFO("  事件类别: THREAD + VTHREAD + PROCESS + CONTROL");
    LOG_INFO("  Buffer 模式: Linear (InterruptHookTrace)");
    
    return 0;
}

// ============================================================================
// 清理内核 trace
// ============================================================================

void Tracer::cleanupTrace() {
    LOG_INFO("清理内核 trace 资源...");
    
    TraceEvent(_NTO_TRACE_STOP);
    
    if (hook_id_ != -1) {
        InterruptDetach(hook_id_);
        hook_id_ = -1;
    }
    
    if (connection_id_ != -1) {
        ConnectDetach(connection_id_);
        connection_id_ = -1;
    }
    
    if (channel_id_ != -1) {
        ChannelDestroy(channel_id_);
        channel_id_ = -1;
    }
    
    if (kernel_buffers_ != nullptr && kernel_buffers_ != MAP_FAILED) {
        munmap(kernel_buffers_, constants::KERNEL_BUFFER_COUNT * sizeof(tracebuf_t));
        kernel_buffers_ = nullptr;
    }
    
    TraceEvent(_NTO_TRACE_DEALLOCBUFFER);
}

// ============================================================================
// 运行采集
// ============================================================================

void Tracer::runCollection() {
    LOG_INFO("开始采集...");
    LOG_INFO("采集时长: {} 秒{}", config_.duration_sec, 
             config_.duration_sec == 0 ? " (无限，Ctrl+C 停止)" : "");
    
    // 设置时钟频率 (用于时间转换)
    ring_buffer_.setClockFreq(getClockFreq());
    LOG_INFO("时钟频率: {} Hz", ring_buffer_.clockFreq());
    
    ring_buffer_.setState(State::Running);
    
    // 使用 _NTO_TRACE_STARTNOSTATE 启动 trace
    int ret = TraceEvent(_NTO_TRACE_STARTNOSTATE);
    if (ret != 0) {
        LOG_WARN("启动 trace (NOSTATE) 返回: {}", ret);
    }
    LOG_INFO("使用 _NTO_TRACE_STARTNOSTATE 开始采集 (不注入初始状态)");
    
    time_t start_time = time(nullptr);
    int last_sec = -1;
    uint64_t last_events = 0;
    
    struct _pulse pulse;
    uint64_t timeout = constants::MSG_TIMEOUT_NS;
    
    while (!stop_flag_.load()) {
        int elapsed = static_cast<int>(time(nullptr) - start_time);
        
        // 检查超时
        if (config_.duration_sec > 0 && elapsed >= config_.duration_sec) {
            LOG_INFO("采集时长已到");
            break;
        }
        
        // 每秒打印进度
        if (elapsed > last_sec) {
            last_sec = elapsed;
            size_t current_events = ring_buffer_.eventCount();
            size_t delta = current_events - last_events;
            last_events = current_events;
            
            LOG_INFO("[进度] {}/{} 秒 | 事件: {} (+{}) | Buffer: {}", 
                    elapsed, config_.duration_sec, current_events, delta, 
                    static_cast<size_t>(buffers_processed_));
        }
        
        // 等待 pulse
        TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_RECEIVE, nullptr, &timeout, nullptr);
        int rcvid = MsgReceive(channel_id_, &pulse, sizeof(pulse), nullptr);
        
        if (rcvid == 0 && pulse.code == constants::PULSE_CODE_BUFFER_READY) {
            // 数据已在中断回调中处理
        }
    }
    
    // ========================================================================
    // 停止采集并记录真实时间同步点
    // ========================================================================
    
    // 1. 停止采集
    TraceEvent(_NTO_TRACE_STOP);
    
    // 2. **立即**获取系统时间 (这是与最后一个事件最接近的时间点)
    struct timespec wall_time;
    clock_gettime(CLOCK_REALTIME, &wall_time);
    
    // 3. 强制刷新内核 buffer (确保所有数据都到用户态)
    TraceEvent(_NTO_TRACE_FLUSHBUFFER);
    usleep(10000);
    
    // 4. 保存 wallclock 到 ring_buffer (sync_cycles 在 saveToFile 中计算)
    ring_buffer_.setWallclockSec(wall_time.tv_sec);
    ring_buffer_.setWallclockNsec(wall_time.tv_nsec);
    ring_buffer_.setEndTime(ClockCycles());  // 用于统计
    
    LOG_INFO("采集停止");
    LOG_INFO("真实时间同步点 (采集结束时): {}.{:09}", wall_time.tv_sec, wall_time.tv_nsec);
    LOG_INFO("环形缓冲区写入位置: {}", ring_buffer_.writePos());
    LOG_INFO("环绕次数: {}", ring_buffer_.wrapCount());
    
    ring_buffer_.setState(State::Idle);
}

// ============================================================================
// 采集进程/线程信息
// ============================================================================

int Tracer::collectProcessInfo() {
    LOG_INFO("采集进程/线程信息 (使用独立缓冲区)...");
    
    // 1. 分配进程/线程信息缓冲区
    procinfo_buffer_ = std::make_unique<uint8_t[]>(constants::PROCINFO_BUFFER_SIZE);
    procinfo_size_ = 0;
    procinfo_count_ = 0;
    
    // 2. 创建临时 RingBuffer 用于接收进程/线程信息
    RingBuffer temp_rb(constants::PROCINFO_BUFFER_SIZE);
    
    // 3. ★ 关键：切换 active_buffer_ 到临时缓冲区
    //    这样 bufferReadyHandler 会将数据写入 temp_rb 而不是 ring_buffer_
    RingBuffer* saved_buffer = active_buffer_;
    active_buffer_ = &temp_rb;
    
    // 4. 启动 trace (会注入 PROCDESTROY 事件)
    LOG_DEBUG("调用 _NTO_TRACE_START 注入进程/线程信息...");
    int ret = TraceEvent(_NTO_TRACE_START);
    if (ret != 0) {
        LOG_WARN("_NTO_TRACE_START 返回: {}", ret);
    }
    
    // 5. 短暂等待，让内核有时间注入事件
    usleep(50000);
    
    // 6. 停止 trace
    TraceEvent(_NTO_TRACE_STOP);
    
    // 7. Flush 内核缓冲区，确保所有数据都到达用户态
    TraceEvent(_NTO_TRACE_FLUSHBUFFER);
    usleep(10000);
    
    // 8. ★ 关键：恢复 active_buffer_ 到主缓冲区
    active_buffer_ = saved_buffer;
    
    // 9. 从临时缓冲区复制数据
    auto [temp_data, temp_size, temp_count] = temp_rb.getData();
    
    if (temp_size > 0 && temp_size <= constants::PROCINFO_BUFFER_SIZE) {
        std::memcpy(procinfo_buffer_.get(), temp_data, temp_size);
        procinfo_size_ = temp_size;
        procinfo_count_ = temp_count;
        LOG_INFO("采集到 {} 字节进程/线程信息 ({} 个事件)", 
                 procinfo_size_, procinfo_count_);
    } else if (temp_size > constants::PROCINFO_BUFFER_SIZE) {
        std::memcpy(procinfo_buffer_.get(), temp_data, constants::PROCINFO_BUFFER_SIZE);
        procinfo_size_ = constants::PROCINFO_BUFFER_SIZE;
        procinfo_count_ = constants::PROCINFO_BUFFER_SIZE / constants::EVENT_SIZE;
        LOG_WARN("进程/线程信息数据过大，截断到 {} 字节", procinfo_size_);
    } else {
        LOG_WARN("未采集到进程/线程信息 (temp_size={})", temp_size);
    }
    
    LOG_INFO("进程/线程信息采集完成 (主调度数据未受影响)");
    
    return 0;
}

// ============================================================================
// 保存数据到文件
// ============================================================================
//
// 文件格式 (v3 简化版):
// ┌──────────────────────────────────────────────────────────────┐
// │ FileHeader (64 字节)                                         │
// │   - magic: 'QST3'                                            │
// │   - clock_freq, wallclock_sec, wallclock_nsec, sync_cycles   │
// ├──────────────────────────────────────────────────────────────┤
// │ ProcInfoHeader (16 字节)                                     │
// │   - magic: 'PINF', event_count                               │
// ├──────────────────────────────────────────────────────────────┤
// │ ProcInfo 数据 (进程/线程名称)                                │
// ├──────────────────────────────────────────────────────────────┤
// │ MainDataHeader (16 字节)                                     │
// │   - magic: 'MAIN', event_count                               │
// ├──────────────────────────────────────────────────────────────┤
// │ 主调度数据 (按时间顺序写入)                                  │
// │   - 最后一个事件的 data[0] == sync_cycles                    │
// └──────────────────────────────────────────────────────────────┘
//
// ============================================================================

int Tracer::saveToFile() {
    LOG_INFO("保存数据到文件: {}", config_.output_file);
    
    ring_buffer_.setState(State::Dumping);
    
    int fd = open(config_.output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        LOG_ERROR("打开文件失败: {}", strerror(errno));
        return -1;
    }
    
    // 获取环形缓冲区信息
    uint8_t* buffer = ring_buffer_.buffer();
    size_t buffer_size = ring_buffer_.bufferSize();
    size_t write_pos = ring_buffer_.writePos();
    size_t wrap_count = ring_buffer_.wrapCount();
    
    // ========================================================================
    // 计算主调度数据的事件数量和最后一个事件的 sync_cycles
    // ========================================================================
    
    size_t main_event_count = 0;
    uint32_t sync_cycles = 0;
    
    if (wrap_count == 0) {
        // 未回绕：有效数据在 [0, write_pos)
        main_event_count = write_pos / constants::EVENT_SIZE;
        if (main_event_count > 0) {
            // 最后一个事件在 write_pos - EVENT_SIZE
            const QstEvent* last_event = reinterpret_cast<const QstEvent*>(
                buffer + write_pos - constants::EVENT_SIZE);
            sync_cycles = last_event->data[0];
        }
    } else {
        // 已回绕：缓冲区是满的
        main_event_count = buffer_size / constants::EVENT_SIZE;
        if (main_event_count > 0) {
            // 按时间顺序写入后，最后一个事件是 [0, write_pos) 中的最后一个
            // 即 write_pos - EVENT_SIZE 位置（如果 write_pos > 0）
            // 或 buffer_size - EVENT_SIZE 位置（如果 write_pos == 0）
            size_t last_event_offset;
            if (write_pos > 0) {
                last_event_offset = write_pos - constants::EVENT_SIZE;
            } else {
                last_event_offset = buffer_size - constants::EVENT_SIZE;
            }
            const QstEvent* last_event = reinterpret_cast<const QstEvent*>(
                buffer + last_event_offset);
            sync_cycles = last_event->data[0];
        }
    }
    
    LOG_INFO("主调度数据: {} 个事件, sync_cycles={}", main_event_count, sync_cycles);
    
    // ========================================================================
    // 1. 写入文件头 (FileHeader, 64 字节)
    // ========================================================================
    
    FileHeader header{};
    header.magic = constants::MAGIC;
    header.version = constants::VERSION;
    header.clock_freq = ring_buffer_.clockFreq();
    header.wallclock_sec = ring_buffer_.wallclockSec();
    header.wallclock_nsec = ring_buffer_.wallclockNsec();
    header.sync_cycles = sync_cycles;
    std::memset(header.reserved, 0, sizeof(header.reserved));
    
    if (::write(fd, &header, sizeof(header)) != sizeof(header)) {
        LOG_ERROR("写入文件头失败");
        close(fd);
        return -1;
    }
    LOG_DEBUG("写入文件头: {} 字节", sizeof(header));
    
    // ========================================================================
    // 2. 写入进程/线程信息 (ProcInfoHeader + 数据)
    // ========================================================================
    
    ProcInfoHeader procinfo_header{};
    procinfo_header.magic = constants::PROCINFO_MAGIC;
    procinfo_header.event_count = static_cast<uint32_t>(procinfo_count_);
    procinfo_header.reserved = 0;
    
    if (::write(fd, &procinfo_header, sizeof(procinfo_header)) != sizeof(procinfo_header)) {
        LOG_ERROR("写入进程/线程信息头失败");
        close(fd);
        return -1;
    }
    LOG_DEBUG("写入进程/线程信息头: {} 字节", sizeof(procinfo_header));
    
    if (procinfo_buffer_ && procinfo_size_ > 0) {
        ssize_t written = ::write(fd, procinfo_buffer_.get(), procinfo_size_);
        if (written != static_cast<ssize_t>(procinfo_size_)) {
            LOG_ERROR("写入进程/线程信息数据失败: 期望 {}, 实际 {}", 
                      procinfo_size_, written);
            close(fd);
            return -1;
        }
        LOG_DEBUG("写入进程/线程信息数据: {} 字节, {} 个事件", 
                  procinfo_size_, procinfo_count_);
    }
    
    // ========================================================================
    // 3. 写入主调度数据 (MainDataHeader + 数据，按时间顺序)
    // ========================================================================
    
    MainDataHeader maindata_header{};
    maindata_header.magic = constants::MAINDATA_MAGIC;
    maindata_header.event_count = static_cast<uint32_t>(main_event_count);
    maindata_header.reserved = 0;
    
    if (::write(fd, &maindata_header, sizeof(maindata_header)) != sizeof(maindata_header)) {
        LOG_ERROR("写入主调度数据头失败");
        close(fd);
        return -1;
    }
    LOG_DEBUG("写入主调度数据头: {} 字节", sizeof(maindata_header));
    
    size_t main_data_size = 0;
    
    if (wrap_count == 0) {
        // 未回绕：数据已经按顺序 [0, write_pos)
        if (write_pos > 0) {
            ssize_t written = ::write(fd, buffer, write_pos);
            if (written != static_cast<ssize_t>(write_pos)) {
                LOG_ERROR("写入主调度数据失败: 期望 {}, 实际 {}", write_pos, written);
                close(fd);
                return -1;
            }
            main_data_size = write_pos;
        }
    } else {
        // 已回绕：需要按时间顺序重组
        // 时间顺序: [write_pos, buffer_size) → [0, write_pos)
        
        size_t old_data_size = buffer_size - write_pos;  // 旧数据 (较早)
        size_t new_data_size = write_pos;                 // 新数据 (较晚)
        
        // 先写旧数据
        if (old_data_size > 0) {
            ssize_t written = ::write(fd, buffer + write_pos, old_data_size);
            if (written != static_cast<ssize_t>(old_data_size)) {
                LOG_ERROR("写入旧数据失败: 期望 {}, 实际 {}", old_data_size, written);
                close(fd);
                return -1;
            }
        }
        
        // 再写新数据
        if (new_data_size > 0) {
            ssize_t written = ::write(fd, buffer, new_data_size);
            if (written != static_cast<ssize_t>(new_data_size)) {
                LOG_ERROR("写入新数据失败: 期望 {}, 实际 {}", new_data_size, written);
                close(fd);
                return -1;
            }
        }
        
        main_data_size = old_data_size + new_data_size;
    }
    
    LOG_DEBUG("写入主调度数据: {} 字节, {} 个事件", main_data_size, main_event_count);
    
    close(fd);
    ring_buffer_.setState(State::Idle);
    
    // ========================================================================
    // 输出摘要
    // ========================================================================
    
    size_t total_size = sizeof(FileHeader) 
                      + sizeof(ProcInfoHeader) + procinfo_size_
                      + sizeof(MainDataHeader) + main_data_size;
    
    LOG_INFO("保存完成:");
    LOG_INFO("  文件头: {} 字节", sizeof(FileHeader));
    LOG_INFO("  进程/线程信息: {} + {} = {} 字节 ({} 个事件)", 
             sizeof(ProcInfoHeader), procinfo_size_, 
             sizeof(ProcInfoHeader) + procinfo_size_, procinfo_count_);
    LOG_INFO("  主调度数据: {} + {} = {} 字节 ({} 个事件)", 
             sizeof(MainDataHeader), main_data_size, 
             sizeof(MainDataHeader) + main_data_size, main_event_count);
    LOG_INFO("  文件总大小: {} 字节", total_size);
    LOG_INFO("  sync_cycles: {} (最后事件的 cycles)", sync_cycles);
    
    return 0;
}

// ============================================================================
// 统计打印
// ============================================================================

void Tracer::printStats() const {
    LOG_INFO("╔══════════════════════════════════════════╗");
    LOG_INFO("║              采集统计                    ║");
    LOG_INFO("╠══════════════════════════════════════════╣");
    LOG_INFO("║ 事件数量:      {} 个", ring_buffer_.eventCount());
    LOG_INFO("║ 处理 Buffer:   {} 个", static_cast<size_t>(buffers_processed_));
    LOG_INFO("║ 环绕次数:      {} 次", ring_buffer_.wrapCount());
    LOG_INFO("║ 总字节数:      {} 字节", static_cast<size_t>(ring_buffer_.totalBytes()));
    LOG_INFO("╚══════════════════════════════════════════╝");
}

} // namespace qst

