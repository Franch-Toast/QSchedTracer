/**
 * @file core/data_buffer.hpp
 * @brief QSchedTracer - 追踪元数据存储
 *
 * Ring Mode 下，主追踪数据由内核 ring buffer 通过 mmap 直接写入文件。
 * 此类仅存储 clockFreq、wallclock 等文件头需要的元数据。
 */

#pragma once

#include "qst/types.hpp"
#include <atomic>
#include <cstdint>

namespace qst {

class DataBuffer {
public:
    DataBuffer() = default;
    ~DataBuffer() = default;

    DataBuffer(const DataBuffer&) = delete;
    DataBuffer& operator=(const DataBuffer&) = delete;

    State state() const { return state_.load(); }
    void setState(State s) { state_.store(s); }

    uint64_t clockFreq() const { return clock_freq_; }
    void setClockFreq(uint64_t freq) { clock_freq_ = freq; }

    int64_t wallclockSec() const { return wallclock_sec_; }
    void setWallclockSec(int64_t sec) { wallclock_sec_ = sec; }

    int64_t wallclockNsec() const { return wallclock_nsec_; }
    void setWallclockNsec(int64_t nsec) { wallclock_nsec_ = nsec; }

private:
    std::atomic<State> state_{State::Idle};
    uint64_t clock_freq_{0};
    int64_t wallclock_sec_{0};
    int64_t wallclock_nsec_{0};
};

}  // namespace qst
