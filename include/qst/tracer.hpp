/**
 * @file tracer.hpp
 * @brief QSchedTracer - 追踪器主类
 * 
 * @details
 * 轻量级 QNX 调度事件追踪器，采用"飞行记录仪"模式。
 * 
 * ## 核心设计
 * 
 * 1. **InterruptHookTrace + Linear Mode**: 
 *    使用 Linear 模式，buffer 满时触发中断回调，实时获取数据。
 * 
 * 2. **单一环形缓冲区**: 
 *    不按 CPU 分离，简化实现，中断回调中直接 memcpy 整个内核 buffer。
 * 
 * 3. **飞行记录仪模式**: 
 *    持续记录到内存 Ring Buffer，仅在程序退出/信号触发时落盘。
 * 
 * ## 工作流程
 * 
 * ```
 *                          InterruptHookTrace
 *                                 │
 *     内核 trace buffer ──────────┼───────▶ 用户态 Ring Buffer
 *     (tracebuf_t, Linear模式)    │         (memcpy 整块复制)
 *                                 │
 *                                 ▼
 *                          发送 pulse 通知
 *                                 │
 *                                 ▼
 *                          主线程 MsgReceive
 *                          (更新统计，检查停止条件)
 *                                 │
 *                                 ▼
 *                          退出时落盘 → .qst 文件
 * ```
 * 
 * @note 本文件仅支持 QNX Neutrino RTOS 平台
 * 
 * @author QSchedTracer Team
 * @date 2026-01-21
 */

#pragma once

#include "types.hpp"
#include "ring_buffer.hpp"
#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <csignal>

#include <sys/neutrino.h>
#include <sys/trace.h>
#include <sys/kercalls.h>

namespace qst {

/**
 * @brief 追踪器配置
 */
struct TracerConfig {
    int duration_sec = 0;                           ///< 采集时长 (秒, 0=无限，仅 Ctrl+C 退出)
    std::string output_file = "trace.qst";          ///< 输出文件
    size_t buffer_size = constants::DEFAULT_BUFFER_SIZE;  ///< 缓冲区大小
    bool enable_signal_trigger = true;              ///< 启用 SIGKILL 触发落盘 (检测到 SIGKILL 时自动落盘并继续采集)
};

/**
 * @brief QNX 调度追踪器类
 * 
 * 使用飞行记录仪模式采集 QNX 调度事件。
 */
class Tracer {
public:
    /**
     * @brief 构造函数
     * @param config 追踪器配置
     */
    explicit Tracer(const TracerConfig& config = {});
    
    /**
     * @brief 析构函数
     */
    ~Tracer();
    
    // 禁止拷贝和移动
    Tracer(const Tracer&) = delete;
    Tracer& operator=(const Tracer&) = delete;
    Tracer(Tracer&&) = delete;
    Tracer& operator=(Tracer&&) = delete;
    
    /**
     * @brief 运行追踪器
     * @return 0 成功, 非0 失败
     */
    int run();
    
    /**
     * @brief 请求停止
     */
    void requestStop();
    
    /**
     * @brief 获取环形缓冲区
     */
    RingBuffer& ringBuffer() { return ring_buffer_; }
    const RingBuffer& ringBuffer() const { return ring_buffer_; }
    
    /**
     * @brief 获取配置
     */
    const TracerConfig& config() const { return config_; }
    
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
     * @brief 设置内核 trace
     * @return 0 成功, -1 失败
     * 
     * 设置调度追踪 (Fast mode) 和可选的信号追踪 (Wide mode for SIGKILL)
     */
    int setupTrace();
    
    /**
     * @brief 清理内核 trace 资源
     */
    void cleanupTrace();
    
    /**
     * @brief 运行采集循环
     * 
     * 持续采集调度事件，直到：
     * - 达到指定时长 (duration_sec > 0)
     * - 用户按 Ctrl+C
     * - 检测到 SIGKILL (触发落盘后继续采集)
     */
    void runCollection();
    
    /**
     * @brief 采集进程/线程信息 (落盘前)
     * @return 0 成功, -1 失败
     */
    int collectProcessInfo();
    
    /**
     * @brief 保存数据到文件
     * @return 0 成功, -1 失败
     */
    int saveToFile();
    
    /**
     * @brief 打印统计信息
     */
    void printStats() const;

    /**
     * @brief 中断回调函数
     */
    static const struct sigevent* bufferReadyHandler(int info);
    
    /**
     * @brief SignalKill 事件处理器 (Wide mode)
     * @note 运行在内核/中断上下文，只能调用中断安全函数！
     */
    static int signalKillEventHandler(event_data_t* event_data);

private:
    TracerConfig config_;
    RingBuffer ring_buffer_;
    
    // 停止标志
    std::atomic<bool> stop_flag_{false};
    
    // 进程/线程信息缓冲区
    std::unique_ptr<uint8_t[]> procinfo_buffer_;
    size_t procinfo_size_{0};
    size_t procinfo_count_{0};
    
    // 统计
    volatile uint64_t buffers_processed_{0};
    
    // 内核 trace 相关
    paddr_t kernel_paddr_{0};
    tracebuf_t* kernel_buffers_{nullptr};
    int hook_id_{-1};
    int channel_id_{-1};
    int connection_id_{-1};
    struct sigevent pulse_event_;

    // 全局实例指针 (用于静态回调)
    static Tracer* instance_;
    
    // 活动缓冲区指针 (用于切换写入目标)
    // - 正常采集时指向 ring_buffer_
    // - 采集进程信息时临时切换到其他缓冲区
    RingBuffer* active_buffer_{nullptr};
    
    // ========== 信号追踪相关 ==========
    
    // 信号事件数据结构 (用于事件处理器)
    event_data_t signal_event_data_;
    uint32_t signal_data_array_[10];  // Wide mode: nd, pid, tid, signo, code, value
    
    // 信号检测标志 (volatile，在中断处理器中设置)
    static volatile sig_atomic_t signal_detected_;
    static volatile int last_signal_target_pid_;
    static volatile int last_signal_signo_;
    
};

} // namespace qst

