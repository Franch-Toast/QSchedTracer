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
        "QSchedTracer v3.0 - QNX Ring Mode Scheduler Tracer\n"
        "\n"
        "  Lightweight kernel event recorder for QNX Neutrino RTOS.\n"
        "  Uses Ring Mode + mmap zero-copy to continuously capture scheduling\n"
        "  events with minimal overhead. Output is QST v4 binary format.\n"
        "\n"
        "USAGE\n"
        "  %s [OPTIONS]\n"
        "\n"
        "OPTIONS\n"
        "  -b <MB>     Kernel trace buffer size in megabytes (default: 10).\n"
        "              Range: 1-1024. Each kernel buffer is ~16 KB (sizeof(tracebuf_t)).\n"
        "              QNX 7.1: total buffers = MB * 1024 / 16.\n"
        "              QNX 8.0: per-CPU buffers = total / num_cpus.\n"
        "              Larger buffers reduce the chance of ring overwrites during\n"
        "              high-frequency events but consume more kernel memory.\n"
        "\n"
        "  -o <dir>    Output directory (default: current directory).\n"
        "              Created automatically if it does not exist.\n"
        "\n"
        "  -p <name>   Output file prefix (default: \"tracer\").\n"
        "              Final filename: <prefix>.YYYYMMDD.HHMMSS.uuuuuu.qst\n"
        "\n"
        "  -a          Capture ALL event classes (default: scheduling only).\n"
        "              Default mode captures: Thread (wide), Process, Comm (wide),\n"
        "              Control (buffer events), selected KerCalls (mutex, semaphore,\n"
        "              condvar, MsgSend/Receive/Reply, SchedYield).\n"
        "              With -a: ADDALLCLASSES + SETCLASSWIDE on all classes,\n"
        "              including System, Interrupt, and all KerCalls.\n"
        "\n"
        "  -v          Verbose output. Enables debug-level logging including\n"
        "              per-buffer state dumps before/after each operation.\n"
        "\n"
        "  -h          Show this help message and exit.\n"
        "\n"
        "SIGNALS\n"
        "  SIGINT      (Ctrl+C) Stop tracing, dump ring buffer, and exit.\n"
        "  SIGTERM     Same as SIGINT.\n"
        "  SIGUSR1     Dump current ring buffer to a .qst file, then restart\n"
        "              tracing (ring buffer reused, new file created).\n"
        "              Can be sent multiple times for multi-round capture.\n"
        "\n"
        "OUTPUT FORMAT\n"
        "  QST v4 binary file containing raw kernel tracebuf_t blocks:\n"
        "\n"
        "    QstFileHeader (64 bytes)\n"
        "      magic='QST4', version=4, clock_freq, capture_start/end_ns,\n"
        "      num_cpus, os_version (710/800), tracebuf_size, data_offset\n"
        "    SectionHeader (24 bytes, magic='DATA')\n"
        "      Raw tracebuf_t[] — scheduling/IPC/kernel call events\n"
        "    SectionHeader (24 bytes, magic='PINF')\n"
        "      Raw tracebuf_t[] — process/thread name information\n"
        "\n"
        "  Use qst_parser (C++) to convert .qst files to JSON or Chrome Trace.\n"
        "\n"
        "PLATFORM SUPPORT\n"
        "  QNX 7.1    ALLOCBUFFER -> mmap(MAP_PHYS) physical address mapping\n"
        "  QNX 8.0    LOGGER_ATTACH -> per-CPU ALLOCBUFFER virtual address\n"
        "             (falls back to legacy paddr+mmap if ATTACH fails)\n"
        "\n"
        "EXAMPLES\n"
        "  # Basic capture (10 MB buffer, scheduling events, output to cwd)\n"
        "  %s\n"
        "\n"
        "  # 50 MB buffer, all events, output to /data/traces\n"
        "  %s -a -b 50 -o /data/traces\n"
        "\n"
        "  # Custom prefix, verbose logging\n"
        "  %s -p myapp -v\n"
        "\n"
        "  # Multi-round capture (terminal 1: start tracer)\n"
        "  %s -b 20 -o /tmp -p test\n"
        "  # (terminal 2: trigger dump+restart, repeat as needed)\n"
        "  kill -USR1 $(pidof qst_tracer)\n"
        "  kill -USR1 $(pidof qst_tracer)\n"
        "  # (terminal 2: final stop)\n"
        "  kill -INT $(pidof qst_tracer)\n"
        "\n"
        "  # Parse output files\n"
        "  qst_parser test.*.qst -o trace.json\n"
        "  qst_parser test.*.qst -n              # summary only\n"
        "\n",
        prog, prog, prog, prog, prog);
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
