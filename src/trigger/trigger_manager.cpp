/**
 * @file trigger/trigger_manager.cpp
 * @brief QSchedTracer - 触发器管理器实现
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#include "qst/trigger/trigger_manager.hpp"
#include "qst/trigger/kernel_event_trigger.hpp"
#include "qst/log.hpp"

#include <sys/trace.h>

namespace qst {
namespace trigger {

TriggerManager::~TriggerManager() {
    cleanupKernelEvents();
}

void TriggerManager::initialize(const std::vector<config::TriggerConfig>& configs) {
    triggers_.clear();
    registered_handlers_.clear();
    
    for (const auto& config : configs) {
        switch (config.type) {
            case config::TriggerType::KernelEvent:
                triggers_.push_back(
                    std::make_unique<KernelEventTrigger>(config)
                );
                break;
                
            case config::TriggerType::Topic:
                LOG_WARN("Topic 触发器尚未实现，跳过");
                break;
                
            case config::TriggerType::Timeout:
                LOG_WARN("Timeout 触发器尚未实现，跳过");
                break;
        }
    }
    
    LOG_INFO("初始化触发器管理器: {} 个触发器", triggers_.size());
}

int TriggerManager::setupKernelEvents(const std::vector<config::SpecificEventConfig>& specific_events,
                                       int (*handler)(event_data_t*), 
                                       event_data_t* event_data) {
    LOG_INFO("注册触发器事件处理器...");
    
    // 从 specific_events 获取事件列表（使用外部 class ID）
    // TraceEvent API 需要外部 class ID
    for (const auto& event_config : specific_events) {
        int event_class = event_config.class_id;  // 外部 class ID
        int event_id = event_config.event_id;
        
        // 注册事件处理器
        if (TraceEvent(_NTO_TRACE_ADDEVENTHANDLER, event_class, event_id, 
                       handler, event_data) == -1) {
            LOG_WARN("注册事件处理器 class={}, event={} 失败 (外部 class)",
                    event_class, event_id);
        } else {
            registered_handlers_.emplace_back(event_class, event_id);
            LOG_INFO("注册触发器事件处理器: class={}, event={} (外部 class)",
                    event_class, event_id);
        }
    }
    
    LOG_INFO("触发器事件处理器注册完成: {} 个处理器", registered_handlers_.size());
    return 0;
}

void TriggerManager::cleanupKernelEvents() {
    for (const auto& [cls, evt] : registered_handlers_) {
        TraceEvent(_NTO_TRACE_DELEVENTHANDLER, cls, evt);
        LOG_DEBUG("删除触发器事件处理器: class={}, event={}", cls, evt);
    }
    registered_handlers_.clear();
}

void TriggerManager::addTrigger(std::unique_ptr<ITrigger> trigger) {
    if (trigger) {
        LOG_DEBUG("添加触发器: {}", trigger->name());
        triggers_.push_back(std::move(trigger));
    }
}

void TriggerManager::clear() {
    cleanupKernelEvents();
    triggers_.clear();
}

TriggerResult TriggerManager::checkAll(const TriggerContext& ctx) {
    // 注意：此函数可能在中断上下文中被调用，不能使用非安全函数
    // 不能调用 trigger->name() 因为它会创建 std::string (涉及内存分配)
    for (size_t i = 0; i < triggers_.size(); ++i) {
        TriggerResult result = triggers_[i]->check(ctx);
        if (result != TriggerResult::None) {
            last_triggered_index_ = static_cast<int>(i);
            return result;
        }
    }
    return TriggerResult::None;
}

} // namespace trigger
} // namespace qst

