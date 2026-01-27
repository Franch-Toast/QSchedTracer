/**
 * @file event/event_manager.cpp
 * @brief QSchedTracer - 事件管理器实现
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#include "qst/event/event_manager.hpp"
#include "qst/log.hpp"

#include <sys/trace.h>
#include <sys/neutrino.h>

namespace qst {
namespace event {

EventManager::~EventManager() {
    if (initialized_) {
        cleanup();
    }
}

int EventManager::setup(const config::TracerConfig& config) {
    LOG_INFO("设置事件过滤器...");
    
    // 1. 清空所有事件类
    if (TraceEvent(_NTO_TRACE_DELALLCLASSES) == -1) {
        LOG_ERROR("清空事件类失败");
        return -1;
    }
    
    // 2. 清除 PID/TID 过滤
    TraceEvent(_NTO_TRACE_CLRCLASSPID, _NTO_TRACE_KERCALL);
    TraceEvent(_NTO_TRACE_CLRCLASSTID, _NTO_TRACE_KERCALL);
    TraceEvent(_NTO_TRACE_CLRCLASSPID, _NTO_TRACE_KERCALLENTER);
    TraceEvent(_NTO_TRACE_CLRCLASSTID, _NTO_TRACE_KERCALLENTER);
    
    // 3. 设置调度事件 (核心功能)
    if (config.scheduling.enabled) {
        if (setupSchedulingEvents(config.scheduling) != 0) {
            LOG_ERROR("设置调度事件失败");
            return -1;
        }
    }
    
    // 4. 设置扩展事件类
    for (const auto& ec : config.extra_events.classes) {
        if (addClass(ec) != 0) {
            LOG_WARN("添加事件类 {} 失败", ec.class_id);
        }
    }
    
    // 5. 设置特定事件
    for (const auto& se : config.extra_events.specific_events) {
        if (addSpecificEvent(se) != 0) {
            LOG_WARN("添加特定事件 class={}, event={} 失败", 
                    se.class_id, se.event_id);
        }
    }
    
    initialized_ = true;
    LOG_INFO("事件过滤器设置完成");
    return 0;
}

void EventManager::cleanup() {
    LOG_INFO("清理事件过滤器...");
    
    // 删除已注册的事件处理器
    for (const auto& [cls, evt] : registered_handlers_) {
        TraceEvent(_NTO_TRACE_DELEVENTHANDLER, cls, evt);
    }
    registered_handlers_.clear();
    
    // 清空所有事件类
    TraceEvent(_NTO_TRACE_DELALLCLASSES);
    
    initialized_ = false;
    LOG_INFO("事件过滤器已清理");
}

int EventManager::setEventHandler(int event_class, int event_id,
                                  int (*handler)(event_data_t*),
                                  event_data_t* data) {
    if (TraceEvent(_NTO_TRACE_ADDEVENTHANDLER, event_class, event_id, 
                   handler, data) == -1) {
        LOG_ERROR("注册事件处理器失败: class={}, event={}", event_class, event_id);
        return -1;
    }
    
    registered_handlers_.emplace_back(event_class, event_id);
    LOG_DEBUG("注册事件处理器: class={}, event={}", event_class, event_id);
    return 0;
}

int EventManager::setupSchedulingEvents(const config::SchedulingConfig& config) {
    LOG_INFO("设置调度事件: mode={}", 
             config::eventModeToString(config.mode));
    
    // 设置模式
    if (config.mode == config::EventMode::Wide) {
        TraceEvent(_NTO_TRACE_SETALLCLASSESWIDE);
    } else {
        TraceEvent(_NTO_TRACE_SETALLCLASSESFAST);
    }
    
    // 添加核心调度事件类
    // _NTO_TRACE_THREAD (10)
    if (TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_THREAD) == -1) {
        LOG_ERROR("添加 THREAD 类失败");
        return -1;
    }
    
    // 添加 THREAD RUNNING 事件
    if (TraceEvent(_NTO_TRACE_ADDEVENT, _NTO_TRACE_THREAD, 
                   _NTO_TRACE_THRUNNING) == -1) {
        LOG_ERROR("添加 THRUNNING 事件失败");
        return -1;
    }
    
    // _NTO_TRACE_VTHREAD (11)
    if (TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_VTHREAD) == -1) {
        LOG_ERROR("添加 VTHREAD 类失败");
        return -1;
    }
    
    // _NTO_TRACE_PROCESS (9)
    if (TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_PROCESS) == -1) {
        LOG_ERROR("添加 PROCESS 类失败");
        return -1;
    }
    
    // _NTO_TRACE_CONTROL (1) - 时间同步等
    if (TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_CONTROL) == -1) {
        LOG_ERROR("添加 CONTROL 类失败");
        return -1;
    }
    
    LOG_INFO("调度事件设置完成");
    return 0;
}

int EventManager::addClass(const config::EventClassConfig& config) {
    LOG_DEBUG("添加事件类: class={}, mode={}, comment={}", 
              config.class_id, 
              config::eventModeToString(config.mode),
              config.comment);
    
    // 设置模式
    if (config.mode == config::EventMode::Wide) {
        if (TraceEvent(_NTO_TRACE_SETCLASSWIDE, config.class_id) == -1) {
            LOG_ERROR("设置事件类 {} Wide 模式失败", config.class_id);
            return -1;
        }
    } else {
        if (TraceEvent(_NTO_TRACE_SETCLASSFAST, config.class_id) == -1) {
            LOG_ERROR("设置事件类 {} Fast 模式失败", config.class_id);
            return -1;
        }
    }
    
    // 添加事件类
    if (TraceEvent(_NTO_TRACE_ADDCLASS, config.class_id) == -1) {
        LOG_ERROR("添加事件类 {} 失败", config.class_id);
        return -1;
    }
    
    return 0;
}

int EventManager::addSpecificEvent(const config::SpecificEventConfig& config) {
    LOG_DEBUG("添加特定事件: class={}, event={}, mode={}, comment={}",
              config.class_id, config.event_id,
              config::eventModeToString(config.mode),
              config.comment);
    
    // 设置特定事件的模式 (使用 SETEVENTWIDE/SETEVENTFAST，而不是 SETCLASSWIDE)
    if (config.mode == config::EventMode::Wide) {
        if (TraceEvent(_NTO_TRACE_SETEVENTWIDE, config.class_id, config.event_id) == -1) {
            LOG_WARN("设置事件 class={}, event={} Wide 模式失败", 
                    config.class_id, config.event_id);
        } else {
            LOG_DEBUG("设置事件 class={}, event={} 为 Wide 模式", 
                     config.class_id, config.event_id);
        }
    } else {
        if (TraceEvent(_NTO_TRACE_SETEVENTFAST, config.class_id, config.event_id) == -1) {
            LOG_WARN("设置事件 class={}, event={} Fast 模式失败",
                    config.class_id, config.event_id);
        }
    }
    
    // 添加事件
    if (TraceEvent(_NTO_TRACE_ADDEVENT, config.class_id, config.event_id) == -1) {
        LOG_ERROR("添加事件 class={}, event={} 失败", 
                 config.class_id, config.event_id);
        return -1;
    }
    
    LOG_INFO("已添加特定事件: class={}, event={}, mode={} ({})",
             config.class_id, config.event_id,
             config::eventModeToString(config.mode),
             config.comment);
    
    return 0;
}

} // namespace event
} // namespace qst

