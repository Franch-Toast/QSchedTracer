#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <set>

#include "qst/constants.h"
#include "qst/buffer_reader.h"
#include "qst/json_writer.h"

namespace qst {

using ArgsMap = std::unordered_map<std::string, std::string>;

struct Stats {
    int64_t thread_slices = 0;
    int64_t cpu_running_slices = 0;
    int64_t interrupt_slices = 0;
    int64_t interrupt_instants = 0;
    int64_t kercall_instants = 0;
    int64_t comm_instants = 0;
    int64_t system_instants = 0;
    int64_t sync_flows = 0;
    int64_t ipc_msg_flows = 0;
    int64_t ipc_send_ack_flows = 0;
    int64_t ipc_reply_flows = 0;
    int64_t signal_flows = 0;
    int64_t total_events_processed = 0;
    int64_t total_buffers = 0;
    double file_size_kb = 0;
};

struct ThreadPending {
    int64_t ts_ns;
    const char* state;
    int cpu_id;
    int priority;
    int policy;
    bool is_vthread;
    bool is_wide;
};

struct CpuPending {
    int64_t ts_ns;
    int pid;
    int tid;
};

struct IntPending {
    int64_t ts_ns;
    ArgsMap args;
};

struct CombineBuf {
    int int_event;
    int cpu_id;
    int64_t ts_ns;
    std::vector<uint32_t> data;
};

struct ThreadCombine {
    int cpu_id, pid, tid;
    const char* state;
    bool is_vthread;
    int priority, policy, partition_id, sched_flags;
    bool has_cont;
    int64_t ts_ns;
};

class StreamingParser {
public:
    StreamingParser(const std::string& filepath, bool verbose = false,
                    bool lightweight = false,
                    const std::set<int>* filter_pids = nullptr,
                    const std::set<int>* filter_tids = nullptr,
                    int64_t filter_time_start_ns = 0,
                    int64_t filter_time_end_ns = 0);

    Stats parse_and_export_json(const std::string& output_path);
    void parse_summary_print();
    const Stats& stats() const { return stats_; }

private:
    std::string filepath_;
    bool verbose_;
    bool lightweight_;

    const std::set<int>* filter_pids_ = nullptr;
    const std::set<int>* filter_tids_ = nullptr;
    int64_t filter_time_start_ = 0;
    int64_t filter_time_end_ = 0;

    FileAccess fa_;
    BufferReader* reader_ = nullptr;
    JsonTraceWriter* jw_ = nullptr;

    Stats stats_;
    std::unordered_map<int, std::string> kercall_names_;

    // Thread combine buffers
    std::unordered_map<std::string, ThreadCombine> thread_combine_;
    std::unordered_map<std::string, CombineBuf> proc_combine_;
    std::unordered_map<std::string, CombineBuf> combine_buf_;

    // Per-thread last state
    std::unordered_map<int64_t, ThreadPending> thread_pending_;

    // Per-CPU running thread
    std::unordered_map<int, CpuPending> cpu_pending_;

    // Interrupt pairing
    std::unordered_map<int64_t, IntPending> int_pending_;

    int flow_id_ = 0;

    // Sync flow tracking
    std::unordered_map<int64_t, std::pair<uint64_t, int64_t>> thread_sync_wait_;
    std::unordered_map<int, std::tuple<uint64_t, int64_t, int, int>> active_sync_release_;

    // IPC flow tracking
    std::unordered_map<int64_t, int64_t> ipc_send_pending_;
    std::unordered_map<int64_t, std::tuple<int, int, int64_t>> ipc_reply_pending_;

    // Signal flow tracking
    std::unordered_map<int, std::tuple<int, int, int64_t, int>> signal_pending_;

    // Helper methods
    static int64_t make_key(int a, int b) {
        return (static_cast<int64_t>(a) << 32) | static_cast<uint32_t>(b);
    }

    bool in_time_range(int64_t ts_ns) const;
    bool pid_tid_match(int pid, int tid = 0) const;

    std::string get_process_label(int pid) const;
    std::string get_thread_label(int pid, int tid) const;
    std::string get_thread_name(int pid, int tid) const;

    void resolve_pid_tid_args(ArgsMap& args) const;

    // Core pipeline
    void process_backward();
    void process_global(int64_t clock_freq, int64_t anchor_ns);
    void process_percpu(int64_t clock_freq, int64_t anchor_ns);

    struct CarryInfo { uint32_t first_cycle; int64_t first_ts; };
    std::vector<CarryInfo> compute_carries(
        const std::vector<BufferMeta>& buffers,
        int64_t clock_freq, int64_t anchor_ns);
    void forward_dispatch(const std::vector<BufferMeta>& buffers,
                         const std::vector<CarryInfo>& carries,
                         int64_t clock_freq);

