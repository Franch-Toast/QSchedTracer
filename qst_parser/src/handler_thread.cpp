#include "qst/streaming_parser.h"

#include <cstring>
#include <cstdio>

namespace qst {

// =============================================================================
// PR_TH handler
// =============================================================================

void StreamingParser::handle_pr_th(int int_event, int cpu_id, int struct_type,
                                   uint32_t d0, uint32_t d1, uint32_t d2,
                                   int64_t ts_ns) {
    if (int_event >= 2 * MAX_TH_STATE_NUM) {
        handle_process_event(int_event, cpu_id, struct_type, d0, d1, d2, ts_ns);
        return;
    }

    bool is_vthread = int_event >= MAX_TH_STATE_NUM;
    int state_code = is_vthread ? (int_event - MAX_TH_STATE_NUM) : int_event;
    const char* state = get_thread_state(state_code);
    if (!state) {
        static char buf[32];
        snprintf(buf, sizeof(buf), "STATE_%d", state_code);
        state = buf;
    }

    if (struct_type == ST_SIMPLE) {
        emit_thread_state(ts_ns, cpu_id, static_cast<int>(d1),
                          static_cast<int>(d2), state, is_vthread, false,
                          0, 0, 0, 0);
    } else if (struct_type == ST_COMBINE_BEGIN) {
        std::string key = "th_" + std::to_string(cpu_id) + "_" + std::to_string(d0);
        auto& buf = thread_combine_[key];
        buf = {cpu_id, static_cast<int>(d1), static_cast<int>(d2),
               state, is_vthread, 0, 0, 0, 0, false, ts_ns};
    } else if (struct_type == ST_COMBINE_CONT) {
        std::string key = "th_" + std::to_string(cpu_id) + "_" + std::to_string(d0);
        auto it = thread_combine_.find(key);
        if (it != thread_combine_.end()) {
            it->second.priority = static_cast<int>(d1);
            it->second.policy = static_cast<int>(d2);
            it->second.has_cont = true;
        }
    } else if (struct_type == ST_COMBINE_END) {
        std::string key = "th_" + std::to_string(cpu_id) + "_" + std::to_string(d0);
        auto it = thread_combine_.find(key);
        if (it != thread_combine_.end()) {
            auto& buf = it->second;
            if (buf.has_cont) {
                buf.partition_id = static_cast<int>(d1);
                buf.sched_flags = static_cast<int>(d2);
            } else {
                buf.priority = static_cast<int>(d1);
                buf.policy = static_cast<int>(d2);
            }
            emit_thread_state(buf.ts_ns, buf.cpu_id, buf.pid, buf.tid,
                              buf.state, buf.is_vthread, true,
                              buf.priority, buf.policy,
                              buf.partition_id, buf.sched_flags);
            thread_combine_.erase(it);
        }
    }
}

ArgsMap StreamingParser::build_thread_args(int pid, int tid, int cpu_id,
                                           int priority, int policy,
                                           bool is_vthread, bool is_wide) const {
    ArgsMap args;
    args_set_str(args, "process", get_process_label(pid));
    args_set_str(args, "thread", get_thread_label(pid, tid));
    args_set_int(args, "pid", pid);
    args_set_int(args, "tid", tid);
    args_set_int(args, "cpu", cpu_id);
    if (is_wide) {
        args_set_int(args, "priority", priority);
        args_set_str(args, "policy", format_policy(policy));
    }
    if (is_vthread) {
        args["is_vthread"] = "true";
    }
    return args;
}

void StreamingParser::emit_thread_state(int64_t ts_ns, int cpu_id,
                                        int pid, int tid, const char* state,
                                        bool is_vthread, bool is_wide,
                                        int priority, int policy,
                                        int /*partition_id*/, int /*sched_flags*/) {
    if (strcmp(state, "CREATE") == 0 || strcmp(state, "DESTROY") == 0)
        return;

    if (!jw_) return;

    int64_t key = make_key(pid, tid);
    bool emit = pid_tid_match(pid, tid) && in_time_range(ts_ns);

    auto it = thread_pending_.find(key);
    if (it != thread_pending_.end()) {
        auto& prev = it->second;

        if (emit) {
            auto args = build_thread_args(pid, tid, prev.cpu_id,
                                          prev.priority, prev.policy,
                                          prev.is_vthread, prev.is_wide);
            jw_->write_complete(pid, tid,
                                prev.ts_ns / 1000.0,
                                (ts_ns - prev.ts_ns) / 1000.0,
                                prev.state, "thread", &args);
            stats_.thread_slices++;
        }

        check_sync_flow(prev.state, state, pid, tid, ts_ns);
        check_ipc_reply_flow(prev.state, state, pid, tid, ts_ns);
        check_signal_flow(prev.state, state, pid, tid, ts_ns);

        if (strcmp(prev.state, "RUNNING") == 0) {
            auto cit = cpu_pending_.find(prev.cpu_id);
            if (cit != cpu_pending_.end()) {
                auto& cp = cit->second;
                if (emit) {
                    auto tname = get_thread_name(cp.pid, cp.tid);
                    ArgsMap cpu_args;
                    args_set_str(cpu_args, "process", get_process_label(cp.pid));
                    args_set_str(cpu_args, "thread", get_thread_label(cp.pid, cp.tid));
                    args_set_int(cpu_args, "priority", prev.priority);
                    args_set_str(cpu_args, "policy", format_policy(prev.policy));
                    jw_->write_complete(CPU_PID, cpu_main_tid(prev.cpu_id),
                                        cp.ts_ns / 1000.0,
                                        (ts_ns - cp.ts_ns) / 1000.0,
                                        tname.c_str(), "cpu", &cpu_args);
                    stats_.cpu_running_slices++;
                }
                cpu_pending_.erase(cit);
            }
        }
    }

    thread_pending_[key] = {ts_ns, state, cpu_id, priority, policy,
                            is_vthread, is_wide};
    if (strcmp(state, "RUNNING") == 0) {
        cpu_pending_[cpu_id] = {ts_ns, pid, tid};
    }
}

// =============================================================================
// Process event handler
// =============================================================================

void StreamingParser::handle_process_event(int int_event, int cpu_id,
                                           int struct_type, uint32_t /*d0*/,
                                           uint32_t d1, uint32_t d2,
                                           int64_t ts_ns) {
    int shift = int_event >> 6;
    if (shift < 1) return;
    int ext_event = 1 << (shift - 1);
    constexpr int PROC_CREATE = 1;
    constexpr int PROC_DESTROY = 2;
    constexpr int PROCCREATE_NAME = 4;
    constexpr int PROCTHREAD_NAME = 16;

    if (ext_event == PROC_CREATE && struct_type == ST_SIMPLE) {
        emit_proc_lifecycle(ts_ns, cpu_id, static_cast<int>(d1),
                            static_cast<int>(d2), "PROC_CREATE");
        return;
    }
    if (ext_event == PROC_DESTROY && struct_type == ST_SIMPLE) {
        emit_proc_lifecycle(ts_ns, cpu_id, static_cast<int>(d1),
                            static_cast<int>(d2), "PROC_DESTROY");
        return;
    }

    if (ext_event != PROCCREATE_NAME && ext_event != PROCTHREAD_NAME)
        return;

    std::string key = "proc_" + std::to_string(cpu_id);

    if (struct_type == ST_COMBINE_BEGIN) {
        CombineBuf buf;
        buf.int_event = ext_event;
        buf.cpu_id = cpu_id;
        buf.ts_ns = ts_ns;
        buf.data = {d1, d2};
        proc_combine_[key] = std::move(buf);
    } else if (struct_type == ST_COMBINE_CONT || struct_type == ST_COMBINE_END) {
        auto it = proc_combine_.find(key);
        if (it == proc_combine_.end()) return;
        it->second.data.push_back(d1);
        it->second.data.push_back(d2);

        if (struct_type == ST_COMBINE_END) {
            auto& buf = it->second;
            std::vector<uint8_t> raw_bytes;
            for (size_t i = 2; i < buf.data.size(); i++) {
                uint8_t b[4];
                memcpy(b, &buf.data[i], 4);
                raw_bytes.insert(raw_bytes.end(), b, b + 4);
            }

            size_t len = 0;
            while (len < raw_bytes.size() && raw_bytes[len] != 0) len++;
            std::string name(raw_bytes.begin(), raw_bytes.begin() + len);
            std::string clean;
            for (char c : name) {
                if (c >= ' ' || c == '\t') clean += c;
            }

            if (buf.int_event == PROCCREATE_NAME) {
                int pid = static_cast<int>(buf.data[1]);
                reader_->process_names[pid] = clean;
                if (jw_) jw_->write_process_name(pid, clean);
            } else {
                int pid = static_cast<int>(buf.data[0]);
                int tid = static_cast<int>(buf.data[1]);
                reader_->thread_names[{pid, tid}] = clean;
                if (jw_) jw_->write_thread_name(pid, tid, clean);
            }
            proc_combine_.erase(it);
        }
    }
}

void StreamingParser::emit_proc_lifecycle(int64_t ts_ns, int cpu_id,
                                          int ppid, int pid, const char* name) {
    if (lightweight_) return;
    if (!in_time_range(ts_ns)) return;
    if (filter_pids_ && !pid_tid_match(ppid, 0) && !pid_tid_match(pid, 0))
        return;
    if (!jw_) return;

    ArgsMap args;
    args_set_int(args, "ppid", ppid);
    args_set_int(args, "pid", pid);
    args_set_str(args, "parent", get_process_label(ppid));
    args_set_str(args, "process", get_process_label(pid));
    args_set_int(args, "cpu", cpu_id);
    jw_->write_instant(ppid, 1, ts_ns / 1000.0, name, "process", &args);
}

// =============================================================================
// Flush pending slices
// =============================================================================

void StreamingParser::flush_pending_slices() {
    if (!jw_) return;

    int64_t end_ns = reader_->header.capture_end_ns;

    for (auto& [key, prev] : thread_pending_) {
        if (prev.ts_ns >= end_ns) continue;
        int pid = static_cast<int>(key >> 32);
        int tid = static_cast<int>(key & 0xFFFFFFFF);
        if (!pid_tid_match(pid, tid)) continue;
        if (!in_time_range(end_ns)) continue;

        auto args = build_thread_args(pid, tid, prev.cpu_id,
                                      prev.priority, prev.policy,
                                      prev.is_vthread, prev.is_wide);
        jw_->write_complete(pid, tid,
                            prev.ts_ns / 1000.0,
                            (end_ns - prev.ts_ns) / 1000.0,
                            prev.state, "thread", &args);
        stats_.thread_slices++;
    }

    for (auto& [cpu_id, cp] : cpu_pending_) {
        if (cp.ts_ns >= end_ns) continue;
        if (!pid_tid_match(cp.pid, cp.tid)) continue;
        if (!in_time_range(end_ns)) continue;

        auto tname = get_thread_name(cp.pid, cp.tid);
        ArgsMap cpu_args;
        args_set_str(cpu_args, "process", get_process_label(cp.pid));
        args_set_str(cpu_args, "thread", get_thread_label(cp.pid, cp.tid));
        jw_->write_complete(CPU_PID, cpu_main_tid(cpu_id),
                            cp.ts_ns / 1000.0,
                            (end_ns - cp.ts_ns) / 1000.0,
                            tname.c_str(), "cpu", &cpu_args);
        stats_.cpu_running_slices++;
    }
}

}  // namespace qst
