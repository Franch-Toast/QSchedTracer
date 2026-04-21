#include "qst/streaming_parser.h"
#include "qst/timestamp.h"

#include <algorithm>
#include <cstring>
#include <cstdio>

namespace qst {

// =============================================================================
// Constructor and helpers
// =============================================================================

StreamingParser::StreamingParser(const std::string& filepath, bool verbose,
                                 bool lightweight,
                                 const std::set<int>* filter_pids,
                                 const std::set<int>* filter_tids,
                                 int64_t filter_time_start_ns,
                                 int64_t filter_time_end_ns)
    : filepath_(filepath), verbose_(verbose), lightweight_(lightweight),
      filter_pids_(filter_pids), filter_tids_(filter_tids),
      filter_time_start_(filter_time_start_ns),
      filter_time_end_(filter_time_end_ns) {}

bool StreamingParser::in_time_range(int64_t ts_ns) const {
    if (filter_time_start_ && ts_ns < filter_time_start_) return false;
    if (filter_time_end_ && ts_ns > filter_time_end_) return false;
    return true;
}

bool StreamingParser::pid_tid_match(int pid, int tid) const {
    if (!filter_pids_) return true;
    if (filter_pids_->find(pid) == filter_pids_->end()) return false;
    if (filter_tids_ && tid && filter_tids_->find(tid) == filter_tids_->end())
        return false;
    return true;
}

std::string StreamingParser::get_process_label(int pid) const {
    auto it = reader_->process_names.find(pid);
    if (it != reader_->process_names.end())
        return it->second + " (" + std::to_string(pid) + ")";
    return "Process " + std::to_string(pid);
}

std::string StreamingParser::get_thread_label(int pid, int tid) const {
    auto it = reader_->thread_names.find({pid, tid});
    if (it != reader_->thread_names.end())
        return it->second + " (" + std::to_string(pid) + ":" + std::to_string(tid) + ")";
    auto pit = reader_->process_names.find(pid);
    if (pit != reader_->process_names.end())
        return pit->second + ":" + std::to_string(tid) +
               " (" + std::to_string(pid) + ":" + std::to_string(tid) + ")";
    return "Thread (" + std::to_string(pid) + ":" + std::to_string(tid) + ")";
}

std::string StreamingParser::get_thread_name(int pid, int tid) const {
    auto it = reader_->thread_names.find({pid, tid});
    if (it != reader_->thread_names.end()) return it->second;
    auto pit = reader_->process_names.find(pid);
    if (pit != reader_->process_names.end())
        return pit->second + ":" + std::to_string(tid);
    return "pid=" + std::to_string(pid) + ":" + std::to_string(tid);
}

void StreamingParser::resolve_pid_tid_args(ArgsMap& args) const {
    bool has_owner = args.count("process") > 0;
    for (auto it = args.begin(); it != args.end(); ++it) {
        const auto& key = it->first;
        auto& val = it->second;
        if (val.empty() || val[0] != '#') continue;
        int ival;
        try { ival = std::stoi(val.substr(1)); } catch (...) { continue; }
        if (ival <= 0) continue;

        if (key.find("pid") != std::string::npos) {
            if (key == "pid" && has_owner) continue;
            auto pit = reader_->process_names.find(ival);
            if (pit != reader_->process_names.end())
                val = pit->second + "(" + std::to_string(ival) + ")";
        } else if (key.find("tid") != std::string::npos) {
            if (key == "tid" && has_owner) continue;
        }
    }
}

// =============================================================================
// Parse and export
// =============================================================================

Stats StreamingParser::parse_and_export_json(const std::string& output_path) {
    fa_.open(filepath_);
    BufferReader reader(fa_, verbose_);
    reader_ = &reader;
    reader.read_structure();
    reader.process_names.emplace(1, "procnto");

    FILE* outf = fopen(output_path.c_str(), "w");
    if (!outf) throw std::runtime_error("Cannot open output: " + output_path);

    JsonTraceWriter jw(outf);
    jw_ = &jw;

    jw.write_process_name(CPU_PID, "CPU Tracks");
    jw.write_process_sort_index(CPU_PID, -1000);
    for (uint32_t cpu = 0; cpu < reader.header.num_cpus; cpu++) {
        jw.write_thread_name(CPU_PID, cpu_main_tid(cpu),
                             "CPU " + std::to_string(cpu));
        jw.write_thread_name(CPU_PID, cpu_irq_tid(cpu),
                             "CPU " + std::to_string(cpu) + " IRQ");
    }

    for (auto& [pid, name] : reader.process_names)
        jw.write_process_name(pid, name);
    for (auto& [key, name] : reader.thread_names)
        jw.write_thread_name(key.first, key.second, name);

    process_backward();
    flush_pending_slices();

    jw.finalize();
    fclose(outf);
    jw_ = nullptr;
    reader_ = nullptr;

    FILE* sf = fopen(output_path.c_str(), "r");
    if (sf) {
        fseek(sf, 0, SEEK_END);
        stats_.file_size_kb = ftell(sf) / 1024.0;
        fclose(sf);
    }

    fa_.close();
    return stats_;
}

void StreamingParser::parse_summary_print() {
    fa_.open(filepath_);
    BufferReader reader(fa_, verbose_);
    reader_ = &reader;
    reader.read_structure();

    auto& h = reader.header;
    int64_t total_events = 0;
    for (auto& b : reader.sorted_buffers)
        total_events += b.num_events;

    printf("\n=== QST v4 File Summary ===\n");
    printf("  OS Version:     %u\n", h.os_version);
    printf("  CPUs:           %u\n", h.num_cpus);
    printf("  Clock Freq:     %lu Hz\n", (unsigned long)h.clock_freq);
    double dur_s = (h.capture_end_ns - h.capture_start_ns) / 1e9;
    printf("  Duration:       %.3f s\n", dur_s);
    printf("  TraceBuffer:    %u B, data@%u\n", h.tracebuf_size, h.data_offset);
    printf("  Valid Buffers:  %zu\n", reader.sorted_buffers.size());
    printf("  Total Events:   %ld\n", (long)total_events);
    printf("  Process Names:  %zu\n", reader.process_names.size());
    printf("  Thread Names:   %zu\n", reader.thread_names.size());

    reader_ = nullptr;
    fa_.close();
}

// =============================================================================
// Backward timestamp reconstruction
// =============================================================================

void StreamingParser::process_backward() {
    auto& h = reader_->header;
    int64_t clock_freq = static_cast<int64_t>(h.clock_freq);
    int64_t anchor_ns = h.capture_end_ns;

    if (h.os_version == 800 && h.bufs_per_cpu > 0) {
        process_percpu(clock_freq, anchor_ns);
    } else {
        process_global(clock_freq, anchor_ns);
    }
}

void StreamingParser::process_global(int64_t clock_freq, int64_t anchor_ns) {
    auto& buffers = reader_->sorted_buffers;
    if (buffers.empty()) return;
    auto carries = compute_carries(buffers, clock_freq, anchor_ns);
    forward_dispatch(buffers, carries, clock_freq);
}

void StreamingParser::process_percpu(int64_t clock_freq, int64_t anchor_ns) {
    auto& groups = reader_->get_cpu_groups();

    uint32_t global_max_cycle = 0;
    bool first = true;
    for (auto& group : groups) {
        if (group.empty()) continue;
        auto& last_buf = group.back();
        size_t last_ev_off = static_cast<size_t>(last_buf.num_events - 1) * 16 + 4;
        uint32_t cycle = fa_.u32_at(last_buf.file_offset + last_ev_off);
        if (first || cycle_gt(cycle, global_max_cycle)) {
            global_max_cycle = cycle;
            first = false;
        }
    }

    for (auto& group : groups) {
        if (group.empty()) continue;
        auto& last_buf = group.back();
        size_t last_ev_off = static_cast<size_t>(last_buf.num_events - 1) * 16 + 4;
        uint32_t cpu_last_cycle = fa_.u32_at(last_buf.file_offset + last_ev_off);
        int32_t delta = signed_cycle_diff(global_max_cycle, cpu_last_cycle);
        int64_t cpu_anchor_ns = anchor_ns - cycles_to_ns(delta, clock_freq);

        auto carries = compute_carries(group, clock_freq, cpu_anchor_ns);
        forward_dispatch(group, carries, clock_freq);
    }
}

std::vector<StreamingParser::CarryInfo> StreamingParser::compute_carries(
        const std::vector<BufferMeta>& buffers,
        int64_t clock_freq, int64_t anchor_ns) {
    std::vector<CarryInfo> carries(buffers.size());

    uint32_t carry_cycle = 0;
    int64_t carry_ts = 0;
    bool has_carry = false;

    for (int i = static_cast<int>(buffers.size()) - 1; i >= 0; i--) {
        auto& buf = buffers[i];
        uint32_t first_cycle = fa_.u32_at(buf.file_offset + 4);
        uint32_t last_cycle = first_cycle;
        if (buf.num_events > 1) {
            last_cycle = fa_.u32_at(
                buf.file_offset + static_cast<size_t>(buf.num_events - 1) * 16 + 4);
        }

        int64_t last_ts;
        if (!has_carry) {
            last_ts = anchor_ns;
        } else {
            int32_t diff = signed_cycle_diff(carry_cycle, last_cycle);
            last_ts = carry_ts - floor_div(static_cast<int64_t>(diff) * BILLION, clock_freq);
        }

        int32_t inner_diff = signed_cycle_diff(last_cycle, first_cycle);
        int64_t first_ts = last_ts -
                           floor_div(static_cast<int64_t>(inner_diff) * BILLION, clock_freq);

        carry_cycle = first_cycle;
        carry_ts = first_ts;
        has_carry = true;
        carries[i] = {first_cycle, first_ts};
    }

    return carries;
}

// =============================================================================
// Forward dispatch
// =============================================================================

void StreamingParser::forward_dispatch(
        const std::vector<BufferMeta>& buffers,
        const std::vector<CarryInfo>& carries,
        int64_t clock_freq) {

    for (size_t buf_idx = 0; buf_idx < buffers.size(); buf_idx++) {
        auto& buf = buffers[buf_idx];
        const uint8_t* events_data = fa_.ptr(buf.file_offset);
        uint32_t ne = buf.num_events;
        uint32_t base_cycle = carries[buf_idx].first_cycle;
        int64_t base_ts = carries[buf_idx].first_ts;
        uint32_t prev_cycle = base_cycle;
        int64_t prev_ts = base_ts;

        for (uint32_t i = 0; i < ne; i++) {
            const uint8_t* ep = events_data + i * 16;
            uint32_t header, d0, d1, d2;
            memcpy(&header, ep, 4);
            memcpy(&d0, ep + 4, 4);
            memcpy(&d1, ep + 8, 4);
            memcpy(&d2, ep + 12, 4);

            int64_t ts_ns;
            if (i == 0) {
                ts_ns = base_ts;
            } else {
                int32_t diff = signed_cycle_diff(d0, prev_cycle);
                ts_ns = prev_ts + floor_div(static_cast<int64_t>(diff) * BILLION, clock_freq);
            }
            prev_cycle = d0;
            prev_ts = ts_ns;

            dispatch_event(header, d0, d1, d2, ts_ns);
        }
        stats_.total_buffers++;
    }
}

// =============================================================================
// Event dispatch
// =============================================================================

void StreamingParser::dispatch_event(uint32_t header, uint32_t d0,
                                     uint32_t d1, uint32_t d2, int64_t ts_ns) {
    stats_.total_events_processed++;
    int int_class = (header >> 10) & 0x1F;
    int int_event = header & 0x3FF;
    int cpu_id = (header >> 24) & 0x3F;
    int struct_type = (header >> 30) & 0x3;

    if (int_class == IC_PR_TH) {
        handle_pr_th(int_event, cpu_id, struct_type, d0, d1, d2, ts_ns);
    } else if (lightweight_) {
        return;
    } else if (int_class == IC_INT) {
        handle_interrupt(int_event, cpu_id, struct_type, d0, d1, d2, ts_ns);
    } else if (int_class == IC_KER_CALL) {
        handle_kercall(int_event, cpu_id, struct_type, d0, d1, d2, ts_ns);
    } else if (int_class == IC_COMM) {
        handle_comm(int_event, cpu_id, struct_type, d0, d1, d2, ts_ns);
    } else if (int_class == IC_SYSTEM) {
        handle_system(int_event, cpu_id, struct_type, d0, d1, d2, ts_ns);
    }
}

}  // namespace qst
