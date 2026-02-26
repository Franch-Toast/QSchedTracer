/**
 * @file event/event_manager.cpp
 * @brief QSchedTracer - 事件管理器实现
 * 
 * 采集策略：基于方案 C（详尽采集），针对调度问题分析优化
 * 
 * 采集的事件类别：
 * 1. 线程状态事件（全部，Wide mode）- 调度延迟、阻塞分析、优先级反转检测
 * 2. VThread 状态事件（全部，Wide mode）- 虚拟线程调度
 * 3. 进程事件 - 进程/线程名称
 * 4. 中断事件（INTENTER/INTEXIT）- 中断延迟分析
 * 5. 通信事件 - 轻量级 IPC 追踪
 * 6. 系统事件（IPI/PAGEWAIT）- 跨核调度、内存性能
 * 7. KerCall 事件 - 同步原语和消息传递详细信息
 * 
 * @author QSchedTracer Team
 * @date 2026-01-28
 */

#include "qst/event/event_manager.hpp"
#include "qst/log.hpp"

#include <sys/trace.h>
#include <sys/neutrino.h>
#include <sys/kercalls.h>

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
    TraceEvent(_NTO_TRACE_CLRCLASSPID, _NTO_TRACE_KERCALLEXIT);
    TraceEvent(_NTO_TRACE_CLRCLASSTID, _NTO_TRACE_KERCALLEXIT);
    
    // 3. 设置默认的调度事件（全部 Wide mode）
    if (setupDefaultEvents() != 0) {
        LOG_ERROR("设置默认事件失败");
        return -1;
    }
    
    // 4. 设置扩展事件类（用户可通过配置添加额外事件）
    for (const auto& ec : config.extra_events.classes) {
        if (addClass(ec) != 0) {
            LOG_WARN("添加事件类 {} 失败", ec.class_id);
        }
    }
    
    // 5. 设置特定事件（用户可通过配置添加额外事件）
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

/**
 * @brief 设置默认采集事件（全部 Wide mode）
 * 
 * 根据 TRACE_EVENT_SELECTION_GUIDE.md 最终决定：
 * 
 * 【线程状态事件】- 全类 Wide
 * 【虚拟线程状态事件】- 全类 Wide
 * 【进程事件】- 全类
 * 【中断事件】- 暂不采集
 * 【通信事件】- 全类 Wide
 * 【系统事件】- 暂不采集（IPI/PAGEWAIT）
 * 【控制事件】- 仅 CONTROLBUFFER（调试用）
 * 【KerCall 事件】- 同步原语 + IPC + 调度
 */
int EventManager::setupDefaultEvents() {
    LOG_INFO("配置默认事件（调度分析专用，全部 Wide mode）...");
    
    // ========================================
    // 1. 线程状态事件 (_NTO_TRACE_THREAD) - 全类 Wide
    // ========================================
    LOG_INFO("  [1/8] 线程状态事件（全类 Wide mode）...");
    if (TraceEvent(_NTO_TRACE_SETCLASSWIDE, _NTO_TRACE_THREAD) == -1) {
        LOG_WARN("设置 THREAD 类 Wide 模式失败");
    }
    if (TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_THREAD) == -1) {
        LOG_ERROR("添加 THREAD 类失败");
        return -1;
    }
    LOG_INFO("    已添加: RUNNING, READY, MUTEX, SEM, CONDVAR, SEND, RECEIVE, REPLY, NANOSLEEP, INTR, etc.");
    
    // ========================================
    // 2. 虚拟线程状态事件 (_NTO_TRACE_VTHREAD) - 全类 Wide
    // ========================================
    LOG_INFO("  [2/8] 虚拟线程状态事件（全类 Wide mode）...");
    if (TraceEvent(_NTO_TRACE_SETCLASSWIDE, _NTO_TRACE_VTHREAD) == -1) {
        LOG_WARN("设置 VTHREAD 类 Wide 模式失败");
    }
    if (TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_VTHREAD) == -1) {
        LOG_ERROR("添加 VTHREAD 类失败");
        return -1;
    }
    
    // ========================================
    // 3. 进程事件 (_NTO_TRACE_PROCESS) - 全类
    // ========================================
    LOG_INFO("  [3/8] 进程事件（全类）...");
    if (TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_PROCESS) == -1) {
        LOG_ERROR("添加 PROCESS 类失败");
        return -1;
    }
    LOG_INFO("    已添加: PROCCREATE_NAME, PROCDESTROY_NAME, PROCTHREAD_NAME");
    
    // ========================================
    // 4. 中断事件 - 暂不采集
    // ========================================
    // _NTO_TRACE_INTENTER / _NTO_TRACE_INTEXIT 暂时不考虑
    LOG_INFO("  [4/8] 中断事件 - 跳过（暂不采集）");
    
    // ========================================
    // 5. 通信事件 (_NTO_TRACE_COMM) - 全类 Wide
    // ========================================
    LOG_INFO("  [5/8] 通信事件（全类 Wide mode）...");
    if (TraceEvent(_NTO_TRACE_SETCLASSWIDE, _NTO_TRACE_COMM) == -1) {
        LOG_WARN("设置 COMM 类 Wide 模式失败");
    }
    if (TraceEvent(_NTO_TRACE_ADDCLASS, _NTO_TRACE_COMM) == -1) {
        LOG_ERROR("添加 COMM 类失败");
        return -1;
    }
    LOG_INFO("    已添加: SMSG, RMSG, REPLY, ERROR, SPULSE, RPULSE, SIGNAL, etc.");
    
    // ========================================
    // 6. 系统事件 - 暂不采集
    // ========================================
    // _NTO_TRACE_SYS_IPI / _NTO_TRACE_SYS_PAGEWAIT 暂时不考虑
    LOG_INFO("  [6/8] 系统事件 - 跳过（暂不采集）");
    
    // ========================================
    // 7. 控制事件 - 仅 CONTROLBUFFER
    // ========================================
    LOG_INFO("  [7/8] 控制事件（仅 CONTROLBUFFER）...");
    // 只添加 CONTROLBUFFER 用于判断是否有漏抓
    if (TraceEvent(_NTO_TRACE_ADDEVENT, _NTO_TRACE_CONTROL, _NTO_TRACE_CONTROLBUFFER) == -1) {
        LOG_WARN("添加 CONTROLBUFFER 事件失败");
    } else {
        LOG_INFO("    已添加: CONTROLBUFFER（用于调试检测漏抓）");
    }
    
    // ========================================
    // 8. KerCall 事件（同步原语 + IPC + 调度）
    // ========================================
    LOG_INFO("  [8/8] KerCall 事件（同步原语 + IPC + 调度）...");
    
    // --- Mutex: 仅 LOCK ENTER ---
    setupKerCallEnterOnly(__KER_SYNC_MUTEX_LOCK, "SYNC_MUTEX_LOCK");
    setupKerCallEnterOnly(__KER_SYNC_MUTEX_UNLOCK, "SYNC_MUTEX_UNLOCK");
    
    // --- Semaphore: 仅 ENTER ---
    setupKerCallEnterOnly(__KER_SYNC_SEM_WAIT, "SYNC_SEM_WAIT");
    setupKerCallEnterOnly(__KER_SYNC_SEM_POST, "SYNC_SEM_POST");
    
    // --- Condvar: 仅 ENTER ---
    setupKerCallEnterOnly(__KER_SYNC_CONDVAR_WAIT, "SYNC_CONDVAR_WAIT");
    setupKerCallEnterOnly(__KER_SYNC_CONDVAR_SIGNAL, "SYNC_CONDVAR_SIGNAL");
    
    // --- IPC: ENTER + EXIT ---
    setupKerCallEvent(__KER_MSG_SENDV, "MSG_SENDV");
    setupKerCallEvent(__KER_MSG_RECEIVEV, "MSG_RECEIVEV");
    setupKerCallEvent(__KER_MSG_REPLYV, "MSG_REPLYV");
    
    // --- 调度: 仅 YIELD ENTER ---
    setupKerCallEnterOnly(__KER_SCHED_YIELD, "SCHED_YIELD");
    // __KER_SCHED_SET 不采集
    
    LOG_INFO("默认事件配置完成");
    return 0;
}

