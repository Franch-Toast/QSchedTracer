/**
 * @file trigger/kernel_event_trigger.cpp
 * @brief QSchedTracer - 内核事件触发器实现
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#include "qst/trigger/kernel_event_trigger.hpp"
#include "qst/log.hpp"

namespace qst {
namespace trigger {

KernelEventTrigger::KernelEventTrigger(const config::TriggerConfig& config)
    : event_class_(config.event_class)
    , event_id_(config.event_id)
    , condition_(config.condition)
{
    LOG_DEBUG("创建内核事件触发器: class={}, event={}",
              event_class_, event_id_);
}

std::string KernelEventTrigger::name() const {
    return "KernelEvent[class=" + std::to_string(event_class_) + 
           ",event=" + std::to_string(event_id_) + "]";
}

TriggerResult KernelEventTrigger::check(const TriggerContext& ctx) {
    // 注意：此函数可能在中断上下文中被调用，不能使用 LOG_DEBUG 等非安全函数
    
    // 1. 检查事件类型
    if (ctx.event_class != event_class_ || ctx.event_id != event_id_) {
        return TriggerResult::None;
    }
    
    // 2. 检查条件
    if (!checkCondition(ctx)) {
        return TriggerResult::None;
    }
    
    // 3. 触发 - 注意：不能在这里打印日志，触发信息会在主循环的 handleTrigger 中打印
    return TriggerResult::SaveAndContinue;
}

bool KernelEventTrigger::checkCondition(const TriggerContext& ctx) const {
    // 注意：此函数可能在中断上下文中被调用，不能使用非安全函数
    
    // 无条件，匹配所有
    if (!condition_.has_condition) {
        return true;
    }
    
    // 获取数据值
    int64_t data_value = ctx.getData(condition_.data_index);
    
    // 比较
    switch (condition_.op) {
        case config::CompareOp::Eq:
            return (data_value == condition_.value);
        case config::CompareOp::Ne:
            return (data_value != condition_.value);
        case config::CompareOp::Gt:
            return (data_value > condition_.value);
        case config::CompareOp::Lt:
            return (data_value < condition_.value);
        case config::CompareOp::Ge:
            return (data_value >= condition_.value);
        case config::CompareOp::Le:
            return (data_value <= condition_.value);
    }
    
    return false;
}

} // namespace trigger
} // namespace qst

