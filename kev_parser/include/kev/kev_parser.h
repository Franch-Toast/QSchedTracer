#pragma once

#include "kev/types.h"
#include "kev/raw_kev_ts.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#ifdef KEV_LINUX_HOST
#include "kev/qnx_compat.h"
#else
#include <sys/traceparser.h>
#include <sys/trace.h>
#endif

namespace kev {

class NameTracker;
class OutputWriter;
class StatsSummary;
struct EventFilter;

struct PrescanResult {
    int64_t  first_event_ns = 0;
    int64_t  real_end_ns    = 0;
    uint64_t post_collection_count = 0;
    bool     gap_detected   = false;
    bool     clk_failed     = false;
    uint64_t cycles_per_sec = 0;
};

struct ParseConfig {
    std::string input_path;
    std::string output_path;
    std::string format = "jsonl";  // "jsonl" | "text"
    std::string mode   = "full";   // "full" | "summary"
    bool verbose = false;
    int64_t time_start_ns = -1;
    int64_t time_end_ns   = -1;
    std::vector<int> filter_pids;
    std::vector<int> filter_tids;
    std::vector<std::string> filter_classes;

    int64_t wall_start_ns = -1;
    int64_t wall_end_ns   = -1;
};

class KevParser {
public:
    explicit KevParser(ParseConfig config);
    ~KevParser();

    KevParser(const KevParser&) = delete;
    KevParser& operator=(const KevParser&) = delete;

    PrescanResult prescan();
    bool parse();
    void set_time_filter(int64_t start_ns, int64_t end_ns);

private:
    ParseConfig config_;
    struct traceparser_state* tps_ = nullptr;

    std::unique_ptr<NameTracker>   names_;
    std::unique_ptr<OutputWriter>  writer_;
    std::unique_ptr<StatsSummary>  stats_;
    std::unique_ptr<EventFilter>   filter_;

    uint64_t clock_freq_ = 0;
    uint64_t seq_ = 0;
    bool meta_written_ = false;

    static constexpr int64_t GAP_THRESHOLD_NS = 1000000000LL;
    int64_t  last_event_ts_      = 0;
    int64_t  first_event_ns_     = 0;
    uint64_t post_collection_count_ = 0;
    bool     prescan_mode_       = false;
    int64_t  prescan_real_end_ns_ = 0;

    bool     clk_fallback_     = false;
    int      current_event_cpu_  = 0;
    uint32_t current_event_d0_  = 0;

    std::unique_ptr<RawKevTimestamps> raw_ts_;

    bool check_ring_gap(int64_t ts);

    int64_t prescan_prev_ts_        = 0;
    int64_t prescan_max_gap_        = 0;
    int64_t prescan_gap_before_ts_  = 0;
    int64_t prescan_gap_boundary_   = 0;

    PrescanResult cached_prescan_;
    bool prescan_done_ = false;

    struct PendingName {
        enum Kind { PROC, THREAD } kind;
        int pid, tid_or_parent;
        std::string name;
    };
    std::vector<PendingName> pending_names_;

    // Per-CPU tracking: which pid/tid is running on each CPU
    struct CpuContext { int pid = 0; int tid = 0; };
    CpuContext cpu_ctx_[64];

    void register_callbacks();
    Metadata extract_metadata();
    int64_t get_timestamp_ns();
    bool should_write(int64_t ts, int pid, int tid, const char* cls) const;
    void write_event(int cpu, const char* cls, const char* evt,
                     int pid, int tid, int64_t ts, KVList data = {});
    std::string build_process_summary_json();

    static int cb_thread(struct traceparser_state*, void*, unsigned, unsigned, unsigned*, unsigned);
    static int cb_control(struct traceparser_state*, void*, unsigned, unsigned, unsigned*, unsigned);
    static int cb_kercall_enter(struct traceparser_state*, void*, unsigned, unsigned, unsigned*, unsigned);
    static int cb_kercall_exit(struct traceparser_state*, void*, unsigned, unsigned, unsigned*, unsigned);
    static int cb_comm(struct traceparser_state*, void*, unsigned, unsigned, unsigned*, unsigned);
    static int cb_proc_create(struct traceparser_state*, void*, unsigned, unsigned, unsigned*, unsigned);
    static int cb_proc_create_name(struct traceparser_state*, void*, unsigned, unsigned, unsigned*, unsigned);
    static int cb_proc_thread_name(struct traceparser_state*, void*, unsigned, unsigned, unsigned*, unsigned);
    static int cb_proc_destroy(struct traceparser_state*, void*, unsigned, unsigned, unsigned*, unsigned);
    static int cb_system(struct traceparser_state*, void*, unsigned, unsigned, unsigned*, unsigned);
    static int cb_interrupt(struct traceparser_state*, void*, unsigned, unsigned, unsigned*, unsigned);

    void on_thread(unsigned header, unsigned* buf, unsigned len);
    void on_control(unsigned header, unsigned* buf, unsigned len);
    void on_kercall(unsigned header, unsigned* buf, unsigned len, bool is_enter);
    void on_comm(unsigned header, unsigned* buf, unsigned len);
    void on_proc_create(unsigned header, unsigned* buf, unsigned len);
    void on_proc_create_name(unsigned header, unsigned* buf, unsigned len);
    void on_proc_thread_name(unsigned header, unsigned* buf, unsigned len);
    void on_proc_destroy(unsigned header, unsigned* buf, unsigned len);
    void on_system(unsigned header, unsigned* buf, unsigned len);
    void on_interrupt(unsigned header, unsigned* buf, unsigned len);
};

} // namespace kev
