/**
 * @file core/tracer_engine.hpp
 * @brief QSchedTracer - 追踪引擎主类
 * 
 * @details
 * 轻量级 QNX 调度事件追踪器，采用"飞行记录仪"模式。
 * 
 * ## 核心设计
 * 
 * 1. **InterruptHookTrace + Linear Mode**: 
 *    使用 Linear 模式，buffer 满时触发中断回调，实时获取数据。
 * 
 * 2. **单一数据缓冲区**: 
 *    不按 CPU 分离，简化实现，中断回调中直接 memcpy 整个内核 buffer。
 * 
 * 3. **飞行记录仪模式**: 
 *    持续记录到内存 DataBuffer，仅在触发条件满足/Ctrl+C 时落盘。
 * 
 * 4. **配置驱动**: 
 *    通过 JSON 配置文件控制事件过滤和触发条件。
 * 
 * @note 本文件仅支持 QNX Neutrino RTOS 平台
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#pragma once

#include "qst/config/types.hpp"
#include "qst/core/data_buffer.hpp"
#include "qst/event/event_manager.hpp"
#include "qst/trigger/trigger_manager.hpp"
#include "qst/data/data_manager.hpp"

#include <string>
#include <memory>
#include <atomic>
#include <csignal>

#include <sys/neutrino.h>
#include <sys/trace.h>

namespace qst {
namespace core {

/**
 * @brief QNX 调度追踪引擎
 * 
 * 使用飞行记录仪模式采集 QNX 调度事件。
 * 通过配置控制事件过滤和落盘触发。
 */
class TracerEngine {
public:
    /**
     * @brief 构造函数
     * @param config 追踪器配置
     */
    explicit TracerEngine(const config::TracerConfig& config);
    
    /**
     * @brief 析构函数
     */
    ~TracerEngine();
    
    // 禁止拷贝和移动
    TracerEngine(const TracerEngine&) = delete;
    TracerEngine& operator=(const TracerEngine&) = delete;
    TracerEngine(TracerEngine&&) = delete;
    TracerEngine& operator=(TracerEngine&&) = delete;
    
    /**
     * @brief 运行追踪引擎
     * @return 0 成功, 非0 失败
     */
    int run();
    
    /**
     * @brief 请求停止
     */
    void requestStop();
    
    /**
     * @brief 获取数据缓冲区
     */
    DataBuffer& dataBuffer() { return data_buffer_; }
    const DataBuffer& dataBuffer() const { return data_buffer_; }
    
    /**
     * @brief 获取配置
     */
    const config::TracerConfig& config() const { return config_; }
    
    /**
     * @brief 获取 CPU 数量
     */
    static int getCpuCount();
    
    /**
     * @brief 获取时钟频率
     */
    static uint64_t getClockFreq();

private:
    /**
     * @brief 初始化
     * @return 0 成功, -1 失败
     */
    int initialize();
    
    /**
     * @brief 设置内核 trace
     * @return 0 成功, -1 失败
     */
    int setupKernelTrace();
    
    /**
     * @brief 清理内核 trace 资源
     */
    void cleanup();
    
    /**
     * @brief 运行采集循环
     */
    void runLoop();
    
    /**
     * @brief 处理触发落盘
     */
    void handleTrigger();
    
    /**
     * @brief 打印统计信息
     */
    void printStats() const;

    /**
     * @brief 中断回调函数
     */
    static const struct sigevent* bufferReadyHandler(int info);
    
    /**
     * @brief 事件触发处理器 (用于触发器检查)
     */
    static int eventTriggerHandler(event_data_t* event_data);

private:
    config::TracerConfig config_;           ///< 配置
    DataBuffer data_buffer_;                ///< 主数据缓冲区
    
    event::EventManager event_manager_;     ///< 事件管理器
    trigger::TriggerManager trigger_manager_; ///< 触发器管理器
    std::unique_ptr<data::DataManager> data_manager_; ///< 数据管理器
    
    
    // 触发标志 (在中断上下文设置)
    static volatile sig_atomic_t trigger_flag_;
    
    // 统计
    volatile uint64_t buffers_processed_{0};
    
    // 内核 trace 相关
    paddr_t kernel_paddr_{0};
    tracebuf_t* kernel_buffers_{nullptr};
    int hook_id_{-1};
    int channel_id_{-1};
    int connection_id_{-1};
    
    // Pulse 事件 (用于中断到用户空间通信)
    struct sigevent buffer_pulse_event_;    ///< Buffer Ready pulse
    struct sigevent trigger_pulse_event_;   ///< Trigger pulse (触发落盘)
    
    // 事件数据结构 (用于事件处理器)
    event_data_t event_data_;
    uint32_t event_data_array_[10];  // Wide mode 数据

    // 全局实例指针 (用于静态回调)
    static TracerEngine* instance_;
};

// 全局活动缓冲区指针 (用于中断回调写入)
extern DataBuffer* g_active_buffer;

} // namespace core
} // namespace qst

