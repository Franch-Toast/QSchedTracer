#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>

namespace kev {

struct EventFilter {
    int64_t time_start_ns = -1;
    int64_t time_end_ns   = -1;
    std::unordered_set<int> pids;
    std::unordered_set<int> tids;
    std::unordered_set<std::string> classes;

    bool passes(int64_t ts_ns, int pid, int tid) const;
    bool passes_class(const char* cls) const;
    bool has_filters() const;
};

} // namespace kev
