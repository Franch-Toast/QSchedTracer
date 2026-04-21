#include "qst/data/data_manager.hpp"
#include "qst/log.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <iomanip>

#ifdef __QNX__
#include <cstddef>
#endif

namespace qst {
namespace data {

DataManager::DataManager(DataBuffer& buffer,
                         const std::string& output_dir,
                         const std::string& file_prefix)
    : buffer_(buffer)
    , output_dir_(output_dir)
    , file_prefix_(file_prefix)
{
    if (mkdir(output_dir_.c_str(), 0755) != 0 && errno != EEXIST) {
        LOG_WARN("Failed to create dir: {}, errno: {}", output_dir_, errno);
    }
    chmod(output_dir_.c_str(), 0777);
    LOG_INFO("DataManager initialized, output: {}", output_dir_);
}

DataManager::~DataManager() {
    if (open_fd_ >= 0) {
        close(open_fd_);
        open_fd_ = -1;
    }
}

std::string DataManager::generateFinalPath(
    const std::chrono::system_clock::time_point& end_time) {

    auto end_t = std::chrono::system_clock::to_time_t(end_time);
    auto end_us = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time.time_since_epoch()) % 1000000;

    struct tm end_tm;
    localtime_r(&end_t, &end_tm);

    std::ostringstream oss;
    oss << output_dir_ << "/" << file_prefix_ << "."
        << std::put_time(&end_tm, "%Y%m%d.%H%M%S")
        << "." << std::setfill('0') << std::setw(6) << end_us.count()
        << ".qst";

    last_filename_ = oss.str();
    return last_filename_;
}

ssize_t DataManager::writeAll(int fd, const void* buf, size_t count) {
    const uint8_t* p = static_cast<const uint8_t*>(buf);
    size_t remaining = count;
    while (remaining > 0) {
        ssize_t n = ::write(fd, p, remaining);
        if (n <= 0) {
            LOG_ERROR("write() failed: {} (wrote {}/{})",
                      strerror(errno), count - remaining, count);
            return -1;
        }
        p += n;
        remaining -= n;
    }
    return static_cast<ssize_t>(count);
}

#ifdef __QNX__

int DataManager::openQstFile(uint64_t clock_freq,
                              const std::chrono::system_clock::time_point& start_time,
                              const std::chrono::system_clock::time_point& end_time,
                              uint32_t bufs_per_cpu) {
    if (open_fd_ >= 0) {
        LOG_WARN("Previous QST file not closed, closing now");
        close(open_fd_);
        open_fd_ = -1;
    }

    generateFinalPath(end_time);

    int fd = open(last_filename_.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        LOG_ERROR("Failed to create file: {}, error: {}",
                  last_filename_, strerror(errno));
        return -1;
    }

    v4::QstFileHeader fh;
    std::memset(&fh, 0, sizeof(fh));
    fh.magic = v4::FILE_MAGIC;
    fh.version = v4::FILE_VERSION;
    fh.header_size = sizeof(v4::QstFileHeader);
    fh.flags = 0;
    fh.clock_freq = clock_freq;
    fh.capture_start_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        start_time.time_since_epoch()).count();
    fh.capture_end_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time.time_since_epoch()).count();
    fh.num_cpus = static_cast<uint32_t>(sysconf(_SC_NPROCESSORS_ONLN));
#if defined(QNX_800)
    fh.os_version = 800;
#else
    fh.os_version = 710;
#endif
    fh.tracebuf_size = sizeof(tracebuf_t);
    fh.data_offset = offsetof(tracebuf_t, data);

    if (bufs_per_cpu > 0) {
        std::memcpy(fh.reserved, &bufs_per_cpu, sizeof(uint32_t));
    }

    if (writeAll(fd, &fh, sizeof(fh)) < 0) {
        LOG_ERROR("Failed to write QstFileHeader");
        close(fd);
        return -1;
    }

    open_fd_ = fd;
    uint32_t tb_sz = fh.tracebuf_size;
    uint32_t d_off = fh.data_offset;
    LOG_INFO("[v4] Opened {} (tracebuf_size={} data_offset={})",
             last_filename_, tb_sz, d_off);
    return 0;
}

int DataManager::writeSectionHeader(uint32_t magic, uint32_t entry_count) {
    if (open_fd_ < 0) {
        LOG_ERROR("No open QST file for section header");
        return -1;
    }

    v4::SectionHeader sh;
    std::memset(&sh, 0, sizeof(sh));
    sh.magic = magic;
    sh.version = 1;
    sh.payload_size = static_cast<uint64_t>(entry_count) * sizeof(tracebuf_t);
    sh.entry_count = entry_count;

    if (writeAll(open_fd_, &sh, sizeof(sh)) < 0) {
        LOG_ERROR("Failed to write SectionHeader (magic=0x{:x} count={})",
                  magic, entry_count);
        return -1;
    }
    return 0;
}

int DataManager::writeContiguousTracebufs(const tracebuf_t* base, int N,
                                           int start, int count) {
    if (open_fd_ < 0 || count <= 0) return -1;

    const size_t buf_sz = sizeof(tracebuf_t);

    if (count == N) {
        if (writeAll(open_fd_, base, static_cast<size_t>(N) * buf_sz) < 0)
            return -1;
    } else if (start + count <= N) {
        if (writeAll(open_fd_, &base[start], static_cast<size_t>(count) * buf_sz) < 0)
            return -1;
    } else {
        int seg1 = N - start;
        int seg2 = count - seg1;
        if (writeAll(open_fd_, &base[start], static_cast<size_t>(seg1) * buf_sz) < 0)
            return -1;
        if (writeAll(open_fd_, &base[0], static_cast<size_t>(seg2) * buf_sz) < 0)
            return -1;
    }
    return 0;
}

int DataManager::closeQstFile() {
    if (open_fd_ < 0) {
        LOG_WARN("No open QST file to close");
        return -1;
    }
    close(open_fd_);
    open_fd_ = -1;
    LOG_INFO("[v4] File complete: {}", last_filename_);
    return 0;
}

#endif  // __QNX__

}  // namespace data
}  // namespace qst
