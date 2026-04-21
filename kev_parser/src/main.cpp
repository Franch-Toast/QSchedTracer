#include "kev/kev_parser.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cinttypes>
#include <string>
#include <vector>
#include <regex>

static void print_usage(const char* prog) {
    std::fprintf(stderr,
        "kev_parser — QNX Kernel Event (.kev) Trace File Parser\n"
        "\n"
        "  Parses QNX tracelogger .kev files using the QNX libtraceparser library\n"
        "  and exports structured event data as JSON Lines (.jsonl) or human-readable\n"
        "  text (.txt). Supports both QNX 7.1 and QNX 8.0 trace files.\n"
        "  Automatically detects and handles ring-buffer sequence gaps.\n"
        "\n"
        "USAGE:\n"
        "  %s [OPTIONS] <input.kev>\n"
        "\n"
        "OUTPUT OPTIONS:\n"
        "  -o PATH             Set the output file path.\n"
        "                      Default: <input>.jsonl (or <input>.txt with -f text)\n"
        "                      Use '-' to write to stdout.\n"
        "\n"
        "  -f FORMAT           Output format:\n"
        "                        jsonl  — JSON Lines, one event per line (default)\n"
        "                        text   — Human-readable columnar text format\n"
        "\n"
        "  -m MODE             Output mode:\n"
        "                        full    — Output all matched events (default)\n"
        "                        summary — Print only per-event-class statistics,\n"
        "                                  no individual events\n"
        "\n"
        "  -v, --verbose       Print detailed progress and diagnostics to stderr,\n"
        "                      including trace metadata and time resolution info.\n"
        "\n"
        "FILTERING OPTIONS:\n"
        "  --time-start TIME   Only output events at or after this time.\n"
        "                      See TIME FORMATS below.\n"
        "\n"
        "  --time-end TIME     Only output events at or before this time.\n"
        "                      See TIME FORMATS below.\n"
        "\n"
        "  --pid PID,...       Filter by process ID. Comma-separated list.\n"
        "                      Only events from specified PIDs are output.\n"
        "\n"
        "  --tid TID,...       Filter by thread ID. Comma-separated list.\n"
        "                      Only events from specified TIDs are output.\n"
        "\n"
        "  --class CLS,...     Filter by event class. Comma-separated list.\n"
        "                      Available classes:\n"
        "                        THREAD         — Thread state transitions\n"
        "                        KERCALL_ENTER  — Kernel call entry events\n"
        "                        KERCALL_EXIT   — Kernel call exit events\n"
        "                        COMM           — IPC communication events\n"
        "                        SYSTEM         — System events (MMAP, Timer, etc.)\n"
        "                        INT            — Hardware interrupts\n"
        "                        PROCESS        — Process create/destroy/name events\n"
        "\n"
        "TIME FORMATS:\n"
        "  +N.nnn              Relative offset from trace start, in seconds.\n"
        "                      Example: +2.5 means 2.5 seconds after first event.\n"
        "\n"
        "  -N.nnn              Relative offset from trace end, in seconds.\n"
        "                      Example: -5 means the last 5 seconds of the trace.\n"
        "\n"
        "  YYYY-MM-DD HH:MM:SS[.fractional]\n"
        "                      Absolute wall-clock time (UTC). Requires the filename\n"
        "                      to contain timestamp metadata (e.g., tracer.log.PID.\n"
        "                      YYYYMMDD.HHMMSS.uuuuuu-YYYYMMDD.HHMMSS.uuuuuu.kev).\n"
        "                      Fractional part can be up to 9 digits (nanoseconds).\n"
        "\n"
        "  NNN                 Raw trace nanoseconds (absolute kernel time).\n"
        "\n"
        "  -h, --help          Show this help message and exit.\n"
        "\n"
        "JSONL OUTPUT FORMAT:\n"
        "  Each line is a JSON object with fields:\n"
        "    {\"ts\": <nanoseconds>, \"cpu\": N, \"class\": \"...\", \"event\": \"...\",\n"
        "     \"pid\": N, \"tid\": N, \"args\": {...}}\n"
        "\n"
        "  Common class/event combinations:\n"
        "    class=THREAD  event=RUNNING/READY/SEND/RECEIVE/REPLY/MUTEX/CONDVAR/...\n"
        "    class=INT     event=INT_ENTRY/INT_EXIT/INT_HANDLER_ENTRY/...\n"
        "    class=KERCALL event=MsgSendv_ENTER/MsgSendv_EXIT/SignalKill_ENTER/...\n"
        "    class=COMM    event=SMSG/SPULSE/REPLY/SIGNAL/...\n"
        "    class=SYSTEM  event=MMAP/MUNMAP/TIMER/IPI/...\n"
        "\n"
        "EXAMPLES:\n"
        "  1) Parse to default .jsonl output:\n"
        "       %s trace.kev\n"
        "\n"
        "  2) Quick summary (event count statistics only, no data output):\n"
        "       %s trace.kev -m summary -o -\n"
        "\n"
        "  3) Export as human-readable text:\n"
        "       %s trace.kev -f text -o trace.txt\n"
        "\n"
        "  4) Show only the last 5 seconds of the trace:\n"
        "       %s trace.kev --time-start -5\n"
        "\n"
        "  5) Filter by absolute wall-clock time and process ID:\n"
        "       %s trace.kev --time-start \"2026-03-19 13:53:00\" --pid 16388\n"
        "\n"
        "  6) Filter specific event classes:\n"
        "       %s trace.kev --class THREAD,INT\n"
        "\n"
        "  7) Pipe summary to stdout for scripting:\n"
        "       %s trace.kev -m summary -o - 2>/dev/null\n"
        "\n"
        "  8) Combine time window + PID + class filter:\n"
        "       %s trace.kev --time-start +1 --time-end +3 --pid 1234 --class KERCALL_ENTER\n"
        "\n"
        "  9) Verbose mode to see trace metadata:\n"
        "       %s trace.kev -v -m summary -o -\n",
        prog, prog, prog, prog, prog, prog, prog, prog, prog, prog);
}

