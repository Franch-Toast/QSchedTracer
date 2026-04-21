#include "qst/streaming_parser.h"
#include "qst/event_names.h"

#include <cstring>
#include <cstdio>

namespace qst {

// =============================================================================
// Interrupt handler
// =============================================================================

void StreamingParser::handle_interrupt(int int_event, int cpu_id,
                                       int struct_type, uint32_t d0,
                                       uint32_t d1, uint32_t d2, int64_t ts_ns) {
    if (!jw_) return;

    std::string ckey = "int_" + std::to_string(cpu_id) + "_" + std::to_string(d0);

    if (struct_type == ST_COMBINE_BEGIN) {
        combine_buf_[ckey] = {int_event, cpu_id, ts_ns, {d1, d2}};
        return;
    } else if (struct_type == ST_COMBINE_CONT) {
        auto it = combine_buf_.find(ckey);
        if (it != combine_buf_.end()) {
            it->second.data.push_back(d1);
            it->second.data.push_back(d2);
        }
        return;
    } else if (struct_type == ST_COMBINE_END) {
        auto it = combine_buf_.find(ckey);
        if (it != combine_buf_.end()) {
            it->second.data.push_back(d1);
            it->second.data.push_back(d2);
            emit_interrupt(it->second.int_event, cpu_id,
                           it->second.data, it->second.ts_ns);
            combine_buf_.erase(it);
        }
        return;
    }

    std::vector<uint32_t> data = {d1, d2};
    emit_interrupt(int_event, cpu_id, data, ts_ns);
}

void StreamingParser::emit_interrupt(int int_event, int cpu_id,
                                     const std::vector<uint32_t>& data,
                                     int64_t ts_ns) {
    int irq_num = 0;
    ArgsMap args;
    char name_buf[32];

    if (int_event == INT_ENTRY) {
        irq_num = static_cast<int>(data[0]);
        args_set_int(args, "irq", irq_num);
        args_set_int(args, "cpu", cpu_id);
        if (data.size() > 1) args_set_uint(args, "ip", data[1]);
    } else if (int_event == INT_ENTRY_64) {
        irq_num = static_cast<int>(data[0]);
        args_set_int(args, "irq", irq_num);
        args_set_int(args, "cpu", cpu_id);
        if (data.size() > 3) {
            uint64_t ip = data[2] | (static_cast<uint64_t>(data[3]) << 32);
            char hex[32]; snprintf(hex, sizeof(hex), "0x%lx", (unsigned long)ip);
            args_set_str(args, "ip", hex);
        }
    } else if (int_event == INT_HANDLER_ENTRY) {
        irq_num = data.size() > 1 ? static_cast<int>(data[1]) : static_cast<int>(data[0]);
        int pid = static_cast<int>(data[0]);
        args_set_int(args, "irq", irq_num);
        args_set_int(args, "cpu", cpu_id);
        args_set_int(args, "pid", pid);
    } else if (int_event == INT_HANDLER_ENTRY_64) {
        irq_num = data.size() > 1 ? static_cast<int>(data[1]) : static_cast<int>(data[0]);
        int pid = static_cast<int>(data[0]);
        args_set_int(args, "irq", irq_num);
        args_set_int(args, "cpu", cpu_id);
        args_set_int(args, "pid", pid);
    } else if (int_event == INT_EXIT) {
        irq_num = static_cast<int>(data[0]);
        int64_t ikey = make_key(cpu_id, irq_num);
        auto it = int_pending_.find(ikey);
        if (it != int_pending_.end() && in_time_range(ts_ns)) {
            snprintf(name_buf, sizeof(name_buf), "IRQ%d", irq_num);
            jw_->write_complete(CPU_PID, cpu_irq_tid(cpu_id),
                                it->second.ts_ns / 1000.0,
                                (ts_ns - it->second.ts_ns) / 1000.0,
                                name_buf, "irq", &it->second.args);
            stats_.interrupt_slices++;
            int_pending_.erase(it);
            return;
        }
        if (it == int_pending_.end()) {
            ArgsMap exit_args;
            args_set_int(exit_args, "irq", irq_num);
            args_set_int(exit_args, "cpu", cpu_id);
            snprintf(name_buf, sizeof(name_buf), "IRQ%d_EXIT", irq_num);
            if (in_time_range(ts_ns)) {
                emit_irq_instant(cpu_id, ts_ns, name_buf, exit_args);
                stats_.interrupt_instants++;
            }
        } else {
            int_pending_.erase(it);
        }
        return;
    } else if (int_event == INT_HANDLER_EXIT) {
        irq_num = static_cast<int>(data[0]);
        int64_t ikey = make_key(cpu_id, irq_num);
        auto it = int_pending_.find(ikey);
        if (it != int_pending_.end() && in_time_range(ts_ns)) {
            if (data.size() > 1)
                args_set_uint(it->second.args, "sigevent", data[1]);
            snprintf(name_buf, sizeof(name_buf), "IRQ%d", irq_num);
            jw_->write_complete(CPU_PID, cpu_irq_tid(cpu_id),
                                it->second.ts_ns / 1000.0,
                                (ts_ns - it->second.ts_ns) / 1000.0,
                                name_buf, "irq", &it->second.args);
            stats_.interrupt_slices++;
            int_pending_.erase(it);
            return;
        }
        if (it == int_pending_.end()) {
            ArgsMap exit_args;
            args_set_int(exit_args, "irq", irq_num);
            args_set_int(exit_args, "cpu", cpu_id);
            snprintf(name_buf, sizeof(name_buf), "IRQ%d_HANDLER_EXIT", irq_num);
            if (in_time_range(ts_ns)) {
                emit_irq_instant(cpu_id, ts_ns, name_buf, exit_args);
                stats_.interrupt_instants++;
            }
        } else {
            int_pending_.erase(it);
        }
        return;
    } else {
        return;
    }

    int64_t ikey = make_key(cpu_id, irq_num);
    int_pending_[ikey] = {ts_ns, std::move(args)};
}

void StreamingParser::emit_irq_instant(int cpu_id, int64_t ts_ns,
                                       const char* name, const ArgsMap& args) {
    jw_->write_instant(CPU_PID, cpu_irq_tid(cpu_id),
                       ts_ns / 1000.0, name, "irq", &args);
}

// =============================================================================
// KerCall handler
// =============================================================================

void StreamingParser::handle_kercall(int int_event, int cpu_id, int struct_type,
                                     uint32_t d0, uint32_t d1, uint32_t d2,
                                     int64_t ts_ns) {
    if (!jw_) return;

    if (struct_type == ST_SIMPLE) {
        emit_kercall(int_event, cpu_id, {d1, d2}, false, ts_ns);
    } else if (struct_type == ST_COMBINE_BEGIN) {
        std::string key = "ker_" + std::to_string(cpu_id) + "_" + std::to_string(d0);
        combine_buf_[key] = {int_event, cpu_id, ts_ns, {d1, d2}};
    } else if (struct_type == ST_COMBINE_CONT) {
        std::string key = "ker_" + std::to_string(cpu_id) + "_" + std::to_string(d0);
        auto it = combine_buf_.find(key);
        if (it != combine_buf_.end()) {
            it->second.data.push_back(d1);
            it->second.data.push_back(d2);
        }
    } else if (struct_type == ST_COMBINE_END) {
        std::string key = "ker_" + std::to_string(cpu_id) + "_" + std::to_string(d0);
        auto it = combine_buf_.find(key);
        if (it != combine_buf_.end()) {
            it->second.data.push_back(d1);
            it->second.data.push_back(d2);
            emit_kercall(it->second.int_event, it->second.cpu_id,
                         it->second.data, true, it->second.ts_ns);
            combine_buf_.erase(it);
        }
    }
}

void StreamingParser::emit_kercall(int int_event, int cpu_id,
                                   const std::vector<uint32_t>& data_words,
                                   bool is_wide, int64_t ts_ns) {
    int call_num = int_event & 0x7F;
    int base_event = int_event & ~0x200;
    const char* suffix;
    bool is_enter;

    if (base_event >= 2 * _TRACE_MAX_KER_CALL_NUM) {
        suffix = "_INT";
        is_enter = false;
    } else if (base_event >= _TRACE_MAX_KER_CALL_NUM) {
        suffix = "_EXIT";
        is_enter = false;
    } else {
        suffix = "_ENTER";
        is_enter = true;
    }

    bool is_64 = (int_event & 0x200) != 0;
    int os_version = reader_->header.os_version;
    const char* base_name = get_kercall_name(call_num, os_version);
    std::string name;
    if (base_name) {
        name = std::string(base_name) + suffix;
    } else {
        name = "KerCall_" + std::to_string(call_num) + suffix;
    }

    KercallFields fields;
    if (is_enter) {
        fields = get_kercall_enter_fields(call_num, is_64);
    } else {
        fields = get_kercall_exit_fields(call_num, is_64);
    }

    const char** fld = is_wide ? fields.wide : fields.fast;
    int fld_count = is_wide ? fields.wide_count : fields.fast_count;

    ArgsMap args;
    for (size_t i = 0; i < data_words.size(); i++) {
        const char* key;
        char default_key[16];
        if (static_cast<int>(i) < fld_count && fld[i]) {
            key = fld[i];
        } else {
            snprintf(default_key, sizeof(default_key), "d%zu", i);
            key = default_key;
        }
        if (strcmp(key, "empty") == 0) continue;

        std::string real_key;
        if (strcmp(key, "pid") == 0) {
            real_key = "target_pid";
        } else if (strcmp(key, "tid") == 0) {
            real_key = "target_tid";
        } else {
            real_key = key;
        }
        args_set_uint(args, real_key.c_str(), data_words[i]);
    }

    auto cit = cpu_pending_.find(cpu_id);
    int pid = cit != cpu_pending_.end() ? cit->second.pid : 0;
    int tid = cit != cpu_pending_.end() ? cit->second.tid : 0;

    track_sync_flow(call_num, is_enter, suffix, cpu_id, pid, tid, args, ts_ns);
    track_ipc_kercall(call_num, is_enter, suffix, cpu_id, pid, tid, args, ts_ns);
    track_signal_kercall(call_num, is_enter, cpu_id, pid, tid, args, ts_ns);

    args_set_int(args, "cpu", cpu_id);
    if (pid) {
        args_set_int(args, "pid", pid);
        args_set_int(args, "tid", tid);
        args_set_str(args, "process", get_process_label(pid));
        args_set_str(args, "thread", get_thread_label(pid, tid));
    }

    resolve_pid_tid_args(args);
    format_kercall_args(args, call_num);

    if (pid_tid_match(pid, tid) && in_time_range(ts_ns)) {
        if (pid && tid) {
            jw_->write_instant(pid, tid, ts_ns / 1000.0,
                               name.c_str(), "kercall", &args);
        } else {
            jw_->write_instant(CPU_PID, cpu_irq_tid(cpu_id),
                               ts_ns / 1000.0, name.c_str(), "kercall", &args);
        }
        stats_.kercall_instants++;
    }
}

// =============================================================================
// Comm handler
// =============================================================================

void StreamingParser::handle_comm(int int_event, int cpu_id, int struct_type,
                                  uint32_t d0, uint32_t d1, uint32_t d2,
                                  int64_t ts_ns) {
    if (!jw_) return;

    if (struct_type == ST_SIMPLE) {
        emit_comm(int_event, cpu_id, {d1, d2}, false, ts_ns);
    } else if (struct_type == ST_COMBINE_BEGIN) {
        std::string key = "comm_" + std::to_string(cpu_id) + "_" + std::to_string(d0);
        combine_buf_[key] = {int_event, cpu_id, ts_ns, {d1, d2}};
    } else if (struct_type == ST_COMBINE_CONT) {
        std::string key = "comm_" + std::to_string(cpu_id) + "_" + std::to_string(d0);
        auto it = combine_buf_.find(key);
        if (it != combine_buf_.end()) {
            it->second.data.push_back(d1);
            it->second.data.push_back(d2);
        }
    } else if (struct_type == ST_COMBINE_END) {
        std::string key = "comm_" + std::to_string(cpu_id) + "_" + std::to_string(d0);
        auto it = combine_buf_.find(key);
        if (it != combine_buf_.end()) {
            it->second.data.push_back(d1);
            it->second.data.push_back(d2);
            emit_comm(it->second.int_event, it->second.cpu_id,
                      it->second.data, true, it->second.ts_ns);
            combine_buf_.erase(it);
        }
    }
}

void StreamingParser::emit_comm(int int_event, int cpu_id,
                                const std::vector<uint32_t>& data_words,
                                bool is_wide, int64_t ts_ns) {
    int comm_code = int_event & 0x1F;
    const char* base_name = get_comm_name(comm_code);
    std::string name;
    if (base_name) {
        name = base_name;
    } else {
        name = "COMM_" + std::to_string(int_event);
    }

    ArgsMap args;
    if (is_wide) {
        int count = 0;
        const char** wf = get_comm_wide_fields(comm_code, &count);
        if (wf && count > 0) {
            for (size_t i = 0; i < data_words.size(); i++) {
                const char* key;
                char default_key[16];
                if (static_cast<int>(i) < count && wf[i]) {
                    key = wf[i];
                } else {
                    snprintf(default_key, sizeof(default_key), "d%zu", i);
                    key = default_key;
                }
                args_set_uint(args, key, data_words[i]);
            }
        } else {
            goto simple_fields;
        }
    } else {
        simple_fields:
        CommField cf = get_comm_field(comm_code);
        if (data_words.size() >= 2) {
            args_set_uint(args, cf.f1, data_words[0]);
            args_set_uint(args, cf.f2, data_words[1]);
        } else if (data_words.size() >= 1) {
            args_set_uint(args, "d1", data_words[0]);
        }
    }

    auto cit = cpu_pending_.find(cpu_id);
    int pid = cit != cpu_pending_.end() ? cit->second.pid : 0;
    int tid = cit != cpu_pending_.end() ? cit->second.tid : 0;
    args_set_int(args, "cpu", cpu_id);
    if (pid) {
        args_set_int(args, "pid", pid);
        args_set_int(args, "tid", tid);
    }

    track_ipc_comm(comm_code, pid, tid, args, ts_ns);
    resolve_pid_tid_args(args);
    format_general_args(args);

    if (pid_tid_match(pid, tid) && in_time_range(ts_ns)) {
        if (pid && tid) {
            jw_->write_instant(pid, tid, ts_ns / 1000.0,
                               name.c_str(), "comm", &args);
        } else {
            jw_->write_instant(CPU_PID, cpu_irq_tid(cpu_id),
                               ts_ns / 1000.0, name.c_str(), "comm", &args);
        }
        stats_.comm_instants++;
    }
}

// =============================================================================
// System handler
// =============================================================================

void StreamingParser::handle_system(int int_event, int cpu_id, int struct_type,
                                    uint32_t d0, uint32_t d1, uint32_t d2,
                                    int64_t ts_ns) {
    if (!jw_) return;

    if (struct_type == ST_SIMPLE) {
        emit_system(int_event, cpu_id, {d1, d2}, false, ts_ns);
    } else if (struct_type == ST_COMBINE_BEGIN) {
        std::string key = "sys_" + std::to_string(cpu_id) + "_" + std::to_string(d0);
        combine_buf_[key] = {int_event, cpu_id, ts_ns, {d1, d2}};
    } else if (struct_type == ST_COMBINE_CONT) {
        std::string key = "sys_" + std::to_string(cpu_id) + "_" + std::to_string(d0);
        auto it = combine_buf_.find(key);
        if (it != combine_buf_.end()) {
            it->second.data.push_back(d1);
            it->second.data.push_back(d2);
        }
    } else if (struct_type == ST_COMBINE_END) {
        std::string key = "sys_" + std::to_string(cpu_id) + "_" + std::to_string(d0);
        auto it = combine_buf_.find(key);
        if (it != combine_buf_.end()) {
            it->second.data.push_back(d1);
            it->second.data.push_back(d2);
            emit_system(it->second.int_event, it->second.cpu_id,
                        it->second.data, true, it->second.ts_ns);
            combine_buf_.erase(it);
        }
    }
}

void StreamingParser::emit_system(int int_event, int cpu_id,
                                  const std::vector<uint32_t>& data_words,
                                  bool is_wide, int64_t ts_ns) {
    int sys_code = int_event & 0x1F;
    const char* base_name = get_system_name(sys_code);
    std::string name;
    if (base_name) {
        name = base_name;
    } else {
        name = "SYSTEM_" + std::to_string(int_event);
    }

    ArgsMap args;
    args_set_int(args, "cpu", cpu_id);

    const SystemFieldLayout* sf = get_system_fields(sys_code);
    if (sf) {
        const char** fld = is_wide ? sf->wide : sf->fast;
        int fld_count = is_wide ? sf->wide_count : sf->fast_count;
        for (size_t i = 0; i < data_words.size(); i++) {
            const char* key;
            char default_key[16];
            if (static_cast<int>(i) < fld_count && fld[i]) {
                key = fld[i];
            } else {
                snprintf(default_key, sizeof(default_key), "d%zu", i);
                key = default_key;
            }
            args_set_uint(args, key, data_words[i]);
        }
    } else {
        for (size_t i = 0; i < data_words.size(); i++) {
            char key[16];
            snprintf(key, sizeof(key), "d%zu", i);
            args_set_uint(args, key, data_words[i]);
        }
    }

    resolve_pid_tid_args(args);

    if (in_time_range(ts_ns)) {
        jw_->write_instant(CPU_PID, cpu_irq_tid(cpu_id),
                           ts_ns / 1000.0, name.c_str(), "system", &args);
        stats_.system_instants++;
    }
}

}  // namespace qst
