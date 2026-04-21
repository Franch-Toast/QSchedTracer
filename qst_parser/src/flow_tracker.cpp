#include "qst/streaming_parser.h"

#include <cstring>
#include <set>

namespace qst {

// =============================================================================
// Sync flow tracking
// =============================================================================

static const std::set<int> SYNC_WAIT_CALLS = {80, 82, 85};
static const std::set<int> SYNC_RELEASE_CALLS = {81, 83, 84};

void StreamingParser::track_sync_flow(int call_num, bool is_enter,
                                      const char* suffix, int cpu_id,
                                      int pid, int tid,
                                      const ArgsMap& raw_args, int64_t ts_ns) {
    if (!is_enter) {
        if (strcmp(suffix, "_EXIT") == 0)
            active_sync_release_.erase(cpu_id);
        return;
    }
    if (!pid) return;

    uint64_t sync_ptr = 0;
    auto spit = raw_args.find("sync_p");
    if (spit != raw_args.end()) {
        sync_ptr = args_get_uint64(raw_args, "sync_p");
    } else {
        uint64_t lo = args_get_uint64(raw_args, "sync_p_lo");
        uint64_t hi = args_get_uint64(raw_args, "sync_p_hi");
        if (lo) sync_ptr = (hi << 32) | (lo & 0xFFFFFFFF);
    }
    if (!sync_ptr) return;

    if (SYNC_WAIT_CALLS.count(call_num)) {
        thread_sync_wait_[make_key(pid, tid)] = {sync_ptr, ts_ns};
    } else if (SYNC_RELEASE_CALLS.count(call_num)) {
        active_sync_release_[cpu_id] = {sync_ptr, ts_ns, pid, tid};
    }
}

void StreamingParser::check_sync_flow(const char* prev_state,
                                      const char* new_state,
                                      int pid, int tid, int64_t ts_ns) {
    const char* sync_type = nullptr;
    if (strcmp(prev_state, "MUTEX") == 0) sync_type = "mutex";
    else if (strcmp(prev_state, "CONDVAR") == 0) sync_type = "condvar";
    else if (strcmp(prev_state, "SEM") == 0) sync_type = "sem";

    if (!sync_type || strcmp(new_state, "READY") != 0) return;

    int64_t key = make_key(pid, tid);
    auto wit = thread_sync_wait_.find(key);
    if (wit == thread_sync_wait_.end()) return;

    auto [sync_ptr, wait_ts] = wit->second;
    thread_sync_wait_.erase(wit);

    const std::tuple<uint64_t, int64_t, int, int>* rel = nullptr;
    for (auto& [_, ri] : active_sync_release_) {
        if (std::get<0>(ri) == sync_ptr) {
            rel = &ri;
            break;
        }
    }
    if (!rel) return;

    auto [_, rel_ts, rel_pid, rel_tid] = *rel;
    if (!in_time_range(ts_ns)) return;
    if (filter_pids_ && !pid_tid_match(pid, tid) && !pid_tid_match(rel_pid, rel_tid))
        return;

    flow_id_++;
    std::string flow_name = std::string("sync_") + sync_type;
    if (jw_) {
        jw_->write_flow_start(rel_pid, rel_tid, rel_ts / 1000.0,
                              flow_id_, flow_name.c_str(), "sync");
        jw_->write_flow_end(pid, tid, ts_ns / 1000.0,
                            flow_id_, flow_name.c_str(), "sync");
    }
    stats_.sync_flows++;
}

// =============================================================================
// IPC flow tracking
// =============================================================================

static const std::set<int> IPC_SEND_CALLS = {11, 12};
constexpr int IPC_RECV_CALL = 14;

void StreamingParser::track_ipc_kercall(int call_num, bool is_enter,
                                        const char* suffix, int /*cpu_id*/,
                                        int pid, int tid,
                                        const ArgsMap& raw_args, int64_t ts_ns) {
    if (!pid) return;

    if (is_enter && IPC_SEND_CALLS.count(call_num)) {
        ipc_send_pending_[make_key(pid, tid)] = ts_ns;
    } else if (!is_enter && strcmp(suffix, "_EXIT") == 0 && call_num == IPC_RECV_CALL) {
        int sender_pid = args_get_int(raw_args, "info_pid");
        int sender_tid = args_get_int(raw_args, "info_tid");
        if (!sender_pid || !sender_tid) return;

        auto sit = ipc_send_pending_.find(make_key(sender_pid, sender_tid));
        if (sit == ipc_send_pending_.end()) return;
        int64_t send_ts = sit->second;
        ipc_send_pending_.erase(sit);

        if (!in_time_range(ts_ns)) return;
        if (filter_pids_ && !pid_tid_match(pid, tid) &&
            !pid_tid_match(sender_pid, sender_tid)) return;

        if (jw_) {
            flow_id_++;
            jw_->write_flow_start(sender_pid, sender_tid, send_ts / 1000.0,
                                  flow_id_, "ipc_msg", "ipc");
            jw_->write_flow_end(pid, tid, ts_ns / 1000.0,
                                flow_id_, "ipc_msg", "ipc");
            stats_.ipc_msg_flows++;

            flow_id_++;
            jw_->write_flow_start(pid, tid, ts_ns / 1000.0,
                                  flow_id_, "ipc_send_ack", "ipc");
            jw_->write_flow_end(sender_pid, sender_tid, ts_ns / 1000.0,
                                flow_id_, "ipc_send_ack", "ipc");
            stats_.ipc_send_ack_flows++;
        }
    }
}

void StreamingParser::track_ipc_comm(int comm_code, int pid, int tid,
                                     const ArgsMap& raw_args, int64_t ts_ns) {
    if (comm_code != 10) return;  // REPLY
    if (!pid) return;

    int target_pid = args_get_int(raw_args, "target_pid");
    int target_tid = args_get_int(raw_args, "target_tid");
    if (target_pid && target_tid) {
        ipc_reply_pending_[make_key(target_pid, target_tid)] = {pid, tid, ts_ns};
    }
}

void StreamingParser::check_ipc_reply_flow(const char* prev_state,
                                           const char* new_state,
                                           int pid, int tid, int64_t ts_ns) {
    if (strcmp(prev_state, "REPLY") != 0 || strcmp(new_state, "READY") != 0)
        return;

    int64_t key = make_key(pid, tid);
    auto it = ipc_reply_pending_.find(key);
    if (it == ipc_reply_pending_.end()) return;

    auto [server_pid, server_tid, reply_ts] = it->second;
    ipc_reply_pending_.erase(it);

    if (!in_time_range(ts_ns)) return;
    if (filter_pids_ && !pid_tid_match(pid, tid) &&
        !pid_tid_match(server_pid, server_tid)) return;

    flow_id_++;
    if (jw_) {
        jw_->write_flow_start(server_pid, server_tid, reply_ts / 1000.0,
                              flow_id_, "ipc_reply", "ipc");
        jw_->write_flow_end(pid, tid, ts_ns / 1000.0,
                            flow_id_, "ipc_reply", "ipc");
    }
    stats_.ipc_reply_flows++;
}

// =============================================================================
// Signal flow tracking
// =============================================================================

static const std::set<int> SIGNAL_KILL_CALLS = {26, 33};

void StreamingParser::track_signal_kercall(int call_num, bool is_enter,
                                           int /*cpu_id*/,
                                           int pid, int tid,
                                           const ArgsMap& raw_args,
                                           int64_t ts_ns) {
    if (!is_enter || !SIGNAL_KILL_CALLS.count(call_num)) return;
    if (!pid) return;

    int target_pid = args_get_int(raw_args, "target_pid");
    if (target_pid == 0) target_pid = pid;
    int target_tid = args_get_int(raw_args, "target_tid");
    signal_pending_[target_pid] = {pid, tid, ts_ns, target_tid};
}

void StreamingParser::check_signal_flow(const char* prev_state,
                                        const char* new_state,
                                        int pid, int tid, int64_t ts_ns) {
    if ((strcmp(prev_state, "SIGWAITINFO") != 0 &&
         strcmp(prev_state, "SIGSUSPEND") != 0) ||
        strcmp(new_state, "READY") != 0)
        return;

    auto it = signal_pending_.find(pid);
    if (it == signal_pending_.end()) return;

    auto [sender_pid, sender_tid, signal_ts, target_tid] = it->second;
    signal_pending_.erase(it);

    if (target_tid != 0 && target_tid != tid) return;
    if (!in_time_range(ts_ns)) return;
    if (filter_pids_ && !pid_tid_match(pid, tid) &&
        !pid_tid_match(sender_pid, sender_tid)) return;

    flow_id_++;
    if (jw_) {
        jw_->write_flow_start(sender_pid, sender_tid, signal_ts / 1000.0,
                              flow_id_, "signal", "signal");
        jw_->write_flow_end(pid, tid, ts_ns / 1000.0,
                            flow_id_, "signal", "signal");
    }
    stats_.signal_flows++;
}

}  // namespace qst
