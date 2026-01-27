/**
 * @file config/config_loader.hpp
 * @brief QSchedTracer - JSON 配置加载器
 * 
 * @details
 * 从 JSON 文件加载配置，支持默认配置。
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#pragma once

#include "types.hpp"
#include <string>

namespace qst {
namespace config {

/**
 * @brief 配置加载器
 */
class ConfigLoader {
public:
    /**
     * @brief 从 JSON 文件加载配置
     * @param path 配置文件路径
     * @return 解析后的配置
     * @throws std::runtime_error 如果文件无法打开或解析失败
     */
    static TracerConfig loadFromFile(const std::string& path);
    
    /**
     * @brief 从 JSON 字符串加载配置
     * @param json_str JSON 字符串
     * @return 解析后的配置
     * @throws std::runtime_error 如果解析失败
     */
    static TracerConfig loadFromString(const std::string& json_str);
    
    /**
     * @brief 获取默认配置
     * @return 默认配置
     */
    static TracerConfig getDefault();
    
    /**
     * @brief 校验配置有效性
     * @param config 要校验的配置
     * @param error 输出错误信息
     * @return true 配置有效, false 配置无效
     */
    static bool validate(const TracerConfig& config, std::string& error);
    
    /**
     * @brief 将配置导出为 JSON 字符串
     * @param config 配置
     * @param pretty 是否格式化输出
     * @return JSON 字符串
     */
    static std::string toJson(const TracerConfig& config, bool pretty = true);
};

} // namespace config
} // namespace qst

