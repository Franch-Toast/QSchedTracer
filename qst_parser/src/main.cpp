#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <set>
#include <chrono>
#include <getopt.h>

#include "qst/streaming_parser.h"
#include "qst/buffer_reader.h"

static int64_t parse_time_arg(const char* str, int64_t capture_start_ns,
                              int64_t capture_end_ns) {
    if (!str || !str[0]) return 0;

    if (str[0] == '+') {
        double secs = std::atof(str + 1);
        return capture_start_ns + static_cast<int64_t>(secs * 1e9);
    }
    if (str[0] == '-') {
        double secs = std::atof(str + 1);
        return capture_end_ns - static_cast<int64_t>(secs * 1e9);
    }

    int Y, M, D, h, m;
    double s;
    if (std::sscanf(str, "%d-%d-%d %d:%d:%lf", &Y, &M, &D, &h, &m, &s) == 6) {
        struct tm tm = {};
        tm.tm_year = Y - 1900;
        tm.tm_mon = M - 1;
        tm.tm_mday = D;
        tm.tm_hour = h;
        tm.tm_min = m;
        tm.tm_sec = static_cast<int>(s);
        double frac = s - tm.tm_sec;
        time_t epoch = timegm(&tm);
        return static_cast<int64_t>(epoch) * 1000000000LL +
               static_cast<int64_t>(frac * 1e9);
    }

    return std::atoll(str);
}

static qst::QstFileHeader read_qst_header(const char* path) {
    qst::QstFileHeader h;
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open %s\n", path);
        return h;
    }
    uint8_t buf[64];
    if (fread(buf, 1, 64, f) == 64)
        h.parse(buf);
    fclose(f);
    return h;
}

