/**
 * @file core/tracer_engine.cpp
 * @brief QSchedTracer - 追踪引擎实现
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#include "qst/core/tracer_engine.hpp"
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
namespace core {

// 全局实例指针
TracerEngine* TracerEngine::instance_ = nullptr;

// 触发标志
volatile sig_atomic_t TracerEngine::trigger_flag_ = 0;

// ============================================================================
// 构造和析构
// ============================================================================

TracerEngine::TracerEngine(const config::TracerConfig& config)
    : config_(config)
    , data_buffer_(config.buffer_size_mb * 1024 * 1024)
{
    instance_ = this;
    
    // 初始化事件数据结构
    std::memset(&event_data_, 0, sizeof(event_data_));
    std::memset(event_data_array_, 0, sizeof(event_data_array_));
    event_data_.data_array = event_data_array_;
}

TracerEngine::~TracerEngine() {
    cleanup();
    instance_ = nullptr;
}

// ============================================================================
// 静态辅助函数
// ============================================================================

int TracerEngine::getCpuCount() {
    if (_syspage_ptr) {
        return _syspage_ptr->num_cpu;
    }
    return static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
}

uint64_t TracerEngine::getClockFreq() {
    if (_syspage_ptr && SYSPAGE_ENTRY(qtime)) {
        return SYSPAGE_ENTRY(qtime)->cycles_per_sec;
    }
    return 1000000000ULL;
}

// ============================================================================
// 主运行函数
// ============================================================================

int TracerEngine::run() {
    // 打印 Banner
    LOG_INFO("╔══════════════════════════════════════════════════════╗");
    LOG_INFO("║   QSchedTracer - QNX 调度追踪 (飞行记录仪模式)       ║");
    LOG_INFO("╠══════════════════════════════════════════════════════╣");
    LOG_INFO("║ 缓冲区:      {} MB", config_.buffer_size_mb);
    LOG_INFO("║ CPU 数量:    {}", getCpuCount());
    LOG_INFO("║ 触发器数:    {}", config_.triggers.size());
    LOG_INFO("╚══════════════════════════════════════════════════════╝");
    
    // 初始化
    if (initialize() != 0) {
        LOG_ERROR("初始化失败");
        return 1;
    }
    
    // 配置内核 trace
    if (setupKernelTrace() != 0) {
        LOG_ERROR("配置内核 trace 失败");
        return 1;
    }
    
    // 运行采集循环
    runLoop();
    
    // 打印统计
    printStats();
    data_buffer_.dumpState();
    
    // 落盘前采集进程/线程信息
    LOG_INFO("准备落盘，采集进程/线程信息...");
    collectProcessInfo();
    
    // 保存数据
    int ret = data_manager_->save();
    
    // 清理
    cleanup();
    
    LOG_INFO("程序结束 (退出码={})", ret);
    return ret;
}

void TracerEngine::requestStop() {
    // 通过发送 PULSE_CODE_STOP 来通知主循环停止
    // 这比设置标志更可靠，因为主循环可能正在阻塞等待
    if (connection_id_ != -1) {
        MsgSendPulse(connection_id_, 
                     SIGEV_PULSE_PRIO_INHERIT,
                     constants::PULSE_CODE_STOP, 
                     constants::TRIGGER_SOURCE_SIGNAL);
    }
}

// ============================================================================
// 初始化
// ============================================================================

int TracerEngine::initialize() {
    LOG_INFO("初始化追踪引擎...");
    
    // 创建数据管理器
    data_manager_ = std::make_unique<data::DataManager>(data_buffer_);
    
    // 初始化触发器管理器
    trigger_manager_.initialize(config_.triggers);
    
    // 设置时钟频率
    data_buffer_.setClockFreq(getClockFreq());
    LOG_INFO("时钟频率: {} Hz", data_buffer_.clockFreq());
    
    return 0;
}

// ============================================================================
// 中断回调
// ============================================================================

const struct sigevent* TracerEngine::bufferReadyHandler(int info) {
    if (instance_ == nullptr) {
        return nullptr;
    }
    
    // 提取 buffer 索引
    int idx = _TRACE_GET_BUFFNUM(info);
    
    if (idx < 0 || idx >= constants::KERNEL_BUFFER_COUNT) {
        return nullptr;
    }
    
    // 更新统计
    instance_->buffers_processed_++;
    
    // 设置 pulse 的 value 为 buffer index
    instance_->buffer_pulse_event_.sigev_value.sival_int = idx;
    
    // 检查触发标志
    if (trigger_flag_) {
        trigger_flag_ = 0;
        instance_->pending_trigger_ = true;
    }
    
    // 发送 pulse 通知用户态线程处理
    return &instance_->buffer_pulse_event_;
}

// ============================================================================
// 事件触发处理器
// ============================================================================

int TracerEngine::eventTriggerHandler(event_data_t* event_data) {
    if (instance_ == nullptr) {
        return 1;  // 继续记录
    }
    
    // 注意：此函数在中断上下文中执行，不能调用非安全函数（如 printf、malloc、LOG_*）
    //
    // 从 header 提取内部 class ID 和 event ID
    // QNX trace header 位布局:
    //   bit 0-9:   event ID (_NTO_TRACE_GETEVENT)
    //   bit 10-14: internal class ID (_NTO_TRACE_GETEVENT_C >> 10)
    //   bit 24-29: CPU ID
    //   bit 30-31: struct type
    //
    // 配置文件 triggers 中使用内部 class ID，与此处匹配
    // 内部 class 映射:
    //   2 = _TRACE_KER_CALL_C (对应 KERCALL/KERCALLENTER/KERCALLEXIT/KERCALLINT)
    //
    // data_array 布局 (Wide mode, __KER_SIGNAL_KILL):
    //   data_array[0] = nd
    //   data_array[1] = pid
    //   data_array[2] = tid
    //   data_array[3] = signo  <- 触发条件检查此值
    //   data_array[4] = code
    //   data_array[5] = value
    
    trigger::TriggerContext ctx;
    // 从 header 提取内部 class 和 event (使用 QNX 宏)
    ctx.event_class = _NTO_TRACE_GETEVENT_C(event_data->header) >> 10;  // 内部 class ID
    ctx.event_id = _NTO_TRACE_GETEVENT(event_data->header);             // event ID
    ctx.data_array = event_data->data_array;
    ctx.data_count = event_data->el_num;
    
    // 检查触发器 (触发器检查函数必须是中断安全的)
    trigger::TriggerResult result = instance_->trigger_manager_.checkAll(ctx);
    
    if (result == trigger::TriggerResult::SaveAndContinue) {
        // 设置触发标志，bufferReadyHandler 会检查此标志并发送触发 pulse
        trigger_flag_ = 1;
    }
    
    return 1;  // 继续记录事件
}

// ============================================================================
// 设置内核 trace
// ============================================================================

int TracerEngine::setupKernelTrace() {
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
    LOG_DEBUG("内核 buffer 物理地址: 0x{:x}", kernel_paddr_);
    
    // 4. 映射内核 buffer 到用户空间
    size_t total_size = constants::KERNEL_BUFFER_COUNT * sizeof(tracebuf_t);
    kernel_buffers_ = static_cast<tracebuf_t*>(
        mmap(nullptr, total_size, PROT_READ, 
             MAP_SHARED | MAP_PHYS, NOFD, kernel_paddr_));
    
    if (kernel_buffers_ == MAP_FAILED) {
        LOG_ERROR("映射内核 buffer 失败: {}", strerror(errno));
        TraceEvent(_NTO_TRACE_DEALLOCBUFFER);
        return -1;
    }
    LOG_DEBUG("映射成功: {} 字节 @ {}", total_size, static_cast<void*>(kernel_buffers_));
    
    // 5. 创建 channel 和 connection 用于 pulse
    // 设置固定优先级，避免被其他高优先级线程抢占，这点必须设置，会影响线程的优先级设置
    channel_id_ = ChannelCreate(0 | _NTO_CHF_FIXED_PRIORITY);
    if (channel_id_ == -1) {
        LOG_ERROR("创建 channel 失败: {}", strerror(errno));
        return -1;
    }
    
    connection_id_ = ConnectAttach(0, 0, channel_id_, _NTO_SIDE_CHANNEL, 0);
    if (connection_id_ == -1) {
        LOG_ERROR("连接 channel 失败: {}", strerror(errno));
        return -1;
    }
    
    // 6. 设置 pulse events
    // Buffer Ready pulse - 通知主线程数据已复制
    SIGEV_PULSE_INIT(&buffer_pulse_event_, connection_id_, 
                     SIGEV_PULSE_PRIO_INHERIT, 
                     constants::PULSE_CODE_BUFFER_READY, 0);
    
    // Trigger pulse - 通知主线程需要落盘
    SIGEV_PULSE_INIT(&trigger_pulse_event_, connection_id_, 
                     SIGEV_PULSE_PRIO_INHERIT, 
                     constants::PULSE_CODE_TRIGGER, 
                     constants::TRIGGER_SOURCE_KERNEL_EVENT);
    
    // 7. 注册中断回调
    hook_id_ = InterruptHookTrace(bufferReadyHandler, 0);
    if (hook_id_ == -1) {
        LOG_ERROR("注册 InterruptHookTrace 失败: {}", strerror(errno));
        return -1;
    }
    LOG_DEBUG("InterruptHookTrace 注册成功: hook_id={}", hook_id_);
    
    // 8. 设置 Linear Mode (buffer 满时触发中断)
    ret = TraceEvent(_NTO_TRACE_SETLINEARMODE);
    if (ret == -1) {
        LOG_ERROR("设置 Linear Mode 失败: {}", strerror(errno));
        return -1;
    }
    
    // 9. 使用事件管理器设置事件过滤器
    if (event_manager_.setup(config_) != 0) {
        LOG_ERROR("设置事件过滤器失败");
        return -1;
    }
    
    // 10. 使用触发器管理器设置触发器相关的内核事件处理器
    // 从 extra_events.specific_events 获取事件列表（使用外部 class ID）
    // triggers 中的 class 是内部 class ID（用于 eventTriggerHandler 中的匹配）
    event_data_.data_array = event_data_array_;
    if (trigger_manager_.setupKernelEvents(config_.extra_events.specific_events,
                                           eventTriggerHandler, &event_data_) != 0) {
        LOG_WARN("设置触发器内核事件处理器失败");
    }
    
    LOG_INFO("内核 trace 配置完成");
    return 0;
}

// ============================================================================
// 清理
// ============================================================================

void TracerEngine::cleanup() {
    LOG_INFO("清理资源...");
    
    // 停止 trace
    TraceEvent(_NTO_TRACE_STOP);
    
    // 清理事件管理器
    event_manager_.cleanup();
    
    // 清理中断回调
    if (hook_id_ != -1) {
        InterruptHookTrace(nullptr, 0);
        hook_id_ = -1;
    }
    
    // 清理 channel
    if (connection_id_ != -1) {
        ConnectDetach(connection_id_);
        connection_id_ = -1;
    }
    if (channel_id_ != -1) {
        ChannelDestroy(channel_id_);
        channel_id_ = -1;
    }
    
    // 取消映射
    if (kernel_buffers_ != nullptr && kernel_buffers_ != MAP_FAILED) {
        size_t total_size = constants::KERNEL_BUFFER_COUNT * sizeof(tracebuf_t);
        munmap(kernel_buffers_, total_size);
        kernel_buffers_ = nullptr;
    }
    
    // 释放内核 buffer
    TraceEvent(_NTO_TRACE_DEALLOCBUFFER);
    
    LOG_INFO("资源清理完成");
}

// ============================================================================
// 采集循环
// ============================================================================

// ============================================================================
// Buffer 数据复制辅助方法
// ============================================================================

size_t TracerEngine::copyBufferData(int idx, DataBuffer& target) {
    if (idx < 0 || idx >= constants::KERNEL_BUFFER_COUNT || kernel_buffers_ == nullptr) {
        return 0;
    }
    
    tracebuf_t* kbuf = &kernel_buffers_[idx];
    uint32_t num_events = kbuf->h.num_events;
    
    if (num_events == 0 || num_events > 1024) {
        return 0;
    }
    
    size_t nbytes = num_events * sizeof(traceevent_t);
    target.write(kbuf->data, nbytes);
    return num_events;
}

size_t TracerEngine::drainPendingPulses(DataBuffer& target, size_t max_count) {
    struct _pulse pulse;
    size_t count = 0;
    
    while (max_count == 0 || count < max_count) {
        // 设置超时，避免无限阻塞
        struct sigevent timeout_event;
        SIGEV_UNBLOCK_INIT(&timeout_event);
        TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_RECEIVE, &timeout_event, nullptr, nullptr);
        
        int rcvid = MsgReceive(channel_id_, &pulse, sizeof(pulse), nullptr);
        if (rcvid != 0) {
            break;  // 超时或错误
        }
        
        if (pulse.code == constants::PULSE_CODE_BUFFER_READY) {
            copyBufferData(pulse.value.sival_int, target);
            count++;
        }
    }
    
    return count;
}

void TracerEngine::runLoop() {
    LOG_INFO("开始采集...");
    LOG_INFO("等待触发信号或停止请求...");
    
    // 启动 trace
    TraceEvent(_NTO_TRACE_STARTNOSTATE);
    data_buffer_.setState(State::Running);
    
    struct _pulse pulse;
    bool running = true;
    
    while (running) {
        int rcvid = MsgReceive(channel_id_, &pulse, sizeof(pulse), nullptr);
        
        if (rcvid == 0) {
            switch (pulse.code) {
                case constants::PULSE_CODE_BUFFER_READY:
                    // 收到 buffer ready pulse，复制数据到主缓冲区
                    copyBufferData(pulse.value.sival_int, data_buffer_);
                    
                    // 检查是否有挂起的触发
                    if (pending_trigger_) {
                        pending_trigger_ = false;
                        handleTrigger();
                    }
                    break;
                    
                case constants::PULSE_CODE_STOP:
                    LOG_INFO("收到停止 pulse");
                    running = false;
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    // 停止采集
    TraceEvent(_NTO_TRACE_STOP);
    
    // 获取系统时间
    struct timespec wall_time;
    clock_gettime(CLOCK_REALTIME, &wall_time);
    
    // 刷新缓冲区并处理剩余 pulse
    TraceEvent(_NTO_TRACE_FLUSHBUFFER);
    drainPendingPulses(data_buffer_);
    
    // 保存时间戳
    data_buffer_.setWallclockSec(wall_time.tv_sec);
    data_buffer_.setWallclockNsec(wall_time.tv_nsec);
    
    LOG_INFO("采集停止");
    LOG_INFO("真实时间同步点: {}.{:09}", wall_time.tv_sec, wall_time.tv_nsec);
    
    data_buffer_.setState(State::Idle);
}

// ============================================================================
// 处理触发落盘
// ============================================================================

void TracerEngine::handleTrigger() {
    LOG_INFO("触发落盘: {}", trigger_manager_.lastTriggeredName());
    
    // 1. 停止 trace
    TraceEvent(_NTO_TRACE_STOP);
    
    // 2. 获取时间戳
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    
    // 3. 刷新缓冲区并处理剩余 pulse
    TraceEvent(_NTO_TRACE_FLUSHBUFFER);
    drainPendingPulses(data_buffer_);
    
    // 4. 保存时间戳
    data_buffer_.setWallclockSec(ts.tv_sec);
    data_buffer_.setWallclockNsec(ts.tv_nsec);
    
    // 5. 采集进程信息并保存
    collectProcessInfo();
    data_manager_->save();
    
    LOG_INFO("保存到: {}", data_manager_->lastFilename());
    
    // 6. 重置缓冲区
    data_buffer_.reset();
    
    // 7. 重新开始
    TraceEvent(_NTO_TRACE_STARTNOSTATE);
    data_buffer_.setState(State::Running);
    
    LOG_INFO("继续采集...");
}

// ============================================================================
// 采集进程/线程信息
// ============================================================================

void TracerEngine::collectProcessInfo() {
    LOG_INFO("采集进程/线程信息...");
    
    // 创建临时缓冲区 预留3MB空间，应该完全足够了
    DataBuffer temp_buffer(constants::PROCINFO_BUFFER_SIZE);
    
    // 启动 trace (会注入进程/线程信息)
    // _NTO_TRACE_START 会触发内核注入当前所有进程/线程的状态信息
    int ret = TraceEvent(_NTO_TRACE_START);
    if (ret != 0) {
        LOG_WARN("_NTO_TRACE_START 返回: {}", ret);
    }
    
    // 短暂等待，让内核有时间注入事件
    // 注意：等待时间过长会导致环形缓冲区被其他事件覆盖
    // 内核注入进程信息通常在 1-2ms 内完成
    usleep(2000);  // 2ms
    
    // 立即停止 trace，避免更多事件进入缓冲区
    TraceEvent(_NTO_TRACE_STOP);
    
    // 不刷新，直接接收数据
    // TraceEvent(_NTO_TRACE_FLUSHBUFFER);
    
    // 接收所有待处理的 buffer pulse
    // 设置为 0 表示接收所有待处理的 pulse（直到超时）
    // 这确保不会遗漏任何进程/线程信息
    size_t pulse_count = drainPendingPulses(temp_buffer, 0);
    
    LOG_DEBUG("接收了 {} 个 buffer pulse", pulse_count);
    
    // 传递给 DataManager
    data_manager_->setProcInfoBuffer(std::move(temp_buffer));
}

// ============================================================================
// 统计信息
// ============================================================================

void TracerEngine::printStats() const {
    size_t usage_percent = data_buffer_.bufferSize() > 0 
        ? (data_buffer_.writePos() * 100) / data_buffer_.bufferSize() 
        : 0;
    
    LOG_INFO("╔══════════════════════════════════════════════════════╗");
    LOG_INFO("║                     采集统计                         ║");
    LOG_INFO("╠══════════════════════════════════════════════════════╣");
    LOG_INFO("║ 中断触发次数:  {}", static_cast<uint64_t>(buffers_processed_));
    LOG_INFO("║ 缓冲区使用:    {}%", usage_percent);
    LOG_INFO("║ 环绕次数:      {}", data_buffer_.wrapCount());
    LOG_INFO("╚══════════════════════════════════════════════════════╝");
}

} // namespace core
} // namespace qst

