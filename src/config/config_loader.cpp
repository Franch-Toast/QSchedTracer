/**
 * @file config/config_loader.cpp
 * @brief QSchedTracer - JSON 配置加载器实现
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#include "qst/config/config_loader.hpp"
#include "qst/log.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

// nlohmann/json - header-only JSON library
#include "nlohmann/json.hpp"

namespace qst {
namespace config {

using json = nlohmann::json;

// ============================================================================
// 内部解析函数
// ============================================================================

namespace {

/**
 * @brief 解析调度配置
 */
SchedulingConfig parseScheduling(const json& j) {
    SchedulingConfig config;
    if (j.contains("enabled")) {
        config.enabled = j["enabled"].get<bool>();
    }
    if (j.contains("mode")) {
        config.mode = stringToEventMode(j["mode"].get<std::string>());
    }
    return config;
}

/**
 * @brief 解析事件类配置
 */
EventClassConfig parseEventClass(const json& j) {
    EventClassConfig config;
    if (j.contains("class")) {
        config.class_id = j["class"].get<int>();
    }
    if (j.contains("mode")) {
        config.mode = stringToEventMode(j["mode"].get<std::string>());
    }
    if (j.contains("comment")) {
        config.comment = j["comment"].get<std::string>();
    }
    return config;
}

/**
 * @brief 解析特定事件配置
 */
SpecificEventConfig parseSpecificEvent(const json& j) {
    SpecificEventConfig config;
    if (j.contains("class")) {
        config.class_id = j["class"].get<int>();
    }
    if (j.contains("event")) {
        config.event_id = j["event"].get<int>();
    }
    if (j.contains("mode")) {
        config.mode = stringToEventMode(j["mode"].get<std::string>());
    }
    if (j.contains("comment")) {
        config.comment = j["comment"].get<std::string>();
    }
    return config;
}

/**
 * @brief 解析扩展事件配置
 */
ExtraEventsConfig parseExtraEvents(const json& j) {
    ExtraEventsConfig config;
    
    if (j.contains("classes")) {
        for (const auto& item : j["classes"]) {
            config.classes.push_back(parseEventClass(item));
        }
    }
    
    if (j.contains("specific_events")) {
        for (const auto& item : j["specific_events"]) {
            config.specific_events.push_back(parseSpecificEvent(item));
        }
    }
    
    return config;
}

/**
 * @brief 解析触发条件
 */
TriggerCondition parseCondition(const json& j) {
    TriggerCondition cond;
    cond.has_condition = true;
    
    if (j.contains("data_index")) {
        cond.data_index = j["data_index"].get<int>();
    }
    if (j.contains("op")) {
        cond.op = stringToCompareOp(j["op"].get<std::string>());
    }
    if (j.contains("value")) {
        cond.value = j["value"].get<int64_t>();
    }
    
    return cond;
}

/**
 * @brief 解析触发器配置
 */
TriggerConfig parseTrigger(const json& j) {
    TriggerConfig config;
    
    if (j.contains("type")) {
        config.type = stringToTriggerType(j["type"].get<std::string>());
    }
    
    // kernel_event 参数
    if (j.contains("class")) {
        config.event_class = j["class"].get<int>();
    }
    if (j.contains("event")) {
        config.event_id = j["event"].get<int>();
    }
    if (j.contains("condition")) {
        config.condition = parseCondition(j["condition"]);
    }
    
    // topic 参数
    if (j.contains("topic_name")) {
        config.topic_name = j["topic_name"].get<std::string>();
    }
    
    // timeout 参数
    if (j.contains("interval_sec")) {
        config.interval_sec = j["interval_sec"].get<int>();
    }
    
    // 注释
    if (j.contains("comment")) {
        config.comment = j["comment"].get<std::string>();
    }
    
    return config;
}

} // anonymous namespace

// ============================================================================
// ConfigLoader 实现
// ============================================================================

TracerConfig ConfigLoader::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open config file: " + path);
    }
    
    std::stringstream buffer;
    buffer << f.rdbuf();
    
    return loadFromString(buffer.str());
}

TracerConfig ConfigLoader::loadFromString(const std::string& json_str) {
    json j;
    try {
        j = json::parse(json_str);
    } catch (const json::parse_error& e) {
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    }
    
    TracerConfig config;
    
    // 版本
    if (j.contains("version")) {
        config.version = j["version"].get<std::string>();
    }
    
    // 缓冲区配置
    if (j.contains("buffer") && j["buffer"].contains("size_mb")) {
        config.buffer_size_mb = j["buffer"]["size_mb"].get<size_t>();
    }
    
    // 调度配置
    if (j.contains("scheduling")) {
        config.scheduling = parseScheduling(j["scheduling"]);
    }
    
    // 扩展事件
    if (j.contains("extra_events")) {
        config.extra_events = parseExtraEvents(j["extra_events"]);
    }
    
    // 触发器
    if (j.contains("triggers")) {
        for (const auto& item : j["triggers"]) {
            config.triggers.push_back(parseTrigger(item));
        }
    }
    
    return config;
}

