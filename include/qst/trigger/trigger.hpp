/**
 * @file trigger/trigger.hpp
 * @brief QSchedTracer - 触发器接口定义
 * 
 * @details
 * 定义触发器的基础接口和数据结构。
 * 使用策略模式 (Strategy Pattern) 支持多种触发类型。
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#pragma once

#include "qst/config/types.hpp"
#include <string>
#include <cstdint>

namespace qst {
namespace trigger {

// ============================================================================
// 触发结果
// ============================================================================

/**
 * @brief 触发器检查结果
 */
enum class TriggerResult {
    None,           ///< 未触发
    SaveAndContinue ///< 触发：保存数据后继续采集
};

// ============================================================================
// 触发器上下文
// ============================================================================

/**
 * @brief 触发器检查上下文
 * 
 * 在事件处理器中提供当前事件的信息，供触发器检查使用。
 */
struct TriggerContext {
    int event_class;                ///< 事件类 ID
    int event_id;                   ///< 事件 ID
    const uint32_t* data_array;     ///< event_data_t.data_array 指针
    size_t data_count;              ///< 数据项数量
    
    /**
     * @brief 获取指定索引的数据
     * @param index 数据索引
     * @return 数据值，索引越界返回 0
     */
    int64_t getData(int index) const {
        if (index >= 0 && static_cast<size_t>(index) < data_count) {
            return static_cast<int64_t>(data_array[index]);
        }
        return 0;
    }
};

// ============================================================================
// 触发器接口
// ============================================================================

/**
 * @brief 触发器接口 (策略模式)
 * 
 * 所有触发器类型都必须实现此接口。
 */
class ITrigger {
public:
    virtual ~ITrigger() = default;
    
    /**
     * @brief 检查是否触发
     * @param ctx 触发器上下文
     * @return 触发结果
     */
    virtual TriggerResult check(const TriggerContext& ctx) = 0;
    
    /**
     * @brief 获取触发器名称 (用于日志)
     * @return 触发器名称
     */
    virtual std::string name() const = 0;
    
    /**
     * @brief 获取触发器类型
     * @return 触发器类型
     */
    virtual config::TriggerType type() const = 0;
};

} // namespace trigger
} // namespace qst