static std::vector<int> parse_int_list(const char* s) {
    std::vector<int> result;
    const char* p = s;
    while (*p) {
        result.push_back(std::atoi(p));
        while (*p && *p != ',') ++p;
        if (*p == ',') ++p;
    }
    return result;
}

static std::vector<std::string> parse_str_list(const char* s) {
    std::vector<std::string> result;
    std::string token;
    for (const char* p = s; ; ++p) {
        if (*p == ',' || *p == '\0') {
            if (!token.empty()) result.push_back(token);
            token.clear();
            if (*p == '\0') break;
        } else {
            token += *p;
        }
    }
    return result;
}

static int64_t parse_filename_wallclock(const std::string& ts_str) {
    if (ts_str.size() < 20) return -1;
    struct tm tm = {};
    int year, mon, day, hour, min, sec, usec = 0;
    if (std::sscanf(ts_str.c_str(), "%4d%2d%2d.%2d%2d%2d.%6d",
                    &year, &mon, &day, &hour, &min, &sec, &usec) < 6)
        return -1;
    tm.tm_year = year - 1900;
    tm.tm_mon  = mon - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min  = min;
    tm.tm_sec  = sec;
    time_t epoch = timegm(&tm);
    if (epoch < 0) return -1;
    return static_cast<int64_t>(epoch) * 1000000000LL +
           static_cast<int64_t>(usec) * 1000LL;
}

static void extract_wall_times_from_filename(const std::string& path,
                                              int64_t& wall_start_ns,
                                              int64_t& wall_end_ns) {
    wall_start_ns = -1;
    wall_end_ns = -1;

    auto slash = path.rfind('/');
    std::string basename = (slash != std::string::npos) ? path.substr(slash + 1) : path;

    std::regex re("(\\d{8}\\.\\d{6}\\.\\d{6})");
    auto begin = std::sregex_iterator(basename.begin(), basename.end(), re);
    auto end_it = std::sregex_iterator();

    std::vector<std::string> matches;
    for (auto it = begin; it != end_it; ++it) {
        matches.push_back((*it)[1].str());
    }

    if (matches.size() >= 2) {
        wall_start_ns = parse_filename_wallclock(matches[0]);
        wall_end_ns   = parse_filename_wallclock(matches[1]);
    } else if (matches.size() == 1) {
        wall_end_ns = parse_filename_wallclock(matches[0]);
    }
}

static int64_t parse_wallclock_datetime(const char* s) {
    struct tm tm = {};
    char frac[16] = {};
    int n = std::sscanf(s, "%d-%d-%d %d:%d:%d.%15[0-9]",
                        &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                        &tm.tm_hour, &tm.tm_min, &tm.tm_sec, frac);
    if (n < 6) return -1;
    tm.tm_year -= 1900;
    tm.tm_mon  -= 1;
    time_t epoch = timegm(&tm);
    if (epoch < 0) return -1;

    int64_t frac_ns = 0;
    if (n >= 7 && frac[0]) {
        char padded[10] = "000000000";
        for (int i = 0; i < 9 && frac[i]; ++i) padded[i] = frac[i];
        frac_ns = std::strtoll(padded, nullptr, 10);
    }
    return static_cast<int64_t>(epoch) * 1000000000LL + frac_ns;
}

enum class TimeKind { RAW_NS, REL_START, REL_END, WALLCLOCK };

struct TimeSpec {
    TimeKind kind = TimeKind::RAW_NS;
    int64_t  value_ns = 0;

    int64_t resolve(int64_t first_ns, int64_t real_end_ns,
                    int64_t wall_offset_ns) const {
        switch (kind) {
            case TimeKind::RAW_NS:    return value_ns;
            case TimeKind::REL_START: return first_ns + value_ns;
            case TimeKind::REL_END:   return real_end_ns - value_ns;
            case TimeKind::WALLCLOCK: return value_ns - wall_offset_ns;
        }
        return value_ns;
    }
};

