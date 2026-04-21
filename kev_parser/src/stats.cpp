#include "kev/stats.h"
#include "kev/event_names.h"
#include <cstdio>

namespace kev {

void StatsSummary::record(EventClass cls, int pid, int tid,
                          int state_index, int kercall_num) {
    ++total_events;

    switch (cls) {
        case EventClass::THREAD:        ++counts.thread;        break;
        case EventClass::KERCALL_ENTER: ++counts.kercall_enter; break;
        case EventClass::KERCALL_EXIT:  ++counts.kercall_exit;  break;
        case EventClass::COMM:          ++counts.comm;          break;
        case EventClass::PROCESS:       ++counts.process;       break;
        case EventClass::SYSTEM:        ++counts.system;        break;
        case EventClass::CONTROL:       ++counts.control;       break;
        case EventClass::INT:           ++counts.interrupt;     break;
    }

    if (pid > 0) {
        auto& ps = per_process[pid];
        ++ps.event_count;
        if (tid > 0) ps.tids.insert(tid);
    }

    if (state_index >= 0 && state_index < MAX_STATES)
        ++state_dist[state_index];
    else if (cls == EventClass::THREAD)
        ++state_dist[MAX_STATES - 1];

    if (kercall_num >= 0 && kercall_num < MAX_KERCALLS)
        ++kercall_dist[kercall_num];
}

void StatsSummary::update_time(int64_t ts_ns) {
    if (ts_ns > 0) {
        if (ts_ns < first_time_ns) first_time_ns = ts_ns;
        if (ts_ns > last_time_ns)  last_time_ns = ts_ns;
    }
}

std::vector<StatsSummary::RankedEntry> StatsSummary::top_kercalls(int n) const {
    std::vector<RankedEntry> result;
    for (int i = 0; i < MAX_KERCALLS; ++i) {
        if (kercall_dist[i] > 0) {
            const char* name = event_names::kercall_name(i);
            if (name)
                result.push_back({name, kercall_dist[i]});
            else {
                char tmp[16];
                std::snprintf(tmp, sizeof(tmp), "KER_%d", i);
                result.push_back({tmp, kercall_dist[i]});
            }
        }
    }
    std::sort(result.begin(), result.end(),
              [](auto& a, auto& b) { return a.count > b.count; });
    if (static_cast<int>(result.size()) > n) result.resize(n);
    return result;
}

std::vector<StatsSummary::RankedEntry> StatsSummary::top_states() const {
    std::vector<RankedEntry> result;
    for (int i = 0; i < MAX_STATES; ++i) {
        if (state_dist[i] > 0) {
            const char* name = (i == MAX_STATES - 1)
                ? "UNKNOWN" : event_names::thread_state_name(i);
            if (name)
                result.push_back({name, state_dist[i]});
        }
    }
    std::sort(result.begin(), result.end(),
              [](auto& a, auto& b) { return a.count > b.count; });
    return result;
}

} // namespace kev