TracerConfig ConfigLoader::getDefault() {
    TracerConfig config;
    
    config.version = "1.0";
    config.buffer_size_mb = 10;
    
    // 默认启用调度追踪
    config.scheduling.enabled = true;
    config.scheduling.mode = EventMode::Fast;
    
    // 默认添加 __KER_SIGNAL_KILL (Wide mode)
    // extra_events 使用外部 class ID (用于 TraceEvent API)
    SpecificEventConfig signal_kill;
    signal_kill.class_id = 3;   // _NTO_TRACE_KERCALLENTER (外部 class)
    signal_kill.event_id = 26;  // __KER_SIGNAL_KILL
    signal_kill.mode = EventMode::Wide;
    signal_kill.comment = "KERCALLENTER: __KER_SIGNAL_KILL (外部 class=3)";
    config.extra_events.specific_events.push_back(signal_kill);
    
    // 默认添加 SIGKILL 触发器
    // triggers 使用内部 class ID (用于事件处理器中的匹配)
    // 内部 class 映射: 2 = _TRACE_KER_CALL_C (对应 KERCALL/KERCALLENTER/KERCALLEXIT)
    TriggerConfig trigger;
    trigger.type = TriggerType::KernelEvent;
    trigger.event_class = 2;    // 内部 class ID (_TRACE_KER_CALL_C)
    trigger.event_id = 26;      // __KER_SIGNAL_KILL
    trigger.condition.has_condition = true;
    trigger.condition.data_index = 3;  // signo
    trigger.condition.op = CompareOp::Eq;
    trigger.condition.value = 9;       // SIGKILL
    trigger.comment = "内部 class=2, event=26, data[3]=signo, 9=SIGKILL";
    config.triggers.push_back(trigger);
    
    return config;
}

bool ConfigLoader::validate(const TracerConfig& config, std::string& error) {
    // 检查缓冲区大小
    if (config.buffer_size_mb == 0) {
        error = "buffer_size_mb must be > 0";
        return false;
    }
    if (config.buffer_size_mb > 1024) {
        error = "buffer_size_mb too large (max 1024 MB)";
        return false;
    }
    
    // 检查事件类 ID
    for (const auto& ec : config.extra_events.classes) {
        if (ec.class_id < 0 || ec.class_id > 14) {
            error = "Invalid event class ID: " + std::to_string(ec.class_id);
            return false;
        }
    }
    
    // 检查特定事件
    for (const auto& se : config.extra_events.specific_events) {
        if (se.class_id < 0 || se.class_id > 14) {
            error = "Invalid event class ID in specific_events: " + std::to_string(se.class_id);
            return false;
        }
    }
    
    // 检查触发器
    for (const auto& t : config.triggers) {
        if (t.type == TriggerType::KernelEvent) {
            if (t.event_class < 0 || t.event_class > 14) {
                error = "Invalid trigger event class: " + std::to_string(t.event_class);
                return false;
            }
            if (t.condition.has_condition) {
                if (t.condition.data_index < 0 || t.condition.data_index > 10) {
                    error = "Invalid condition data_index: " + std::to_string(t.condition.data_index);
                    return false;
                }
            }
        }
    }
    
    return true;
}

std::string ConfigLoader::toJson(const TracerConfig& config, bool pretty) {
    json j;
    
    j["version"] = config.version;
    j["buffer"]["size_mb"] = config.buffer_size_mb;
    
    // scheduling
    j["scheduling"]["enabled"] = config.scheduling.enabled;
    j["scheduling"]["mode"] = eventModeToString(config.scheduling.mode);
    
    // extra_events
    j["extra_events"]["classes"] = json::array();
    for (const auto& ec : config.extra_events.classes) {
        json item;
        item["class"] = ec.class_id;
        item["mode"] = eventModeToString(ec.mode);
        if (!ec.comment.empty()) {
            item["comment"] = ec.comment;
        }
        j["extra_events"]["classes"].push_back(item);
    }
    
    j["extra_events"]["specific_events"] = json::array();
    for (const auto& se : config.extra_events.specific_events) {
        json item;
        item["class"] = se.class_id;
        item["event"] = se.event_id;
        item["mode"] = eventModeToString(se.mode);
        if (!se.comment.empty()) {
            item["comment"] = se.comment;
        }
        j["extra_events"]["specific_events"].push_back(item);
    }
    
    // triggers
    j["triggers"] = json::array();
    for (const auto& t : config.triggers) {
        json item;
        item["type"] = triggerTypeToString(t.type);
        
        if (t.type == TriggerType::KernelEvent) {
            item["class"] = t.event_class;
            item["event"] = t.event_id;
            if (t.condition.has_condition) {
                item["condition"]["data_index"] = t.condition.data_index;
                item["condition"]["op"] = compareOpToString(t.condition.op);
                item["condition"]["value"] = t.condition.value;
            }
        } else if (t.type == TriggerType::Topic) {
            item["topic_name"] = t.topic_name;
        } else if (t.type == TriggerType::Timeout) {
            item["interval_sec"] = t.interval_sec;
        }
        
        if (!t.comment.empty()) {
            item["comment"] = t.comment;
        }
        
        j["triggers"].push_back(item);
    }
    
    return pretty ? j.dump(2) : j.dump();
}

} // namespace config
} // namespace qst