    // Event dispatch
    void dispatch_event(uint32_t header, uint32_t d0, uint32_t d1, uint32_t d2,
                        int64_t ts_ns);

    // Handlers
    void handle_pr_th(int int_event, int cpu_id, int struct_type,
                      uint32_t d0, uint32_t d1, uint32_t d2, int64_t ts_ns);
    void emit_thread_state(int64_t ts_ns, int cpu_id, int pid, int tid,
                           const char* state, bool is_vthread, bool is_wide,
                           int priority, int policy,
                           int partition_id, int sched_flags);

    void handle_process_event(int int_event, int cpu_id, int struct_type,
                              uint32_t d0, uint32_t d1, uint32_t d2, int64_t ts_ns);
    void emit_proc_lifecycle(int64_t ts_ns, int cpu_id, int ppid, int pid,
                             const char* name);

    void handle_interrupt(int int_event, int cpu_id, int struct_type,
                          uint32_t d0, uint32_t d1, uint32_t d2, int64_t ts_ns);
    void emit_interrupt(int int_event, int cpu_id,
                        const std::vector<uint32_t>& data, int64_t ts_ns);
    void emit_irq_instant(int cpu_id, int64_t ts_ns,
                          const char* name, const ArgsMap& args);

    void handle_kercall(int int_event, int cpu_id, int struct_type,
                        uint32_t d0, uint32_t d1, uint32_t d2, int64_t ts_ns);
    void emit_kercall(int int_event, int cpu_id,
                      const std::vector<uint32_t>& data_words,
                      bool is_wide, int64_t ts_ns);

    void handle_comm(int int_event, int cpu_id, int struct_type,
                     uint32_t d0, uint32_t d1, uint32_t d2, int64_t ts_ns);
    void emit_comm(int int_event, int cpu_id,
                   const std::vector<uint32_t>& data_words,
                   bool is_wide, int64_t ts_ns);

    void handle_system(int int_event, int cpu_id, int struct_type,
                       uint32_t d0, uint32_t d1, uint32_t d2, int64_t ts_ns);
    void emit_system(int int_event, int cpu_id,
                     const std::vector<uint32_t>& data_words,
                     bool is_wide, int64_t ts_ns);

    // Flow tracking
    void track_sync_flow(int call_num, bool is_enter, const char* suffix,
                         int cpu_id, int pid, int tid,
                         const ArgsMap& raw_args, int64_t ts_ns);
    void check_sync_flow(const char* prev_state, const char* new_state,
                         int pid, int tid, int64_t ts_ns);

    void track_ipc_kercall(int call_num, bool is_enter, const char* suffix,
                           int cpu_id, int pid, int tid,
                           const ArgsMap& raw_args, int64_t ts_ns);
    void track_ipc_comm(int comm_code, int pid, int tid,
                        const ArgsMap& raw_args, int64_t ts_ns);
    void check_ipc_reply_flow(const char* prev_state, const char* new_state,
                              int pid, int tid, int64_t ts_ns);

    void track_signal_kercall(int call_num, bool is_enter, int cpu_id,
                              int pid, int tid,
                              const ArgsMap& raw_args, int64_t ts_ns);
    void check_signal_flow(const char* prev_state, const char* new_state,
                           int pid, int tid, int64_t ts_ns);

    // Build args
    ArgsMap build_thread_args(int pid, int tid, int cpu_id,
                              int priority, int policy,
                              bool is_vthread, bool is_wide) const;

    // Flush
    void flush_pending_slices();

    // Args helpers
    static void args_set_int(ArgsMap& m, const char* key, int val) {
        m[key] = "#" + std::to_string(val);
    }
    static void args_set_int64(ArgsMap& m, const char* key, int64_t val) {
        m[key] = "#" + std::to_string(val);
    }
    static void args_set_str(ArgsMap& m, const char* key, const std::string& val) {
        m[key] = val;
    }
    static void args_set_uint(ArgsMap& m, const char* key, uint32_t val) {
        m[key] = "#" + std::to_string(val);
    }
    static int args_get_int(const ArgsMap& m, const char* key, int def = 0) {
        auto it = m.find(key);
        if (it == m.end()) return def;
        const auto& v = it->second;
        if (v.empty()) return def;
        try {
            return std::stoi(v[0] == '#' ? v.substr(1) : v);
        } catch (...) { return def; }
    }
    static uint64_t args_get_uint64(const ArgsMap& m, const char* key) {
        auto it = m.find(key);
        if (it == m.end()) return 0;
        const auto& v = it->second;
        if (v.empty()) return 0;
        try {
            return std::stoull(v[0] == '#' ? v.substr(1) : v);
        } catch (...) { return 0; }
    }
};

}  // namespace qst
