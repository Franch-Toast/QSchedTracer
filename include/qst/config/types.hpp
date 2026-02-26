/**
 * @file config/types.hpp
 * @brief QSchedTracer - 配置数据结构定义
 * 
 * @details
 * 定义 JSON 配置文件对应的 C++ 数据结构。
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace qst {
namespace config {

// ============================================================================
// 枚举类型
// ============================================================================

/**
 * @brief 事件捕获模式
 */
enum class EventMode {
    Fast,   ///< Fast mode - 最小数据量，高性能
    Wide    ///< Wide mode - 完整数据，性能较低
};

/**
 * @brief 比较操作符
 */
enum class CompareOp {
    Eq,     ///< ==
    Ne,     ///< !=
    Gt,     ///< >
    Lt,     ///< <
    Ge,     ///< >=
    Le      ///< <=
};

/**
 * @brief 触发器类型
 */
enum class TriggerType {
    KernelEvent,    ///< 内核事件触发
    Topic,          ///< Topic 消息触发 (未来)
    Timeout         ///< 定时触发 (未来)
};

// ============================================================================
// 配置数据结构
// ============================================================================

/**
 * @brief 调度配置 (核心功能)
 * 
 * 控制默认调度事件追踪：
 * - THREAD (class 10)
 * - VTHREAD (class 11)
 * - PROCESS (class 9)
 * - CONTROL (class 1)
 */
struct SchedulingConfig {
    bool enabled = true;            ///< 是否启用调度追踪
    EventMode mode = EventMode::Fast;  ///< 事件模式
};

/**
 * @brief 事件类配置 (扩展)
 */
struct EventClassConfig {
    int class_id = 0;               ///< QNX 事件类 ID
    EventMode mode = EventMode::Fast;  ///< 事件模式
};

/**
 * @brief 特定事件配置 (扩展)
 */
struct SpecificEventConfig {
    int class_id = 0;               ///< 事件类 ID
    int event_id = 0;               ///< 事件 ID
    EventMode mode = EventMode::Wide;  ///< 事件模式
};

/**
 * @brief 扩展事件配置
 */
struct ExtraEventsConfig {
    std::vector<EventClassConfig> classes;          ///< 添加整个事件类
    std::vector<SpecificEventConfig> specific_events;  ///< 添加特定事件
};

/**
 * @brief 触发条件 (基于 traceevent_t.data[] 索引)
 */
struct TriggerCondition {
    int data_index = 0;             ///< event_data_t.data_array[] 索引 (0-5 for Wide mode)
    CompareOp op = CompareOp::Eq;   ///< 比较操作符
    int64_t value = 0;              ///< 比较值
    bool has_condition = false;     ///< 是否有条件 (无条件则匹配所有)
};

/**
 * @brief 触发器配置
 * 
 * 注意：触发器只负责检查条件和决定何时触发落盘。
 * 事件的采集模式 (Wide/Fast) 应该在 extra_events.specific_events 中配置。
 */
struct TriggerConfig {
    TriggerType type = TriggerType::KernelEvent;  ///< 触发器类型
    
    // kernel_event 类型参数
    int event_class = 0;            ///< 事件类 ID
    int event_id = 0;               ///< 事件 ID
    TriggerCondition condition;     ///< 触发条件
    
    // topic 类型参数 (未来)
    std::string topic_name;         ///< Topic 名称
    
    // timeout 类型参数 (未来)
    int interval_sec = 0;           ///< 定时间隔 (秒)
};

/**
 * @brief 完整配置结构
 */
struct TracerConfig {
    std::string version = "1.0";    ///< 配置版本
    
    // 基础配置
    size_t buffer_size_mb = 10;     ///< 缓冲区大小 (MB)
    
    // 调度配置 (核心功能，硬编码事件类)
    SchedulingConfig scheduling;
    
    // 扩展事件配置 (高级功能)
    ExtraEventsConfig extra_events;
    
    // 触发器配置
    std::vector<TriggerConfig> triggers;
};

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 将 EventMode 转换为字符串
 */
inline const char* eventModeToString(EventMode mode) {
    return mode == EventMode::Wide ? "wide" : "fast";
}

/**
 * @brief 将字符串转换为 EventMode
 */
inline EventMode stringToEventMode(const std::string& str) {
    return str == "wide" ? EventMode::Wide : EventMode::Fast;
}

/**
 * @brief 将 CompareOp 转换为字符串
 */
inline const char* compareOpToString(CompareOp op) {
    switch (op) {
        case CompareOp::Eq: return "eq";
        case CompareOp::Ne: return "ne";
        case CompareOp::Gt: return "gt";
        case CompareOp::Lt: return "lt";
        case CompareOp::Ge: return "ge";
        case CompareOp::Le: return "le";
        default: return "eq";
    }
}

/**
 * @brief 将字符串转换为 CompareOp
 */
inline CompareOp stringToCompareOp(const std::string& str) {
    if (str == "ne") return CompareOp::Ne;
    if (str == "gt") return CompareOp::Gt;
    if (str == "lt") return CompareOp::Lt;
    if (str == "ge") return CompareOp::Ge;
    if (str == "le") return CompareOp::Le;
    return CompareOp::Eq;  // 默认
}

/**
 * @brief 将 TriggerType 转换为字符串
 */
inline const char* triggerTypeToString(TriggerType type) {
    switch (type) {
        case TriggerType::KernelEvent: return "kernel_event";
        case TriggerType::Topic: return "topic";
        case TriggerType::Timeout: return "timeout";
        default: return "kernel_event";
    }
}

/**
 * @brief 将字符串转换为 TriggerType
 */
inline TriggerType stringToTriggerType(const std::string& str) {
    if (str == "topic") return TriggerType::Topic;
    if (str == "timeout") return TriggerType::Timeout;
    return TriggerType::KernelEvent;  // 默认
}

} // namespace config
} // namespace qst