static void usage(const char* prog) {
    fprintf(stderr,
        "qst_parser — QNX QST v4 Trace File Parser\n"
        "\n"
        "  Parses QNX kernel trace files (.qst) and exports them as Chrome JSON\n"
        "  Trace Event Format (.json) for visualization in Perfetto UI or Chrome\n"
        "  Tracing. Supports both QNX 7.1 and QNX 8.0 trace files.\n"
        "\n"
        "USAGE:\n"
        "  %s [OPTIONS] <input.qst>\n"
        "\n"
        "OPTIONS:\n"
        "  -o, --output PATH       Set the output file path.\n"
        "                          Default: <input-basename>.json (same directory)\n"
        "\n"
        "  -f, --format FMT        Output format. Currently only 'json' is supported.\n"
        "                          Default: json\n"
        "\n"
        "  -n, --no-export         Parse the file and print a summary (OS version,\n"
        "                          CPUs, clock frequency, duration, buffer/event counts,\n"
        "                          process/thread names) without producing output.\n"
        "                          Useful for quick file inspection.\n"
        "\n"
        "  -l, --lightweight       Lightweight mode: only parse Thread state and CPU\n"
        "                          running events. Skip Interrupt, KerCall, Comm, and\n"
        "                          System events. Produces a smaller output file and\n"
        "                          runs faster. No flow events are generated.\n"
        "\n"
        "  -v, --verbose           Print detailed information during parsing, including\n"
        "                          buffer-level progress and name resolution.\n"
        "\n"
        "  --pid PID               Filter output by process ID. Only events belonging\n"
        "                          to the specified process(es) are exported. Can be\n"
        "                          specified multiple times to include multiple PIDs.\n"
        "\n"
        "  --tid TID               Filter output by thread ID. Must be combined with\n"
        "                          --pid. Only threads matching BOTH pid and tid filters\n"
        "                          are exported. Can be specified multiple times.\n"
        "\n"
        "  --time-start TIME       Filter events: only export events at or after TIME.\n"
        "  --time-end TIME         Filter events: only export events at or before TIME.\n"
        "\n"
        "  -h, --help              Show this help message and exit.\n"
        "\n"
        "TIME FORMATS:\n"
        "  +N.nnn                  Relative seconds from trace start (e.g., +5.0)\n"
        "  -N.nnn                  Relative seconds from trace end (e.g., -10.0)\n"
        "  \"YYYY-MM-DD HH:MM:SS[.frac]\"  Absolute wall clock time (UTC)\n"
        "  NNN                     Raw epoch nanosecond value\n"
        "\n"
        "OUTPUT FORMAT:\n"
        "  The output is a Chrome JSON Trace Event Format file containing:\n"
        "\n"
        "  • Thread State Slices (X)   — Duration events showing each thread's state\n"
        "                                (RUNNING, READY, SEND, RECEIVE, REPLY, MUTEX,\n"
        "                                CONDVAR, SEM, NANOSLEEP, SIGWAITINFO, etc.)\n"
        "  • CPU Running Slices (X)    — Which thread is running on each CPU over time\n"
        "  • Interrupt Slices (X)      — IRQ entry/exit paired as duration events\n"
        "  • KerCall Instants (i)      — Kernel call Enter/Exit/Interrupt with decoded\n"
        "                                argument fields (MsgSend, SignalKill, etc.)\n"
        "  • Comm Instants (i)         — IPC communication events (SMSG, SPULSE, REPLY)\n"
        "  • System Instants (i)       — System events (MMAP, MUNMAP, Timer, IPI, etc.)\n"
        "  • Flow Arrows (s/f)         — Causal links between events:\n"
        "      sync_mutex/condvar/sem  — Sync primitive wait → wake\n"
        "      ipc_msg                 — MsgSend → MsgReceive\n"
        "      ipc_send_ack            — MsgReceive → sender acknowledged\n"
        "      ipc_reply               — MsgReply → sender wakeup\n"
        "      signal                  — SignalKill → target SIGWAITINFO/SIGSUSPEND\n"
        "\n"
        "  Open the .json file at https://ui.perfetto.dev for visualization.\n"
        "\n"
        "EXAMPLES:\n"
        "  1) Parse and export to default JSON path (trace.json):\n"
        "       %s trace.qst\n"
        "\n"
        "  2) Specify a custom output path:\n"
        "       %s trace.qst -o /tmp/output.json\n"
        "\n"
        "  3) Quick inspection — print file summary without export:\n"
        "       %s trace.qst -n\n"
        "\n"
        "  4) Lightweight mode — thread/CPU events only (fast, small output):\n"
        "       %s trace.qst -l -o threads_only.json\n"
        "\n"
        "  5) Filter a specific process (e.g., PID 12345):\n"
        "       %s trace.qst --pid 12345\n"
        "\n"
        "  6) Filter multiple processes:\n"
        "       %s trace.qst --pid 12345 --pid 67890\n"
        "\n"
        "  7) Filter specific threads within a process:\n"
        "       %s trace.qst --pid 12345 --tid 1 --tid 3\n"
        "\n"
        "  8) Combine lightweight mode with PID filter:\n"
        "       %s trace.qst -l --pid 12345 -o filtered.json\n"
        "\n"
        "  9) Export only the last 5 seconds of the trace:\n"
        "       %s trace.qst --time-start -5\n"
        "\n"
        " 10) Export a specific wall clock time range:\n"
        "       %s trace.qst --time-start \"2026-03-19 13:53:00\" "
                "--time-end \"2026-03-19 13:54:00\"\n"
        "\n"
        " 11) Export the first 10 seconds:\n"
        "       %s trace.qst --time-end +10\n",
        prog, prog, prog, prog, prog, prog, prog, prog, prog,
        prog, prog, prog);
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"output",      required_argument, nullptr, 'o'},
        {"format",      required_argument, nullptr, 'f'},
        {"no-export",   no_argument,       nullptr, 'n'},
        {"lightweight", no_argument,       nullptr, 'l'},
        {"verbose",     no_argument,       nullptr, 'v'},
        {"pid",         required_argument, nullptr, 'P'},
        {"tid",         required_argument, nullptr, 'T'},
        {"time-start",  required_argument, nullptr, 'S'},
        {"time-end",    required_argument, nullptr, 'E'},
        {"help",        no_argument,       nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    std::string output_path;
    std::string format = "json";
    bool no_export = false;
    bool lightweight = false;
    bool verbose = false;
    std::set<int> pids, tids;
    const char* time_start_str = nullptr;
    const char* time_end_str = nullptr;

    int opt;
    while ((opt = getopt_long(argc, argv, "o:f:nlvh", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'o': output_path = optarg; break;
            case 'f': format = optarg; break;
            case 'n': no_export = true; break;
            case 'l': lightweight = true; break;
            case 'v': verbose = true; break;
            case 'P': pids.insert(atoi(optarg)); break;
            case 'T': tids.insert(atoi(optarg)); break;
            case 'S': time_start_str = optarg; break;
            case 'E': time_end_str = optarg; break;
            case 'h': usage(argv[0]); return 0;
            default: usage(argv[0]); return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: input file required\n\n");
        usage(argv[0]);
        return 1;
    }
    std::string input = argv[optind];

    int64_t filter_time_start_ns = 0;
    int64_t filter_time_end_ns = 0;
    if (time_start_str || time_end_str) {
        auto qh = read_qst_header(input.c_str());
        if (qh.magic == 0) {
            fprintf(stderr, "Error: cannot read QST header for time range resolution\n");
            return 1;
        }
        if (time_start_str)
            filter_time_start_ns = parse_time_arg(time_start_str,
                                                   qh.capture_start_ns, qh.capture_end_ns);
        if (time_end_str)
            filter_time_end_ns = parse_time_arg(time_end_str,
                                                 qh.capture_start_ns, qh.capture_end_ns);
    }

    const std::set<int>* filter_pids = pids.empty() ? nullptr : &pids;
    const std::set<int>* filter_tids = tids.empty() ? nullptr : &tids;

    qst::StreamingParser sp(input, verbose, lightweight,
                            filter_pids, filter_tids,
                            filter_time_start_ns, filter_time_end_ns);

    if (no_export) {
        auto t0 = std::chrono::steady_clock::now();
        sp.parse_summary_print();
        auto t1 = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(t1 - t0).count();
        printf("  Parse Time:     %.2f s\n", elapsed);
    } else {
        if (output_path.empty()) {
            size_t dot = input.rfind('.');
            if (dot != std::string::npos)
                output_path = input.substr(0, dot) + ".json";
            else
                output_path = input + ".json";
        }

        printf("Parsing and exporting: %s\n", input.c_str());
        printf("  Format: %s\n", format.c_str());
        if (lightweight) printf("  Mode: lightweight (thread/CPU events only)\n");
        if (filter_pids) printf("  PID filter: %zu process(es)\n", pids.size());
        if (filter_time_start_ns)
            printf("  Time start: %lld ns\n", (long long)filter_time_start_ns);
        if (filter_time_end_ns)
            printf("  Time end:   %lld ns\n", (long long)filter_time_end_ns);

        auto t0 = std::chrono::steady_clock::now();
        auto stats = sp.parse_and_export_json(output_path);
        auto t1 = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(t1 - t0).count();

        printf("\n=== Export Statistics ===\n");
        printf("  Thread State Slices:  %ld\n", (long)stats.thread_slices);
        printf("  CPU Running Slices:   %ld\n", (long)stats.cpu_running_slices);
        printf("  Interrupt Slices:     %ld\n", (long)stats.interrupt_slices);
        printf("  Interrupt Instants:   %ld\n", (long)stats.interrupt_instants);
        printf("  KerCall Instants:     %ld\n", (long)stats.kercall_instants);
        printf("  Comm Instants:        %ld\n", (long)stats.comm_instants);
        printf("  System Instants:      %ld\n", (long)stats.system_instants);

        long total_flows = stats.sync_flows + stats.ipc_msg_flows +
                           stats.ipc_send_ack_flows + stats.ipc_reply_flows +
                           stats.signal_flows;
        if (total_flows) {
            printf("  Flow Events:          %ld  "
                   "(sync:%ld msg:%ld ack:%ld reply:%ld signal:%ld)\n",
                   total_flows, (long)stats.sync_flows,
                   (long)stats.ipc_msg_flows, (long)stats.ipc_send_ack_flows,
                   (long)stats.ipc_reply_flows, (long)stats.signal_flows);
        }
        printf("  Total Events:         %ld\n", (long)stats.total_events_processed);
        printf("  Total Buffers:        %ld\n", (long)stats.total_buffers);
        printf("  File Size:            %.1f KB\n", stats.file_size_kb);
        printf("  Time:                 %.2f s\n", elapsed);
        printf("\nOpen https://ui.perfetto.dev and load %s\n", output_path.c_str());
    }

    return 0;
}
