#include "kev/raw_kev_ts.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cinttypes>
#include <vector>

namespace kev {

static inline int32_t signed_cycle_diff(uint32_t a, uint32_t b) {
    return static_cast<int32_t>(a - b);
}

static constexpr size_t  CHUNK_EVENTS    = 65536;
static constexpr int64_t GAP_THRESHOLD_CYCLES = 19200000LL; // 1 second at 19.2MHz

RawKevTimestamps::ScanResult RawKevTimestamps::build(
        const char* filename, const Config& cfg)
{
    ScanResult r{};

    FILE* f = std::fopen(filename, "rb");
    if (!f) return r;

    char hdr_buf[8192];
    size_t hdr_read = std::fread(hdr_buf, 1, sizeof(hdr_buf), f);
    hdr_buf[hdr_read < sizeof(hdr_buf) ? hdr_read : sizeof(hdr_buf) - 1] = '\0';

    const char* end_marker = "TRACE_HEADER_END::";
    const char* found = std::strstr(hdr_buf, end_marker);
    if (!found) { std::fclose(f); return r; }
    size_t header_end = static_cast<size_t>(found - hdr_buf)
                      + std::strlen(end_marker);

    const char* sp_marker = "TRACE_SYSPAGE_LEN::";
    const char* sp = std::strstr(hdr_buf, sp_marker);
    size_t syspage_len = sp
        ? std::strtoul(sp + std::strlen(sp_marker), nullptr, 10) : 0;

    uint64_t cps = cfg.cycles_per_sec;
    if (cps == 0) {
        const char* cm = "TRACE_CYCLES_PER_SEC::";
        const char* cs = std::strstr(hdr_buf, cm);
        if (cs) cps = std::strtoull(cs + std::strlen(cm), nullptr, 10);
    }
    if (cps == 0) { std::fclose(f); return r; }
    cps_ = cps;

    size_t events_start = header_end + syspage_len;
    std::fseek(f, static_cast<long>(events_start), SEEK_SET);

    /* ---- scan all logical events, collecting d0 values ---- */
    struct LogEvent { uint32_t d0; };
    std::vector<LogEvent> all_events;
    all_events.reserve(16000000);

    uint32_t chunk[CHUNK_EVENTS * 4];
    size_t total_raw = 0;

    while (true) {
        size_t n = std::fread(chunk, 16, CHUNK_EVENTS, f);
        if (n == 0) break;

        for (size_t i = 0; i < n; ++i) {
            uint32_t header = chunk[i * 4];
            uint32_t d0     = chunk[i * 4 + 1];
            int cls = (header >> 10) & 0x1F;
            uint32_t struct_type = header & (0x3u << 30);

            ++total_raw;

            if (cls == 1) continue;                         // CONTROL events
            if (struct_type == 0x80000000u ||
                struct_type == 0xC0000000u) continue;       // continuation/end

            all_events.push_back({d0});
        }
    }
    std::fclose(f);

    if (all_events.empty()) return r;

    r.total_raw_events     = total_raw;
    r.total_logical_events = all_events.size();

    /* ---- compute global relative timestamps via telescoping sum ---- */
    first_d0_ = all_events[0].d0;
    uint32_t prev_d0 = first_d0_;

    int64_t cum_cycles = 0;
    int64_t max_gap_cycles = 0;
    size_t  max_gap_idx    = 0;

    for (size_t i = 1; i < all_events.size(); ++i) {
        int32_t delta = signed_cycle_diff(all_events[i].d0, prev_d0);
        cum_cycles += delta;

        if (delta > max_gap_cycles) {
            max_gap_cycles = delta;
            max_gap_idx    = i;
        }
        prev_d0 = all_events[i].d0;
    }

    /* ---- gap detection ---- */
    int64_t gap_threshold = static_cast<int64_t>(cps);  // 1 second
    size_t last_real_idx = all_events.size() - 1;

    if (max_gap_cycles > gap_threshold && max_gap_idx > 0) {
        gap_detected_ = true;
        last_real_idx = max_gap_idx - 1;
        gap_d0_ = all_events[max_gap_idx].d0;
        r.gap_detected          = true;
        r.post_collection_count = all_events.size() - max_gap_idx;
    }

    /* ---- anchor: last real event's d0 = wall_end_ns ---- */
    anchor_d0_ = all_events[last_real_idx].d0;
    anchor_ns_ = cfg.wall_end_ns;

    /* ---- compute first and last timestamps ---- */
    int32_t first_delta = signed_cycle_diff(first_d0_, anchor_d0_);
    r.first_event_ns = anchor_ns_
        + static_cast<int64_t>(first_delta) * 1000000000LL
          / static_cast<int64_t>(cps);
    r.real_end_ns    = anchor_ns_;
    real_end_ns_     = anchor_ns_;
    r.success        = true;

    if (cfg.verbose) {
        std::fprintf(stderr, "[RawKevTs] %zu logical events from %zu raw records\n",
                     all_events.size(), total_raw);
        std::fprintf(stderr, "[RawKevTs] anchor_d0=0x%08x  first_d0=0x%08x\n",
                     anchor_d0_, first_d0_);
        std::fprintf(stderr, "[RawKevTs] first=%" PRId64 " ns, real_end=%" PRId64 " ns\n",
                     r.first_event_ns, r.real_end_ns);
        std::fprintf(stderr, "[RawKevTs] duration=%.3f s\n",
                     (double)(r.real_end_ns - r.first_event_ns) / 1e9);
        if (gap_detected_)
            std::fprintf(stderr, "[RawKevTs] gap=%.3f s at event %zu, "
                         "%zu post-collection events\n",
                         (double)max_gap_cycles / cps,
                         max_gap_idx, r.post_collection_count);
    }

    return r;
}

int64_t RawKevTimestamps::lookup(uint32_t event_d0) const {
    int32_t delta = signed_cycle_diff(event_d0, anchor_d0_);
    return anchor_ns_
        + static_cast<int64_t>(delta) * 1000000000LL
          / static_cast<int64_t>(cps_);
}

bool RawKevTimestamps::is_post_collection(uint32_t event_d0) const {
    if (!gap_detected_) return false;
    int32_t delta_from_anchor = signed_cycle_diff(event_d0, anchor_d0_);
    return delta_from_anchor > static_cast<int32_t>(cps_); // > 1 second past anchor
}

} // namespace kev
