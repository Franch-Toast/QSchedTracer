/**
 * @file event/event_manager.hpp
 * @brief QSchedTracer - 事件管理器
 * 
 * @details
 * 根据配置设置 QNX 内核事件过滤器。
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#pragma once

#include "qst/config/types.hpp"
#include <sys/trace.h>
#include <vector>
#include <utility>

namespace qst {
namespace event {

/**
 * @brief 事件管理器
 * 
 * 负责根据配置设置 QNX Trace API 的事件过滤器。
 */
class EventManager {
public:
    /**
     * @brief 默认构造函数
     */
    EventManager() = default;
    
    /**
     * @brief 析构函数
     */
    ~EventManager();
    
    // 禁止拷贝
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;
    
    /**
     * @brief 根据配置设置内核事件过滤器
     * @param config 完整配置
     * @return 0 成功, -1 失败
     */
    int setup(const config::TracerConfig& config);
    
    /**
     * @brief 清理事件过滤器
     */
    void cleanup();
    
    /**
     * @brief 设置事件处理器
     * @param event_class 事件类 ID
     * @param event_id 事件 ID
     * @param handler 处理器函数
     * @param data 处理器数据
     * @return 0 成功, -1 失败
     */
    int setEventHandler(int event_class, int event_id,
                       int (*handler)(event_data_t*),
                       event_data_t* data);

private:
    /**
     * @brief 设置默认的采集事件（全部 Wide mode）
     * 
     * 采集策略（参见 TRACE_EVENT_SELECTION_GUIDE.md）：
     * - 线程状态事件（全类 Wide）
     * - VThread 状态事件（全类 Wide）
     * - 进程事件（全类）
     * - 通信事件（全类 Wide）
     * - 控制事件（仅 CONTROLBUFFER）
     * - KerCall 事件（同步原语 + IPC + 调度）
     * 
     * @return 0 成功, -1 失败
     */
    int setupDefaultEvents();
    
    /**
     * @brief 设置单个 KerCall 事件（ENTER + EXIT，Wide mode）
     * @param kercall_id KerCall ID
     * @param name 事件名称（用于日志）
     */
    void setupKerCallEvent(int kercall_id, const char* name);
    
    /**
     * @brief 设置单个 KerCall 事件（仅 ENTER，Wide mode）
     * @param kercall_id KerCall ID
     * @param name 事件名称（用于日志）
     */
    void setupKerCallEnterOnly(int kercall_id, const char* name);
    
    /**
     * @brief 添加事件类
     * @param config 事件类配置
     * @return 0 成功, -1 失败
     */
    int addClass(const config::EventClassConfig& config);
    
    /**
     * @brief 添加特定事件
     * @param config 特定事件配置
     * @return 0 成功, -1 失败
     */
    int addSpecificEvent(const config::SpecificEventConfig& config);

private:
    /// 已注册的事件处理器 (class, event) 用于清理
    std::vector<std::pair<int, int>> registered_handlers_;
    
    /// 是否已初始化
    bool initialized_{false};
};

} // namespace event
} // namespace qst

