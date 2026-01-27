/**
 * @file main.cpp
 * @brief QSchedTracer - 主程序入口
 * 
 * @details
 * QNX 调度追踪器命令行入口。
 * 
 * ## 用法
 * 
 * ```bash
 * qst_tracer [选项]
 * 
 * 选项:
 *   -c <文件>  配置文件路径 (默认: ./qst_config.json)
 *   -b <MB>    缓冲区大小，覆盖配置 (默认: 配置文件值或 10)
 *   -v         详细输出
 *   -h         显示帮助
 * 
 * 示例:
 *   qst_tracer                          # 使用默认配置
 *   qst_tracer -c /etc/qst/config.json  # 使用指定配置
 *   qst_tracer -b 16                    # 覆盖缓冲区大小
 * ```
 * 
 * @note 本程序仅支持 QNX Neutrino RTOS 平台
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#include "qst/core/tracer_engine.hpp"
#include "qst/config/config_loader.hpp"
#include "qst/log.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h>

// 全局追踪器指针 (用于信号处理)
static qst::core::TracerEngine* g_engine = nullptr;

/**
 * @brief 信号处理函数
 */
static void signalHandler(int sig) {
    LOG_WARN("收到信号 {}，正在停止...", sig);
    if (g_engine) {
        g_engine->requestStop();
    }
}

/**
 * @brief 打印使用说明
 */
static void printUsage(const char* prog) {
    LOG_INFO("QSchedTracer - QNX 调度追踪器 (飞行记录仪模式)");
    LOG_INFO("===================================================");
    LOG_INFO("用法: {} [选项]", prog);
    LOG_INFO("");
    LOG_INFO("选项:");
    LOG_INFO("  -c <文件>  配置文件路径 (默认: ./qst_config.json)");
    LOG_INFO("  -b <MB>    缓冲区大小，覆盖配置 (默认: 配置文件值或 10)");
    LOG_INFO("  -v         详细输出");
    LOG_INFO("  -h         显示帮助");
    LOG_INFO("");
    LOG_INFO("功能说明:");
    LOG_INFO("  1. 调度追踪: 持续采集线程状态变化事件");
    LOG_INFO("  2. 触发落盘: 配置文件定义触发条件 (如 SIGKILL)");
    LOG_INFO("  3. Ctrl+C 退出时自动落盘");
    LOG_INFO("");
    LOG_INFO("输出文件:");
    LOG_INFO("  trace_YYYYMMDD_HHMMSS.qst  (固定格式)");
    LOG_INFO("");
    LOG_INFO("示例:");
    LOG_INFO("  {} -c config.json          # 使用指定配置", prog);
    LOG_INFO("  {} -b 16                   # 16MB 缓冲", prog);
    LOG_INFO("");
    LOG_INFO("解析:");
    LOG_INFO("  python3 qst_parse.py trace_*.qst -o trace.json");
    LOG_INFO("  # 在 https://ui.perfetto.dev/ 中打开");
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    std::string config_file;
    size_t buffer_size_mb = 0;  // 0 = 使用配置文件值
    bool verbose = false;
    
    // 参数解析
    int opt;
    while ((opt = getopt(argc, argv, "c:b:vh")) != -1) {
        switch (opt) {
        case 'c':
            config_file = optarg;
            break;
        case 'b':
            buffer_size_mb = static_cast<size_t>(std::atoi(optarg));
            break;
        case 'v':
            verbose = true;
            break;
        case 'h':
            printUsage(argv[0]);
            return 0;
        default:
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // 设置日志级别
    if (verbose) {
        qst::log::setLevel(qst::log::Level::Debug);
    }
    
    // 加载配置
    qst::config::TracerConfig config;
    
    if (!config_file.empty()) {
        try {
            LOG_INFO("加载配置文件: {}", config_file);
            config = qst::config::ConfigLoader::loadFromFile(config_file);
        } catch (const std::exception& e) {
            LOG_ERROR("加载配置文件失败: {}", e.what());
            return 1;
        }
    } else {
        LOG_WARN("使用默认配置");
        config = qst::config::ConfigLoader::getDefault();
    }
    
    // 覆盖缓冲区大小
    if (buffer_size_mb > 0) {
        config.buffer_size_mb = buffer_size_mb;
        LOG_INFO("覆盖缓冲区大小: {} MB", buffer_size_mb);
    }
    
    // 校验配置
    std::string error;
    if (!qst::config::ConfigLoader::validate(config, error)) {
        LOG_ERROR("配置无效: {}", error);
        return 1;
    }
    
    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // 创建并运行追踪引擎
    try {
        qst::core::TracerEngine engine(config);
        g_engine = &engine;
        
        int ret = engine.run();
        
        g_engine = nullptr;
        return ret;
    } catch (const std::exception& e) {
        LOG_ERROR("异常: {}", e.what());
        return 1;
    }
}
