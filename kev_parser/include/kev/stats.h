#pragma once

#include "kev/types.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

namespace kev {

enum class EventClass : uint8_t {
    THREAD = 0, KERCALL_ENTER, KERCALL_EXIT, COMM,
    PROCESS, SYSTEM, CONTROL, INT
};

class StatsSummary {
public:
    void record(EventClass cls, int pid, int tid,
                int state_index = -1, int kercall_num = -1);
    void update_time(int64_t ts_ns);

    uint64_t total_events = 0;
    int64_t  first_time_ns = INT64_MAX;
    int64_t  last_time_ns = 0;
    RingBufferInfo ring;

    struct Counts {
        uint64_t thread        = 0;
        uint64_t kercall_enter = 0;
        uint64_t kercall_exit  = 0;
        uint64_t comm          = 0;
        uint64_t process       = 0;
        uint64_t system        = 0;
        uint64_t control       = 0;
        uint64_t interrupt     = 0;
    } counts;

    struct ProcessStats {
        std::string name;
        uint64_t event_count = 0;
        std::unordered_set<int> tids;
    };
    std::unordered_map<int, ProcessStats> per_process;

    static constexpr int MAX_STATES   = 32;
    static constexpr int MAX_KERCALLS = 128;
    uint64_t state_dist[MAX_STATES]     = {};
    uint64_t kercall_dist[MAX_KERCALLS] = {};

    struct RankedEntry { std::string name; uint64_t count; };
    std::vector<RankedEntry> top_kercalls(int n = 20) const;
    std::vector<RankedEntry> top_states() const;
};

} // namespace kev
