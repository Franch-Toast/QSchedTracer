#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace qst {

class FileAccess {
public:
    FileAccess() = default;
    ~FileAccess() { close(); }

    FileAccess(const FileAccess&) = delete;
    FileAccess& operator=(const FileAccess&) = delete;

    void open(const std::string& path) {
        fd_ = ::open(path.c_str(), O_RDONLY);
        if (fd_ < 0)
            throw std::runtime_error("Cannot open file: " + path);
        struct stat st;
        if (fstat(fd_, &st) < 0) {
            ::close(fd_);
            fd_ = -1;
            throw std::runtime_error("Cannot stat file: " + path);
        }
        size_ = static_cast<size_t>(st.st_size);
        data_ = static_cast<const uint8_t*>(
            mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0));
        if (data_ == MAP_FAILED) {
            data_ = nullptr;
            ::close(fd_);
            fd_ = -1;
            throw std::runtime_error("mmap failed: " + path);
        }
    }

    void close() {
        if (data_) {
            munmap(const_cast<uint8_t*>(data_), size_);
            data_ = nullptr;
        }
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    const uint8_t* ptr(size_t offset) const { return data_ + offset; }
    size_t file_size() const { return size_; }

    uint32_t u32_at(size_t offset) const {
        uint32_t v;
        memcpy(&v, data_ + offset, 4);
        return v;
    }

    uint64_t u64_at(size_t offset) const {
        uint64_t v;
        memcpy(&v, data_ + offset, 8);
        return v;
    }

    int64_t i64_at(size_t offset) const {
        int64_t v;
        memcpy(&v, data_ + offset, 8);
        return v;
    }

private:
    int fd_ = -1;
    const uint8_t* data_ = nullptr;
    size_t size_ = 0;
};

}  // namespace qst
