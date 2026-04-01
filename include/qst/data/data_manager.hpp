/**
 * @file data/data_manager.hpp
 * @brief QSchedTracer - QST v4 文件写入器
 *
 * QST v4 写入流程（由 TracerEngine 驱动）:
 *   1. openQstFile()                 → 创建文件，写入 QstFileHeader (64B)
 *   2. writeSectionHeader(DATA, N)   → 写入 DATA SectionHeader (24B)
 *   3. writeContiguousTracebufs(...) → 写入 raw tracebuf_t 块 (1~2 次 write)
 *   4. writeSectionHeader(PINF, M)   → 写入 PINF SectionHeader (24B)
 *   5. writeContiguousTracebufs(...) → 写入 procinfo tracebuf_t 块
 *   6. closeQstFile()               → 关闭文件
 */

#pragma once

#include "qst/types.hpp"
#include "qst/core/data_buffer.hpp"
#include <string>
#include <chrono>

#ifdef __QNX__
#include <sys/trace.h>
#endif

namespace qst {
namespace data {

class DataManager {
public:
    DataManager(DataBuffer& buffer,
                const std::string& output_dir,
                const std::string& file_prefix = "tracer");
    ~DataManager();

#ifdef __QNX__
    int openQstFile(uint64_t clock_freq,
                    const std::chrono::system_clock::time_point& start_time,
                    const std::chrono::system_clock::time_point& end_time,
                    int event_type,
                    uint32_t bufs_per_cpu = 0);

    int writeSectionHeader(uint32_t magic, uint32_t entry_count);

    int writeContiguousTracebufs(const tracebuf_t* base, int N,
                                  int start, int count);

    int closeQstFile();
#endif

    const std::string& lastFilename() const { return last_filename_; }
    const std::string& outputDir() const { return output_dir_; }

private:
    std::string generateFinalPath(
        const std::chrono::system_clock::time_point& start_time,
        const std::chrono::system_clock::time_point& end_time,
        int event_type);

    static ssize_t writeAll(int fd, const void* buf, size_t count);

    DataBuffer& buffer_;
    std::string output_dir_;
    std::string file_prefix_;
    std::string last_filename_;
    int open_fd_{-1};
};

}  // namespace data
}  // namespace qst
