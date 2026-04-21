/**
 * @file main.cpp
 * @brief QSchedTracer - 主程序入口
 *
 * 用法:
 *   qst_tracer [选项]
 *
 *   -b <MB>    缓冲区大小 (默认: 10)
 *   -o <dir>   输出目录 (默认: .)
 *   -a         全量采集 (默认: scheduling filtered)
 *   -v         详细输出
 *   -h         显示帮助
 */

#include "qst/core/tracer_engine.hpp"
#include "qst/config/types.hpp"
#include "qst/log.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h>

static qst::core::TracerEngine* g_engine = nullptr;

static void signalHandler(int sig) {
    if (!g_engine) return;
    if (sig == SIGUSR1) {
        g_engine->requestDump();
    } else {
        g_engine->requestStop();
    }
}

static void printUsage(const char* prog) {
    std::fprintf(stderr,
        "QSchedTracer - QNX Ring Mode Scheduler Tracer (QST v4)\n"
        "\n"
        "Usage: %s [options]\n"
        "\n"
        "Options:\n"
        "  -b <MB>    Buffer size in MB (default: 10)\n"
        "  -o <dir>   Output directory (default: .)\n"
        "  -p <name>  Output file prefix (default: tracer)\n"
        "  -a         Capture all events (default: scheduling only)\n"
        "  -v         Verbose output\n"
        "  -h         Show this help\n"
        "\n"
        "Output file:\n"
        "  prefix.YYYYMMDD.HHMMSS.uuuuuu.qst\n"
        "\n"
        "Signals:\n"
        "  SIGINT/SIGTERM  Dump and exit\n"
        "  SIGUSR1         Dump and restart (for multi-round testing)\n"
        "\n",
        prog);
}

int main(int argc, char* argv[]) {
    qst::config::TracerConfig config;

    int opt;
    while ((opt = getopt(argc, argv, "b:o:p:avh")) != -1) {
        switch (opt) {
        case 'b':
            config.buffer_size_mb = static_cast<uint32_t>(std::atoi(optarg));
            break;
        case 'o':
            config.output_dir = optarg;
            break;
        case 'p':
            config.file_prefix = optarg;
            break;
        case 'a':
            config.event_filter = qst::config::EventFilter::All;
            break;
        case 'v':
            config.verbose = true;
            break;
        case 'h':
            printUsage(argv[0]);
            return 0;
        default:
            printUsage(argv[0]);
            return 1;
        }
    }

    if (config.verbose) {
        qst::log::setLevel(qst::log::Level::Debug);
    }

    if (config.buffer_size_mb == 0 || config.buffer_size_mb > 1024) {
        std::fprintf(stderr, "Error: buffer_size_mb must be 1-1024\n");
        return 1;
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGUSR1, signalHandler);

    try {
        qst::core::TracerEngine engine(config);
        g_engine = &engine;

        int ret = engine.run();

        g_engine = nullptr;
        return ret;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception: {}", e.what());
        return 1;
    }
}
