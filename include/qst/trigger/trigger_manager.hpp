/**
 * @file trigger/trigger_manager.hpp
 * @brief QSchedTracer - 触发器管理器
 * 
 * @details
 * 管理多个触发器，检查触发条件。
 * 负责触发器相关的内核事件注册和事件处理器设置。
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#pragma once

#include "trigger.hpp"
#include "qst/config/types.hpp"
#include <sys/trace.h>
#include <vector>
#include <memory>
#include <string>
#include <utility>

namespace qst {
namespace trigger {

/**
 * @brief 触发器管理器
 * 
 * 统一管理多个触发器，遍历检查触发条件。
 * 职责：
 * - 管理触发器对象
 * - 设置触发器相关的内核事件 (通过 setupKernelEvents)
 * - 检查触发条件
 */
class TriggerManager {
public:
    /**
     * @brief 默认构造函数
     */
    TriggerManager() = default;
    
    /**
     * @brief 析构函数
     */
    ~TriggerManager();
    
    // 禁止拷贝
    TriggerManager(const TriggerManager&) = delete;
    TriggerManager& operator=(const TriggerManager&) = delete;
    
    /**
     * @brief 根据配置初始化触发器
     * @param configs 触发器配置列表
     */
    void initialize(const std::vector<config::TriggerConfig>& configs);
    
    /**
     * @brief 设置触发器相关的内核事件处理器
     * 
     * 从 specific_events 获取事件列表（使用外部 class ID），为每个事件注册处理器。
     * 
     * 注意：specific_events 使用外部 class ID（用于 TraceEvent API），
     *       triggers 使用内部 class ID（用于事件处理器中的匹配）。
     * 
     * @param specific_events 特定事件配置列表（包含外部 class ID）
     * @param handler 事件处理器回调函数
     * @param event_data 传递给处理器的数据
     * @return 0 成功, -1 失败
     */
    int setupKernelEvents(const std::vector<config::SpecificEventConfig>& specific_events,
                          int (*handler)(event_data_t*), 
                          event_data_t* event_data);
    
    /**
     * @brief 清理触发器相关的内核事件
     */
    void cleanupKernelEvents();
    
    /**
     * @brief 添加触发器
     * @param trigger 触发器指针
     */
    void addTrigger(std::unique_ptr<ITrigger> trigger);
    
    /**
     * @brief 检查所有触发器
     * @param ctx 触发器上下文
     * @return 触发结果 (任意一个触发器命中则返回 SaveAndContinue)
     */
    TriggerResult checkAll(const TriggerContext& ctx);
    
    /**
     * @brief 获取最后触发的触发器名称
     * @return 触发器名称 (在主循环中调用，不在中断上下文)
     */
    std::string lastTriggeredName() const { 
        if (last_triggered_index_ >= 0 && 
            static_cast<size_t>(last_triggered_index_) < triggers_.size()) {
            return triggers_[last_triggered_index_]->name();
        }
        return "unknown";
    }
    
    /**
     * @brief 获取触发器数量
     * @return 触发器数量
     */
    size_t count() const { return triggers_.size(); }
    
    /**
     * @brief 清空所有触发器
     */
    void clear();

private:
    std::vector<std::unique_ptr<ITrigger>> triggers_;  ///< 触发器列表
    volatile int last_triggered_index_{-1};            ///< 最后触发的触发器索引 (中断安全)
    
    /// 已注册的事件处理器 (class, event) 对
    std::vector<std::pair<int, int>> registered_handlers_;
};

} // namespace trigger
} // namespace qst

