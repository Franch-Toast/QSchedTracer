#include "kev/output_writer.h"
#include <cstring>
#include <cinttypes>

namespace kev {

// ============================================================================
// JsonLinesWriter
// ============================================================================

JsonLinesWriter::JsonLinesWriter(FILE* out) : out_(out) {
    buf_.reserve(FLUSH_THRESHOLD + 4096);
}

JsonLinesWriter::~JsonLinesWriter() { flush(); }

void JsonLinesWriter::append(const char* s) { buf_.append(s); }
void JsonLinesWriter::append(const std::string& s) { buf_.append(s); }

void JsonLinesWriter::maybe_flush() {
    if (buf_.size() >= FLUSH_THRESHOLD) flush();
}

void JsonLinesWriter::flush() {
    if (!buf_.empty()) {
        std::fwrite(buf_.data(), 1, buf_.size(), out_);
        buf_.clear();
    }
    std::fflush(out_);
}

std::string JsonLinesWriter::escape_json(const std::string& s) {
    return kev::escape_json(s);
}

std::string JsonLinesWriter::format_time_s(int64_t ns) {
    char tmp[32];
    double s = static_cast<double>(ns) / 1e9;
    std::snprintf(tmp, sizeof(tmp), "%.9f", s);
    return tmp;
}

void JsonLinesWriter::write_metadata(const Metadata& meta) {
    char tmp[2048];
    std::snprintf(tmp, sizeof(tmp),
        "{\"type\":\"metadata\",\"file_info\":{"
        "\"qnx_version\":\"%s\","
        "\"machine\":\"%s\","
        "\"trace_date\":\"%s\","
        "\"boot_date\":\"%s\","
        "\"cpu_count\":%u,"
        "\"cycles_per_sec\":%" PRIu64 ","
        "\"trace_file\":\"%s\","
        "\"tracelogger_args\":\"%s\""
        "}}\n",
        escape_json(meta.qnx_version).c_str(),
        escape_json(meta.machine).c_str(),
        escape_json(meta.trace_date).c_str(),
        escape_json(meta.boot_date).c_str(),
        meta.cpu_count,
        meta.cycles_per_sec,
        escape_json(meta.trace_file).c_str(),
        escape_json(meta.tracelogger_args).c_str());
    append(tmp);
    maybe_flush();
}

void JsonLinesWriter::write_process(int pid, const std::string& name, int parent_pid) {
    char tmp[512];
    std::snprintf(tmp, sizeof(tmp),
        "{\"type\":\"process\",\"pid\":%d,\"name\":\"%s\",\"parent_pid\":%d}\n",
        pid, escape_json(name).c_str(), parent_pid);
    append(tmp);
    maybe_flush();
}

void JsonLinesWriter::write_thread_name(int pid, int tid, const std::string& name) {
    char tmp[512];
    std::snprintf(tmp, sizeof(tmp),
        "{\"type\":\"thread_name\",\"pid\":%d,\"tid\":%d,\"name\":\"%s\"}\n",
        pid, tid, escape_json(name).c_str());
    append(tmp);
    maybe_flush();
}

void JsonLinesWriter::write_event(const Event& ev) {
    char tmp[256];
    if (ev.has_wall_time()) {
        std::snprintf(tmp, sizeof(tmp),
            "{\"type\":\"event\",\"seq\":%" PRIu64
            ",\"time\":\"%s\",\"cpu\":%d"
            ",\"class\":\"%s\",\"event\":\"%s\""
            ",\"pid\":%d,\"tid\":%d",
            ev.seq, ev.wall_time, ev.cpu,
            ev.event_class, ev.event_name,
            ev.pid, ev.tid);
    } else {
        std::snprintf(tmp, sizeof(tmp),
            "{\"type\":\"event\",\"seq\":%" PRIu64 ",\"time_ns\":%" PRId64
            ",\"time_s\":\"%s\",\"cpu\":%d"
            ",\"class\":\"%s\",\"event\":\"%s\""
            ",\"pid\":%d,\"tid\":%d",
            ev.seq, ev.time_ns, format_time_s(ev.time_ns).c_str(), ev.cpu,
            ev.event_class, ev.event_name,
            ev.pid, ev.tid);
    }
    append(tmp);

    if (ev.process_name && !ev.process_name->empty()) {
        append(",\"process\":\"");
        append(escape_json(*ev.process_name));
        append("\"");
    }
    if (ev.thread_name && !ev.thread_name->empty()) {
        append(",\"thread\":\"");
        append(escape_json(*ev.thread_name));
        append("\"");
    }

    if (!ev.data.empty()) {
        append(",\"data\":{");
        bool first = true;
        for (auto& kv : ev.data) {
            if (!first) append(",");
            first = false;
            append("\"");
            append(kv.key);
            append("\":\"");
            append(escape_json(kv.value));
            append("\"");
        }
        append("}");
    }

    append("}\n");
    maybe_flush();
}

void JsonLinesWriter::write_summary(const StatsSummary& stats,
                                    const std::string& proc_names_json) {
    int64_t dur = (stats.last_time_ns > stats.first_time_ns)
                  ? (stats.last_time_ns - stats.first_time_ns) : 0;

    char tmp[512];
    std::snprintf(tmp, sizeof(tmp),
        "{\"type\":\"summary\",\"total_events\":%" PRIu64
        ",\"trace_start_ns\":%" PRId64
        ",\"trace_end_ns\":%" PRId64
        ",\"duration_ns\":%" PRId64 ",\"duration_s\":\"%s\""
        ",\"event_counts\":{"
        "\"thread\":%" PRIu64
        ",\"kercall_enter\":%" PRIu64
        ",\"kercall_exit\":%" PRIu64
        ",\"comm\":%" PRIu64
        ",\"process\":%" PRIu64
        ",\"system\":%" PRIu64
        ",\"control\":%" PRIu64
        ",\"interrupt\":%" PRIu64
        "}",
        stats.total_events,
        stats.first_time_ns, stats.last_time_ns,
        dur, format_time_s(dur).c_str(),
        stats.counts.thread, stats.counts.kercall_enter,
        stats.counts.kercall_exit, stats.counts.comm,
        stats.counts.process, stats.counts.system,
        stats.counts.control, stats.counts.interrupt);
    append(tmp);

    auto& r = stats.ring;
    std::snprintf(tmp, sizeof(tmp),
        ",\"ring_buffer\":{\"gap_detected\":%s"
        ",\"gap_threshold_ns\":%" PRId64
        ",\"real_end_trace_ns\":%" PRId64
        ",\"post_collection_count\":%" PRIu64,
        r.gap_detected ? "true" : "false",
        r.gap_threshold_ns,
        r.real_end_trace_ns,
        r.post_collection_count);
    append(tmp);
    if (r.has_wall_clock) {
        std::snprintf(tmp, sizeof(tmp),
            ",\"wall_offset_ns\":%" PRId64, r.wall_offset_ns);
        append(tmp);
    }
    if (!r.capture_wallclock_start.empty()) {
        append(",\"capture_wallclock_start\":\"");
        append(r.capture_wallclock_start);
        append("\"");
    }
    if (!r.capture_wallclock_end.empty()) {
        append(",\"capture_wallclock_end\":\"");
        append(r.capture_wallclock_end);
        append("\"");
    }
    append("}");

    auto states = stats.top_states();
    append(",\"thread_state_distribution\":{");
    for (size_t i = 0; i < states.size(); ++i) {
        if (i > 0) append(",");
        std::snprintf(tmp, sizeof(tmp), "\"%s\":%" PRIu64,
                      states[i].name.c_str(), states[i].count);
        append(tmp);
    }
    append("}");

    auto kcs = stats.top_kercalls(20);
    append(",\"top_kercalls\":[");
    for (size_t i = 0; i < kcs.size(); ++i) {
        if (i > 0) append(",");
        std::snprintf(tmp, sizeof(tmp), "{\"name\":\"%s\",\"count\":%" PRIu64 "}",
                      kcs[i].name.c_str(), kcs[i].count);
        append(tmp);
    }
    append("]");

    append(",\"processes\":");
    append(proc_names_json);

    append("}\n");
    flush();
}

// ============================================================================
// TextWriter
// ============================================================================

TextWriter::TextWriter(FILE* out) : out_(out) {
    buf_.reserve(FLUSH_THRESHOLD + 4096);
}

TextWriter::~TextWriter() { flush(); }

void TextWriter::maybe_flush() {
    if (buf_.size() >= FLUSH_THRESHOLD) flush();
}

void TextWriter::flush() {
    if (!buf_.empty()) {
        std::fwrite(buf_.data(), 1, buf_.size(), out_);
        buf_.clear();
    }
    std::fflush(out_);
}

void TextWriter::write_metadata(const Metadata& meta) {
    char tmp[1024];
    std::snprintf(tmp, sizeof(tmp),
        "=== QNX Kernel Trace ===\n"
        "QNX %s | %s | %u CPUs @ %" PRIu64 " Hz\n"
        "Captured: %s\n"
        "tracelogger: %s\n\n",
        meta.qnx_version.c_str(), meta.machine.c_str(),
        meta.cpu_count, meta.cycles_per_sec,
        meta.trace_date.c_str(), meta.tracelogger_args.c_str());
    buf_.append(tmp);
    maybe_flush();
}

void TextWriter::write_process(int pid, const std::string& name, int parent_pid) {
    (void)parent_pid;
    char tmp[256];
    std::snprintf(tmp, sizeof(tmp), "  PROCESS  pid=%-6d  %s\n", pid, name.c_str());
    buf_.append(tmp);
    maybe_flush();
}

void TextWriter::write_thread_name(int pid, int tid, const std::string& name) {
    char tmp[256];
    std::snprintf(tmp, sizeof(tmp), "  THREAD   pid=%-6d tid=%-4d  %s\n",
                  pid, tid, name.c_str());
    buf_.append(tmp);
    maybe_flush();
}

void TextWriter::write_event(const Event& ev) {
    char tmp[512];
    double ts = static_cast<double>(ev.time_ns) / 1e9;
    if (ev.has_wall_time()) {
        std::snprintf(tmp, sizeof(tmp),
            "[%s] CPU%d %-14s %-20s pid=%d/%d",
            ev.wall_time, ev.cpu, ev.event_class, ev.event_name,
            ev.pid, ev.tid);
    } else {
        std::snprintf(tmp, sizeof(tmp),
            "[%13.9f] CPU%d %-14s %-20s pid=%d/%d",
            ts, ev.cpu, ev.event_class, ev.event_name, ev.pid, ev.tid);
    }
    buf_.append(tmp);

    if (ev.process_name && !ev.process_name->empty()) {
        buf_.append(" ");
        buf_.append(*ev.process_name);
    }

    for (auto& kv : ev.data) {
        buf_.append(" ");
        buf_.append(kv.key);
        buf_.append("=");
        buf_.append(kv.value);
    }
    buf_.append("\n");
    maybe_flush();
}

void TextWriter::write_summary(const StatsSummary& stats,
                               const std::string&) {
    int64_t dur = (stats.last_time_ns > stats.first_time_ns)
                  ? (stats.last_time_ns - stats.first_time_ns) : 0;
    double dur_s = static_cast<double>(dur) / 1e9;

    char tmp[512];
    std::snprintf(tmp, sizeof(tmp),
        "\n--- Summary ---\n"
        "Total events: %" PRIu64 " | Duration: %.3f s\n"
        "Thread: %" PRIu64 " | KerCall: %" PRIu64 "+%" PRIu64
        " | Comm: %" PRIu64 " | System: %" PRIu64 "\n"
        "Process: %" PRIu64 " | Control: %" PRIu64 " | Int: %" PRIu64 "\n",
        stats.total_events, dur_s,
        stats.counts.thread,
        stats.counts.kercall_enter, stats.counts.kercall_exit,
        stats.counts.comm, stats.counts.system,
        stats.counts.process, stats.counts.control, stats.counts.interrupt);
    buf_.append(tmp);

    auto& r = stats.ring;
    if (r.gap_detected) {
        std::snprintf(tmp, sizeof(tmp),
            "Ring buffer: gap detected, %" PRIu64 " post-collection events filtered\n",
            r.post_collection_count);
        buf_.append(tmp);
    }
    if (!r.capture_wallclock_end.empty()) {
        buf_.append("Wall clock end: ");
        buf_.append(r.capture_wallclock_end);
        buf_.append("\n");
    }

    auto states = stats.top_states();
    if (!states.empty()) {
        buf_.append("\nThread states:\n");
        for (auto& s : states) {
            std::snprintf(tmp, sizeof(tmp), "  %-16s %8" PRIu64 "\n",
                          s.name.c_str(), s.count);
            buf_.append(tmp);
        }
    }

    auto kcs = stats.top_kercalls(15);
    if (!kcs.empty()) {
        buf_.append("\nTop kernel calls:\n");
        for (auto& k : kcs) {
            std::snprintf(tmp, sizeof(tmp), "  %-28s %8" PRIu64 "\n",
                          k.name.c_str(), k.count);
            buf_.append(tmp);
        }
    }

    flush();
}

// ============================================================================
// Factory
// ============================================================================

std::string escape_json(const std::string& s) {
    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc < 0x20 || c == '"' || c == '\\') goto slow_path;
    }
    return s;

slow_path:
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                    continue;
                out += c;
        }
    }
    return out;
}

std::unique_ptr<OutputWriter> create_writer(const std::string& format, FILE* out) {
    if (format == "text")
        return std::make_unique<TextWriter>(out);
    return std::make_unique<JsonLinesWriter>(out);
}

} // namespace kev