static TimeSpec parse_time_spec(const char* s) {
    TimeSpec ts;
    if (s[0] == '+') {
        ts.kind = TimeKind::REL_START;
        ts.value_ns = static_cast<int64_t>(std::strtod(s + 1, nullptr) * 1e9);
    } else if (s[0] == '-' && (s[1] >= '0' && s[1] <= '9')) {
        ts.kind = TimeKind::REL_END;
        ts.value_ns = static_cast<int64_t>(std::strtod(s + 1, nullptr) * 1e9);
    } else if (std::strchr(s, '-') && std::strchr(s, ':')) {
        ts.kind = TimeKind::WALLCLOCK;
        ts.value_ns = parse_wallclock_datetime(s);
    } else {
        ts.kind = TimeKind::RAW_NS;
        ts.value_ns = std::strtoll(s, nullptr, 10);
    }
    return ts;
}

int main(int argc, char** argv) {
    kev::ParseConfig config;
    const char* time_start_str = nullptr;
    const char* time_end_str   = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--verbose") == 0) {
            config.verbose = true;
        } else if (std::strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            config.output_path = argv[++i];
        } else if (std::strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            config.format = argv[++i];
        } else if (std::strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            config.mode = argv[++i];
        } else if (std::strcmp(argv[i], "--time-start") == 0 && i + 1 < argc) {
            time_start_str = argv[++i];
        } else if (std::strcmp(argv[i], "--time-end") == 0 && i + 1 < argc) {
            time_end_str = argv[++i];
        } else if (std::strcmp(argv[i], "--pid") == 0 && i + 1 < argc) {
            config.filter_pids = parse_int_list(argv[++i]);
        } else if (std::strcmp(argv[i], "--tid") == 0 && i + 1 < argc) {
            config.filter_tids = parse_int_list(argv[++i]);
        } else if (std::strcmp(argv[i], "--class") == 0 && i + 1 < argc) {
            config.filter_classes = parse_str_list(argv[++i]);
        } else if (argv[i][0] != '-') {
            config.input_path = argv[i];
        } else {
            std::fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (config.input_path.empty()) {
        std::fprintf(stderr, "Error: no input file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    extract_wall_times_from_filename(config.input_path,
                                     config.wall_start_ns, config.wall_end_ns);

    if (config.output_path.empty()) {
        std::string ext = (config.format == "text") ? ".txt" : ".jsonl";
        auto dot = config.input_path.rfind('.');
        if (dot != std::string::npos)
            config.output_path = config.input_path.substr(0, dot) + ext;
        else
            config.output_path = config.input_path + ext;
    }

    bool needs_prescan = (config.wall_end_ns > 0);

    TimeSpec ts_start, ts_end;
    bool has_start = false, has_end = false;
    if (time_start_str) {
        ts_start = parse_time_spec(time_start_str);
        has_start = true;
        if (ts_start.kind != TimeKind::RAW_NS) needs_prescan = true;
    }
    if (time_end_str) {
        ts_end = parse_time_spec(time_end_str);
        has_end = true;
        if (ts_end.kind != TimeKind::RAW_NS) needs_prescan = true;
    }

    if (!needs_prescan) {
        if (has_start && ts_start.kind == TimeKind::RAW_NS)
            config.time_start_ns = ts_start.value_ns;
        if (has_end && ts_end.kind == TimeKind::RAW_NS)
            config.time_end_ns = ts_end.value_ns;
    }

    kev::KevParser parser(config);

    if (needs_prescan) {
        auto pr = parser.prescan();

        int64_t wall_offset = 0;
        int64_t first_ns    = pr.first_event_ns;
        int64_t real_end_ns = pr.real_end_ns;

        if (pr.clk_failed && config.wall_end_ns > 0) {
            wall_offset = 0;
        } else if (config.wall_end_ns > 0 && real_end_ns > 0) {
            wall_offset = config.wall_end_ns - real_end_ns;
        }

        int64_t resolved_start = -1, resolved_end = -1;
        if (has_start)
            resolved_start = ts_start.resolve(first_ns, real_end_ns, wall_offset);
        if (has_end)
            resolved_end = ts_end.resolve(first_ns, real_end_ns, wall_offset);

        if (has_start || has_end)
            parser.set_time_filter(resolved_start, resolved_end);

        if (config.verbose) {
            std::fprintf(stderr, "Wall clock: start=%s end=%s\n",
                         config.wall_start_ns > 0 ? "detected" : "none",
                         config.wall_end_ns > 0 ? "detected" : "none");
            if (has_start)
                std::fprintf(stderr, "  time-start resolved: %" PRId64 " ns\n",
                             resolved_start);
            if (has_end)
                std::fprintf(stderr, "  time-end resolved: %" PRId64 " ns\n",
                             resolved_end);
        }
    }

    return parser.parse() ? 0 : 1;
}
