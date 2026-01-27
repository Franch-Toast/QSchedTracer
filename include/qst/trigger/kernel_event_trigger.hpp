/**
 * @file trigger/kernel_event_trigger.hpp
 * @brief QSchedTracer - 内核事件触发器
 * 
 * @details
 * 基于内核 trace 事件的触发器。
 * 当检测到匹配的事件时触发落盘。
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#pragma once

#include "trigger.hpp"
#include "qst/config/types.hpp"

namespace qst {
namespace trigger {

/**
 * @brief 内核事件触发器
 * 
 * 监听指定的内核事件，当事件数据满足条件时触发。
 * 支持通过 data[] 索引和比较操作符进行条件匹配。
 */
class KernelEventTrigger : public ITrigger {
public:
    /**
     * @brief 构造函数
     * @param config 触发器配置
     */
    explicit KernelEventTrigger(const config::TriggerConfig& config);
    
    /**
     * @brief 检查是否触发
     * @param ctx 触发器上下文
     * @return 触发结果
     */
    TriggerResult check(const TriggerContext& ctx) override;
    
    /**
     * @brief 获取触发器名称
     * @return 触发器名称
     */
    std::string name() const override;
    
    /**
     * @brief 获取触发器类型
     * @return TriggerType::KernelEvent
     */
    config::TriggerType type() const override {
        return config::TriggerType::KernelEvent;
    }
    
    // === Getters ===
    
    int eventClass() const { return event_class_; }
    int eventId() const { return event_id_; }
    const config::TriggerCondition& condition() const { return condition_; }
    const std::string& comment() const { return comment_; }

private:
    /**
     * @brief 检查条件是否满足
     * @param ctx 触发器上下文
     * @return true 条件满足, false 条件不满足
     */
    bool checkCondition(const TriggerContext& ctx) const;

private:
    int event_class_;                   ///< 目标事件类 ID
    int event_id_;                      ///< 目标事件 ID
    config::TriggerCondition condition_; ///< 触发条件
    std::string comment_;               ///< 注释
};

} // namespace trigger
} // namespace qst

