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
 *   -d <秒>    采集时长 (默认: 5, 0=无限)
 *   -o <文件>  输出文件 (默认: trace.qst)
 *   -b <MB>    缓冲区大小 (默认: 10)
 *   -v         详细输出
 *   -h         显示帮助
 * 
 * 示例:
 *   qst_tracer -d 10 -o trace.qst
 *   qst_tracer -d 0 -b 8  # 8MB 缓冲，无限采集
 * ```
 * 
 * @note 本程序仅支持 QNX Neutrino RTOS 平台
 * 
 * @author QSchedTracer Team
 * @date 2026-01-21
 */

#include "qst/tracer.hpp"
#include "qst/log.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h>

// 全局追踪器指针 (用于信号处理)
static qst::Tracer* g_tracer = nullptr;

/**
 * @brief 信号处理函数
 */
static void signalHandler(int sig) {
    LOG_WARN("收到信号 {}，正在停止...", sig);
    if (g_tracer) {
        g_tracer->requestStop();
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
    LOG_INFO("  -d <秒>    采集时长 (默认: 0=无限，Ctrl+C 停止)");
    LOG_INFO("  -o <文件>  输出文件 (默认: trace.qst)");
    LOG_INFO("  -b <MB>    缓冲区大小 (默认: 10)");
    LOG_INFO("  -s         禁用 SIGKILL 触发落盘");
    LOG_INFO("  -v         详细输出");
    LOG_INFO("  -h         显示帮助");
    LOG_INFO("");
    LOG_INFO("功能说明:");
    LOG_INFO("  1. 调度追踪 (Fast mode): 持续采集线程状态变化事件");
    LOG_INFO("  2. SIGKILL 触发 (Wide mode): 检测到 SIGKILL 时自动落盘");
    LOG_INFO("     - 落盘后继续采集，直到 Ctrl+C 退出");
    LOG_INFO("     - 落盘文件名: signal_YYYYMMDD_HHMMSS_NNN.qst");
    LOG_INFO("");
    LOG_INFO("示例:");
    LOG_INFO("  {} -o trace.qst              # 无限采集，Ctrl+C 停止", prog);
    LOG_INFO("  {} -d 10 -o trace.qst        # 采集 10 秒", prog);
    LOG_INFO("  {} -d 0 -b 8                 # 8MB 缓冲，无限采集", prog);
    LOG_INFO("  {} -s -d 30                  # 禁用信号触发，采集 30 秒", prog);
    LOG_INFO("");
    LOG_INFO("解析:");
    LOG_INFO("  python3 qst_parse.py trace.qst -o trace.json");
    LOG_INFO("  # 在 https://ui.perfetto.dev/ 中打开");
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    qst::TracerConfig config;
    bool verbose = false;
    
    // 参数解析
    int opt;
    while ((opt = getopt(argc, argv, "d:o:b:svh")) != -1) {
        switch (opt) {
        case 'd':
            config.duration_sec = std::atoi(optarg);
            break;
        case 'o':
            config.output_file = optarg;
            break;
        case 'b':
            config.buffer_size = static_cast<size_t>(std::atoi(optarg)) * 1024 * 1024;
            break;
        case 's':
            config.enable_signal_trigger = false;
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
    
    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // 创建并运行追踪器
    try {
        qst::Tracer tracer(config);
        g_tracer = &tracer;
        
        int ret = tracer.run();
        
        g_tracer = nullptr;
        return ret;
    } catch (const std::exception& e) {
        LOG_ERROR("异常: {}", e.what());
        return 1;
    }
}

