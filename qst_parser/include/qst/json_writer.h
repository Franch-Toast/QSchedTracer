#pragma once

#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace qst {

class JsonTraceWriter {
public:
    explicit JsonTraceWriter(FILE* f) : f_(f) {
        fputc('[', f_);
    }

    ~JsonTraceWriter() = default;

    void write_process_name(int pid, const std::string& name) {
        sep();
        fprintf(f_, "{\"ph\":\"M\",\"pid\":%d,\"tid\":0,"
                "\"name\":\"process_name\",\"args\":{\"name\":\"%s\"}}",
                pid, esc(name).c_str());
    }

    void write_thread_name(int pid, int tid, const std::string& name) {
        sep();
        fprintf(f_, "{\"ph\":\"M\",\"pid\":%d,\"tid\":%d,"
                "\"name\":\"thread_name\",\"args\":{\"name\":\"%s\"}}",
                pid, tid, esc(name).c_str());
    }

    void write_process_sort_index(int pid, int sort_index) {
        sep();
        fprintf(f_, "{\"ph\":\"M\",\"pid\":%d,\"tid\":0,"
                "\"name\":\"process_sort_index\",\"args\":{\"sort_index\":%d}}",
                pid, sort_index);
    }

    void write_complete(int pid, int tid, double start_us, double dur_us,
                        const char* name, const char* cat,
                        const std::unordered_map<std::string, std::string>* args = nullptr) {
        sep();
        if (args && !args->empty()) {
            fprintf(f_, "{\"ph\":\"X\",\"pid\":%d,\"tid\":%d,"
                    "\"ts\":%.3f,\"dur\":%.3f,"
                    "\"name\":\"%s\",\"cat\":\"%s\",\"args\":%s}",
                    pid, tid, start_us, dur_us, esc(name).c_str(), cat,
                    format_args(*args).c_str());
        } else {
            fprintf(f_, "{\"ph\":\"X\",\"pid\":%d,\"tid\":%d,"
                    "\"ts\":%.3f,\"dur\":%.3f,"
                    "\"name\":\"%s\",\"cat\":\"%s\"}",
                    pid, tid, start_us, dur_us, esc(name).c_str(), cat);
        }
        total_++;
    }

    void write_instant(int pid, int tid, double ts_us,
                       const char* name, const char* cat,
                       const std::unordered_map<std::string, std::string>* args = nullptr) {
        sep();
        if (args && !args->empty()) {
            fprintf(f_, "{\"ph\":\"i\",\"pid\":%d,\"tid\":%d,"
                    "\"ts\":%.3f,\"s\":\"t\","
                    "\"name\":\"%s\",\"cat\":\"%s\",\"args\":%s}",
                    pid, tid, ts_us, esc(name).c_str(), cat,
                    format_args(*args).c_str());
        } else {
            fprintf(f_, "{\"ph\":\"i\",\"pid\":%d,\"tid\":%d,"
                    "\"ts\":%.3f,\"s\":\"t\","
                    "\"name\":\"%s\",\"cat\":\"%s\"}",
                    pid, tid, ts_us, esc(name).c_str(), cat);
        }
        total_++;
    }

    void write_flow_start(int pid, int tid, double ts_us,
                          int flow_id, const char* name, const char* cat) {
        sep();
        fprintf(f_, "{\"ph\":\"s\",\"pid\":%d,\"tid\":%d,"
                "\"ts\":%.3f,\"id\":%d,"
                "\"name\":\"%s\",\"cat\":\"%s\"}",
                pid, tid, ts_us, flow_id, esc(name).c_str(), cat);
        total_++;
    }

    void write_flow_end(int pid, int tid, double ts_us,
                        int flow_id, const char* name, const char* cat) {
        sep();
        fprintf(f_, "{\"ph\":\"f\",\"pid\":%d,\"tid\":%d,"
                "\"ts\":%.3f,\"id\":%d,\"bp\":\"e\","
                "\"name\":\"%s\",\"cat\":\"%s\"}",
                pid, tid, ts_us, flow_id, esc(name).c_str(), cat);
        total_++;
    }

    void finalize() {
        fputc(']', f_);
        fflush(f_);
    }

    int total_events() const { return total_; }

private:
    FILE* f_;
    bool first_ = true;
    int total_ = 0;

    void sep() {
        if (first_) { first_ = false; return; }
        fputc(',', f_);
    }

    static std::string esc(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '"':  out += "\\\""; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default: out += c;
            }
        }
        return out;
    }

    static std::string esc(const char* s) {
        return esc(std::string(s));
    }

    static std::string format_args(
            const std::unordered_map<std::string, std::string>& args) {
        std::string out = "{";
        bool first = true;
        for (auto& [k, v] : args) {
            if (!first) out += ',';
            first = false;
            out += '"';
            out += esc(k);
            out += "\":";
            if (!v.empty() && v[0] == '#') {
                // Numeric value (marked with # prefix)
                out += v.substr(1);
            } else if (v == "true" || v == "false") {
                out += v;
            } else {
                out += '"';
                out += esc(v);
                out += '"';
            }
        }
        out += '}';
        return out;
    }
};

}  // namespace qst
