#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

namespace kev {

struct KeyValue {
    std::string key;
    std::string value;
};

using KVList = std::vector<KeyValue>;

struct RingBufferInfo {
    bool     gap_detected         = false;
    int64_t  gap_threshold_ns     = 1000000000LL;
    int64_t  real_end_trace_ns    = 0;
    uint64_t post_collection_count = 0;
    std::string capture_wallclock_start;
    std::string capture_wallclock_end;
    int64_t  wall_offset_ns       = 0;
    bool     has_wall_clock       = false;
};

struct Metadata {
    std::string qnx_version;
    std::string machine;
    std::string trace_date;
    std::string boot_date;
    unsigned    cpu_count       = 0;
    uint64_t    cycles_per_sec  = 0;
    std::string trace_file;
    std::string tracelogger_args;
    int64_t     trace_start_ns  = 0;
    int64_t     trace_end_ns    = 0;
    RingBufferInfo ring;
};

} // namespace kev
