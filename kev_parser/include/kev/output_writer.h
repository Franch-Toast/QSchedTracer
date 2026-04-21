#pragma once

#include "kev/types.h"
#include "kev/stats.h"
#include <cstdio>
#include <string>
#include <memory>

namespace kev {

class OutputWriter {
public:
    virtual ~OutputWriter() = default;

    virtual void write_metadata(const Metadata& meta) = 0;
    virtual void write_process(int pid, const std::string& name, int parent_pid) = 0;
    virtual void write_thread_name(int pid, int tid, const std::string& name) = 0;

    struct Event {
        uint64_t    seq;
        int64_t     time_ns;
        int         cpu;
        const char* event_class;
        const char* event_name;
        int         pid;
        int         tid;
        const std::string* process_name = nullptr;
        const std::string* thread_name  = nullptr;
        KVList      data;
        char        wall_time[40] = {};

        bool has_wall_time() const { return wall_time[0] != '\0'; }
    };
    virtual void write_event(const Event& ev) = 0;
    virtual void write_summary(const StatsSummary& stats,
                               const std::string& proc_names_json) = 0;
    virtual void flush() = 0;
};

class JsonLinesWriter : public OutputWriter {
public:
    explicit JsonLinesWriter(FILE* out);
    ~JsonLinesWriter() override;

    void write_metadata(const Metadata& meta) override;
    void write_process(int pid, const std::string& name, int parent_pid) override;
    void write_thread_name(int pid, int tid, const std::string& name) override;
    void write_event(const Event& ev) override;
    void write_summary(const StatsSummary& stats,
                       const std::string& proc_names_json) override;
    void flush() override;

private:
    FILE* out_;
    std::string buf_;
    static constexpr size_t FLUSH_THRESHOLD = 256 * 1024;

    void append(const char* s);
    void append(const std::string& s);
    void maybe_flush();
    static std::string escape_json(const std::string& s);
    static std::string format_time_s(int64_t ns);
};

class TextWriter : public OutputWriter {
public:
    explicit TextWriter(FILE* out);
    ~TextWriter() override;

    void write_metadata(const Metadata& meta) override;
    void write_process(int pid, const std::string& name, int parent_pid) override;
    void write_thread_name(int pid, int tid, const std::string& name) override;
    void write_event(const Event& ev) override;
    void write_summary(const StatsSummary& stats,
                       const std::string& proc_names_json) override;
    void flush() override;

private:
    FILE* out_;
    std::string buf_;
    static constexpr size_t FLUSH_THRESHOLD = 256 * 1024;

    void maybe_flush();
};

std::unique_ptr<OutputWriter> create_writer(const std::string& format, FILE* out);
std::string escape_json(const std::string& s);

} // namespace kev
