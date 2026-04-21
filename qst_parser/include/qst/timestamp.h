#pragma once

#include <cstdint>

namespace qst {

constexpr uint32_t MAX_U32  = 0xFFFFFFFF;
constexpr uint32_t HALF_U32 = 0x7FFFFFFF;
constexpr int64_t  BILLION  = 1'000'000'000LL;

inline int32_t signed_cycle_diff(uint32_t a, uint32_t b) {
    uint32_t raw = a - b;
    return static_cast<int32_t>(raw);
}

inline bool cycle_gt(uint32_t a, uint32_t b) {
    int32_t d = signed_cycle_diff(a, b);
    return d > 0;
}

// Python-style floor division (rounds toward -inf, not toward zero)
inline int64_t floor_div(int64_t a, int64_t b) {
    return a / b - (a % b != 0 && ((a ^ b) < 0));
}

inline int64_t cycles_to_ns(int64_t cycles, int64_t clock_freq) {
    if (clock_freq == 0) return 0;
    return floor_div(cycles * BILLION, clock_freq);
}

inline int64_t delta_to_ns(uint32_t cur_cycle, uint32_t prev_cycle,
                           int64_t clock_freq) {
    int32_t diff = signed_cycle_diff(cur_cycle, prev_cycle);
    return static_cast<int64_t>(diff) * BILLION / clock_freq;
}

}  // namespace qst
