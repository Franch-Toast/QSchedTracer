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
     * @brief 设置调度事件 (核心功能)
     * @param config 调度配置
     * @return 0 成功, -1 失败
     */
    int setupSchedulingEvents(const config::SchedulingConfig& config);
    
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

