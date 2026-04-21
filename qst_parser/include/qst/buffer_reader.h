#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <algorithm>

#include "qst/constants.h"
#include "qst/file_access.h"

namespace qst {

struct QstFileHeader {
    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t header_size = 0;
    uint32_t flags = 0;
    uint64_t clock_freq = 0;
    int64_t  capture_start_ns = 0;
    int64_t  capture_end_ns = 0;
    uint32_t num_cpus = 0;
    uint32_t os_version = 0;
    uint32_t tracebuf_size = 0;
    uint32_t data_offset = 0;
    uint32_t bufs_per_cpu = 0;

    void parse(const uint8_t* data) {
        memcpy(&magic, data, 4);
        memcpy(&version, data + 4, 4);
        memcpy(&header_size, data + 8, 4);
        memcpy(&flags, data + 12, 4);
        memcpy(&clock_freq, data + 16, 8);
        memcpy(&capture_start_ns, data + 24, 8);
        memcpy(&capture_end_ns, data + 32, 8);
        memcpy(&num_cpus, data + 40, 4);
        memcpy(&os_version, data + 44, 4);
        memcpy(&tracebuf_size, data + 48, 4);
        memcpy(&data_offset, data + 52, 4);
        memcpy(&bufs_per_cpu, data + 56, 4);

        if (magic != QST_MAGIC)
            throw std::runtime_error("Invalid QST magic");
        if (version != QST_VERSION)
            throw std::runtime_error("Unsupported QST version");
    }
};

struct SectionHeader {
    uint32_t magic = 0;
    uint32_t version = 0;
    uint64_t payload_size = 0;
    uint32_t entry_count = 0;

    void parse(const uint8_t* data, uint32_t expected_magic = 0) {
        memcpy(&magic, data, 4);
        memcpy(&version, data + 4, 4);
        memcpy(&payload_size, data + 8, 8);
        memcpy(&entry_count, data + 16, 4);
        if (expected_magic && magic != expected_magic)
            throw std::runtime_error("Section magic mismatch");
    }
};

struct BufferMeta {
    uint32_t seq;
    uint32_t num_events;
    size_t file_offset;  // offset to event data
};

struct PairHash {
    size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int64_t>()(
            (static_cast<int64_t>(p.first) << 32) | static_cast<uint32_t>(p.second));
    }
};

class BufferReader {
public:
    QstFileHeader header;
    uint32_t data_entry_count = 0;
    std::vector<BufferMeta> sorted_buffers;
    std::unordered_map<int, std::string> process_names;
    std::unordered_map<std::pair<int, int>, std::string, PairHash> thread_names;
    std::vector<std::vector<BufferMeta>> cpu_groups;

    explicit BufferReader(FileAccess& fa, bool verbose = false)
        : fa_(fa), verbose_(verbose) {}

    void read_structure() {
        read_file_header();
        scan_data_metadata();
        read_pinf();
        sort_buffers();
    }

    std::vector<std::vector<BufferMeta>>& get_cpu_groups() {
        if (!cpu_groups.empty()) return cpu_groups;
        cpu_groups.push_back(sorted_buffers);
        return cpu_groups;
    }

private:
    FileAccess& fa_;
    bool verbose_;
    size_t data_payload_offset_ = 0;
    uint32_t tb_size_ = 0;
    uint32_t data_off_ = 0;

    struct CombineBuf {
        std::string type;
        int pid = 0;
        int tid = 0;
        std::vector<uint8_t> name_data;
    };
    std::unordered_map<std::string, CombineBuf> pinf_combine_;

    void read_file_header() {
        header.parse(fa_.ptr(0));
        tb_size_ = header.tracebuf_size;
        data_off_ = header.data_offset;
        if (tb_size_ == 0 || data_off_ == 0)
            throw std::runtime_error("Invalid tracebuf layout (old v4?)");
    }

    void scan_data_metadata() {
        size_t pos = FILE_HEADER_SIZE;
        SectionHeader sh;
        sh.parse(fa_.ptr(pos), DATA_MAGIC);
        data_entry_count = sh.entry_count;
        data_payload_offset_ = pos + SECTION_HEADER_SIZE;

        sorted_buffers.reserve(data_entry_count);
        for (uint32_t i = 0; i < data_entry_count; i++) {
            size_t block_start = data_payload_offset_ + static_cast<size_t>(i) * tb_size_;
            uint32_t ne = fa_.u32_at(block_start + TRACEBUF_OFFSET_NUM_EVENTS);
            uint32_t seq = fa_.u32_at(block_start + TRACEBUF_OFFSET_SEQ);
            if (ne == 0) continue;
            size_t ev_offset = block_start + data_off_;
            sorted_buffers.push_back({seq, ne, ev_offset});
        }
    }

