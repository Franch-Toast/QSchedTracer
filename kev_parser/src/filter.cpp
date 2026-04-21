#include "kev/filter.h"

namespace kev {

bool EventFilter::passes(int64_t ts_ns, int pid, int tid) const {
    if (time_start_ns >= 0 && ts_ns < time_start_ns) return false;
    if (time_end_ns >= 0   && ts_ns > time_end_ns)   return false;
    if (!pids.empty() && pids.find(pid) == pids.end()) return false;
    if (!tids.empty() && tids.find(tid) == tids.end()) return false;
    return true;
}

bool EventFilter::passes_class(const char* cls) const {
    if (classes.empty()) return true;
    return classes.count(cls) > 0;
}

bool EventFilter::has_filters() const {
    return time_start_ns >= 0 || time_end_ns >= 0
        || !pids.empty() || !tids.empty() || !classes.empty();
}

} // namespace kev
