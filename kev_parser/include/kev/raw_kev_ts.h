#pragma once

#include <cstdint>
#include <cstddef>

namespace kev {

class RawKevTimestamps {
public:
    struct Config {
        uint64_t cycles_per_sec = 0;
        int64_t  wall_end_ns    = 0;
        bool     verbose        = false;
    };

    struct ScanResult {
        bool     success              = false;
        size_t   total_logical_events = 0;
        size_t   total_raw_events     = 0;
        int64_t  first_event_ns       = 0;
        int64_t  real_end_ns          = 0;
        bool     gap_detected         = false;
        size_t   post_collection_count = 0;
    };

    ScanResult build(const char* filename, const Config& cfg);

    int64_t  lookup(uint32_t event_d0) const;

    int64_t  real_end_ns() const { return real_end_ns_; }
    bool     gap_detected() const { return gap_detected_; }
    bool     is_post_collection(uint32_t event_d0) const;

private:
    uint32_t anchor_d0_       = 0;
    int64_t  anchor_ns_       = 0;
    int64_t  ns_per_cycle_    = 0;   // fixed-point: 1e9 / cps (integer division residual handled)
    uint64_t cps_             = 0;
    bool     gap_detected_    = false;
    int64_t  real_end_ns_     = 0;
    uint32_t gap_d0_          = 0;   // first post-collection d0
    uint32_t first_d0_        = 0;
};

} // namespace kev