    void read_pinf() {
        size_t pinf_offset = data_payload_offset_ +
                             static_cast<size_t>(data_entry_count) * tb_size_;
        SectionHeader sh;
        sh.parse(fa_.ptr(pinf_offset), PINF_MAGIC);

        size_t payload_start = pinf_offset + SECTION_HEADER_SIZE;
        for (uint32_t i = 0; i < sh.entry_count; i++) {
            size_t block_start = payload_start + static_cast<size_t>(i) * tb_size_;
            uint32_t ne = fa_.u32_at(block_start + TRACEBUF_OFFSET_NUM_EVENTS);
            if (ne == 0) continue;
            size_t ev_base = block_start + data_off_;
            const uint8_t* ev_data = fa_.ptr(ev_base);

            for (uint32_t j = 0; j < ne; j++) {
                const uint8_t* ep = ev_data + j * 16;
                uint32_t hdr, d0, d1, d2;
                memcpy(&hdr, ep, 4);
                memcpy(&d0, ep + 4, 4);
                memcpy(&d1, ep + 8, 4);
                memcpy(&d2, ep + 12, 4);

                int int_class = (hdr >> 10) & 0x1F;
                if (int_class != IC_PR_TH) continue;
                int int_event = hdr & 0x3FF;
                int struct_type = (hdr >> 30) & 0x3;

                if (int_event >= 2 * MAX_TH_STATE_NUM) {
                    extract_process_name(hdr, d0, d1, d2, struct_type);
                }
            }
        }
    }

    void extract_process_name(uint32_t header, uint32_t /*d0*/,
                              uint32_t d1, uint32_t d2, int struct_type) {
        int int_event = header & 0x3FF;
        int cpu_id = (header >> 24) & 0x3F;
        int shift = int_event >> 6;
        if (shift < 1) return;
        int ext_event = 1 << (shift - 1);

        constexpr int PROCCREATE_NAME = 4;
        constexpr int PROCTHREAD_NAME = 16;
        if (ext_event != PROCCREATE_NAME && ext_event != PROCTHREAD_NAME) return;

        std::string key = "pinf_" + std::to_string(cpu_id);

        if (struct_type == ST_COMBINE_BEGIN) {
            CombineBuf buf;
            if (ext_event == PROCCREATE_NAME) {
                buf.type = "proc";
                buf.pid = static_cast<int>(d2);
            } else {
                buf.type = "thread";
                buf.pid = static_cast<int>(d1);
                buf.tid = static_cast<int>(d2);
            }
            pinf_combine_[key] = std::move(buf);
        } else if (struct_type == ST_COMBINE_CONT || struct_type == ST_COMBINE_END) {
            auto it = pinf_combine_.find(key);
            if (it == pinf_combine_.end()) return;

            uint8_t bytes[8];
            memcpy(bytes, &d1, 4);
            memcpy(bytes + 4, &d2, 4);
            it->second.name_data.insert(it->second.name_data.end(),
                                        bytes, bytes + 8);

            if (struct_type == ST_COMBINE_END) {
                auto& buf = it->second;
                auto& raw = buf.name_data;
                size_t len = 0;
                while (len < raw.size() && raw[len] != 0) len++;
                std::string name(raw.begin(), raw.begin() + len);
                // Strip non-printable chars
                std::string clean;
                for (char c : name) {
                    if (c >= ' ' || c == '\t') clean += c;
                }

                if (buf.type == "proc") {
                    process_names[buf.pid] = clean;
                } else {
                    thread_names[{buf.pid, buf.tid}] = clean;
                }
                pinf_combine_.erase(it);
            }
        }
    }

    void sort_buffers() {
        if (header.os_version == 800 && header.bufs_per_cpu > 0) {
            sort_per_cpu();
        } else {
            std::sort(sorted_buffers.begin(), sorted_buffers.end(),
                      [](const BufferMeta& a, const BufferMeta& b) {
                          return a.seq < b.seq;
                      });
        }
    }

    void sort_per_cpu() {
        uint32_t K = header.bufs_per_cpu;
        uint32_t num_cpus = header.num_cpus;
        cpu_groups.resize(num_cpus);

        for (auto& buf : sorted_buffers) {
            size_t rel_idx = (buf.file_offset - data_payload_offset_ -
                              data_off_) / tb_size_;
            int cpu = (K > 0) ? static_cast<int>(rel_idx / K) : 0;
            if (cpu >= 0 && cpu < static_cast<int>(num_cpus)) {
                cpu_groups[cpu].push_back(buf);
            }
        }

        for (auto& g : cpu_groups) {
            std::sort(g.begin(), g.end(),
                      [](const BufferMeta& a, const BufferMeta& b) {
                          return a.seq < b.seq;
                      });
        }

        sorted_buffers.clear();
        for (auto& g : cpu_groups) {
            sorted_buffers.insert(sorted_buffers.end(), g.begin(), g.end());
        }
    }
};

}  // namespace qst