/**
 * @brief 设置单个 KerCall 事件（ENTER + EXIT, Wide mode）
 */
void EventManager::setupKerCallEvent(int kercall_id, const char* name) {
    // 设置为 Wide 模式
    if (TraceEvent(_NTO_TRACE_SETEVENTWIDE, _NTO_TRACE_KERCALLENTER, kercall_id) == -1) {
        LOG_WARN("设置 {} ENTER Wide 模式失败", name);
    }
    if (TraceEvent(_NTO_TRACE_SETEVENTWIDE, _NTO_TRACE_KERCALLEXIT, kercall_id) == -1) {
        LOG_WARN("设置 {} EXIT Wide 模式失败", name);
    }
    
    // 添加 ENTER 事件
    if (TraceEvent(_NTO_TRACE_ADDEVENT, _NTO_TRACE_KERCALLENTER, kercall_id) == -1) {
        LOG_WARN("添加 {} ENTER 事件失败", name);
    }
    
    // 添加 EXIT 事件
    if (TraceEvent(_NTO_TRACE_ADDEVENT, _NTO_TRACE_KERCALLEXIT, kercall_id) == -1) {
        LOG_WARN("添加 {} EXIT 事件失败", name);
    }
    
    LOG_DEBUG("    已添加 KerCall: {} (ENTER + EXIT, Wide mode)", name);
}

/**
 * @brief 设置单个 KerCall 事件（仅 ENTER, Wide mode）
 */
void EventManager::setupKerCallEnterOnly(int kercall_id, const char* name) {
    // 设置为 Wide 模式
    if (TraceEvent(_NTO_TRACE_SETEVENTWIDE, _NTO_TRACE_KERCALLENTER, kercall_id) == -1) {
        LOG_WARN("设置 {} ENTER Wide 模式失败", name);
    }
    
    // 添加 ENTER 事件
    if (TraceEvent(_NTO_TRACE_ADDEVENT, _NTO_TRACE_KERCALLENTER, kercall_id) == -1) {
        LOG_WARN("添加 {} ENTER 事件失败", name);
    }
    
    LOG_DEBUG("    已添加 KerCall: {} (ENTER only, Wide mode)", name);
}

int EventManager::addClass(const config::EventClassConfig& config) {
    LOG_DEBUG("添加事件类: class={}, mode={}", 
              config.class_id, 
              config::eventModeToString(config.mode));
    
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
    LOG_DEBUG("添加特定事件: class={}, event={}, mode={}",
              config.class_id, config.event_id,
              config::eventModeToString(config.mode));
    
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
    
    LOG_INFO("已添加特定事件: class={}, event={}, mode={}",
             config.class_id, config.event_id,
             config::eventModeToString(config.mode));
    
    return 0;
}

} // namespace event
} // namespace qst

