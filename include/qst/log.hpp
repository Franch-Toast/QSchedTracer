/**
 * @file log.hpp
 * @brief QSchedTracer - 日志工具 (基于 spdlog)
 * 
 * @details
 * 使用 spdlog 1.17.0 作为日志后端，支持彩色输出和格式化。
 * 
 * @author QSchedTracer Team
 * @date 2026-01-21
 */

#pragma once

// 使用 spdlog header-only 模式
#define SPDLOG_HEADER_ONLY

// 禁用线程信息 (减少依赖)
#define SPDLOG_NO_THREAD_ID

// 禁用 TLS (QNX 兼容性)
#define SPDLOG_NO_TLS

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

namespace qst {
namespace log {

// 日志级别 (与 spdlog 兼容)
enum class Level {
    Debug = spdlog::level::debug,
    Info = spdlog::level::info,
    Warn = spdlog::level::warn,
    Error = spdlog::level::err
};

namespace detail {

// 获取 QST 全局 logger
inline std::shared_ptr<spdlog::logger>& getLogger() {
    static std::shared_ptr<spdlog::logger> logger = nullptr;
    if (!logger) {
        // 创建 stderr 彩色输出 sink
        auto console_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        
        // 设置格式: [时间] [级别] [QST] 消息
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [QST] %v");
        
        // 创建 logger
        logger = std::make_shared<spdlog::logger>("qst", console_sink);
        logger->set_level(spdlog::level::info);  // 默认级别
        logger->flush_on(spdlog::level::warn);   // warn 及以上立即刷新
    }
    return logger;
}

// 当前日志级别引用
inline Level& currentLevelRef() {
    static Level level = Level::Info;
    return level;
}

} // namespace detail

// 获取当前日志级别
inline Level currentLevel() {
    return detail::currentLevelRef();
}

// 设置日志级别
inline void setLevel(Level level) {
    detail::currentLevelRef() = level;
    detail::getLogger()->set_level(static_cast<spdlog::level::level_enum>(level));
}

// 初始化日志系统 (可选，自动初始化)
inline void init() {
    detail::getLogger();  // 触发初始化
}

// 关闭日志系统 (可选，程序退出前调用)
inline void shutdown() {
    if (detail::getLogger()) {
        detail::getLogger()->flush();
    }
}

// 获取文件名 (不含路径)
inline const char* basename(const char* path) {
    const char* p = strrchr(path, '/');
    return p ? p + 1 : path;
}

} // namespace log
} // namespace qst

// ============================================================================
// 日志宏 (基于 spdlog)
// ============================================================================

// 调试日志 (带文件名和行号)
#define LOG_DEBUG(fmt, ...) \
    qst::log::detail::getLogger()->debug("[{}:{}] " fmt, \
        qst::log::basename(__FILE__), __LINE__, ##__VA_ARGS__)

// 信息日志
#define LOG_INFO(fmt, ...) \
    qst::log::detail::getLogger()->info(fmt, ##__VA_ARGS__)

// 警告日志
#define LOG_WARN(fmt, ...) \
    qst::log::detail::getLogger()->warn(fmt, ##__VA_ARGS__)

// 错误日志 (带文件名和行号)
#define LOG_ERROR(fmt, ...) \
    qst::log::detail::getLogger()->error("[{}:{}] " fmt, \
        qst::log::basename(__FILE__), __LINE__, ##__VA_ARGS__)

// 带条件的日志
#define LOG_DEBUG_IF(cond, fmt, ...) \
    do { if (cond) LOG_DEBUG(fmt, ##__VA_ARGS__); } while(0)

#define LOG_INFO_IF(cond, fmt, ...) \
    do { if (cond) LOG_INFO(fmt, ##__VA_ARGS__); } while(0)

#define LOG_WARN_IF(cond, fmt, ...) \
    do { if (cond) LOG_WARN(fmt, ##__VA_ARGS__); } while(0)

#define LOG_ERROR_IF(cond, fmt, ...) \
    do { if (cond) LOG_ERROR(fmt, ##__VA_ARGS__); } while(0)

