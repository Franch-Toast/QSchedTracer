/**
 * @file data/data_manager.cpp
 * @brief QSchedTracer - 数据管理器实现
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#include "qst/data/data_manager.hpp"
#include "qst/log.hpp"
#include "qst/types.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <ctime>

namespace qst {
namespace data {

DataManager::DataManager(DataBuffer& data_buffer)
    : data_buffer_(data_buffer)
{
}

DataManager::~DataManager() = default;

std::string DataManager::generateFilename() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    
    struct tm tm_info;
    localtime_r(&ts.tv_sec, &tm_info);
    
    char filename[256];
    snprintf(filename, sizeof(filename),
             "trace_%04d%02d%02d_%02d%02d%02d.qst",
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
             tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec);
    
    return filename;
}

void DataManager::setProcInfoBuffer(DataBuffer&& buffer) {
    // 从传入的缓冲区获取有序数据
    uint32_t sync_cycles = 0;
    procinfo_buffer_ = buffer.getOrderedData(&procinfo_size_, &procinfo_count_, &sync_cycles);
    
    if (procinfo_buffer_) {
        LOG_INFO("进程/线程信息: {} 字节, {} 个事件", procinfo_size_, procinfo_count_);
    } else {
        procinfo_size_ = 0;
        procinfo_count_ = 0;
        LOG_WARN("未采集到进程/线程信息");
    }
}

int DataManager::save() {
    std::string filename = generateFilename();
    return saveTo(filename);
}

int DataManager::saveTo(const std::string& filename) {
    last_filename_ = filename;
    return writeToFile(filename);
}

int DataManager::writeToFile(const std::string& filename) {
    LOG_INFO("保存数据到文件: {}", filename);
    
    data_buffer_.setState(State::Dumping);
    
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        LOG_ERROR("打开文件失败: {}", strerror(errno));
        return -1;
    }
    
    // ========================================================================
    // 使用 getOrderedData() 获取按时间顺序的主调度数据
    // (直接返回智能指针，无需手动释放)
    // ========================================================================
    
    size_t main_data_size = 0;
    size_t main_event_count = 0;
    uint32_t sync_cycles = 0;
    auto main_data = data_buffer_.getOrderedData(&main_data_size, &main_event_count, &sync_cycles);
    
    LOG_INFO("主调度数据: {} 个事件, sync_cycles={}", main_event_count, sync_cycles);
    
    // ========================================================================
    // 1. 写入文件头 (FileHeader, 64 字节)
    // ========================================================================
    
    FileHeader header{};
    header.magic = constants::MAGIC;
    header.version = constants::VERSION;
    header.clock_freq = data_buffer_.clockFreq();
    header.wallclock_sec = data_buffer_.wallclockSec();
    header.wallclock_nsec = data_buffer_.wallclockNsec();
    header.sync_cycles = sync_cycles;
    std::memset(header.reserved, 0, sizeof(header.reserved));
    
    if (::write(fd, &header, sizeof(header)) != sizeof(header)) {
        LOG_ERROR("写入文件头失败");
        close(fd);
        return -1;
    }
    LOG_DEBUG("写入文件头: {} 字节", sizeof(header));
    
    // ========================================================================
    // 2. 写入进程/线程信息 (ProcInfoHeader + 数据)
    // ========================================================================
    
    ProcInfoHeader procinfo_header{};
    procinfo_header.magic = constants::PROCINFO_MAGIC;
    procinfo_header.event_count = static_cast<uint32_t>(procinfo_count_);
    procinfo_header.reserved = 0;
    
    if (::write(fd, &procinfo_header, sizeof(procinfo_header)) != sizeof(procinfo_header)) {
        LOG_ERROR("写入进程/线程信息头失败");
        close(fd);
        return -1;
    }
    LOG_DEBUG("写入进程/线程信息头: {} 字节", sizeof(procinfo_header));
    
    if (procinfo_buffer_ && procinfo_size_ > 0) {
        ssize_t written = ::write(fd, procinfo_buffer_.get(), procinfo_size_);
        if (written != static_cast<ssize_t>(procinfo_size_)) {
            LOG_ERROR("写入进程/线程信息数据失败: 期望 {}, 实际 {}", 
                      procinfo_size_, written);
            close(fd);
            return -1;
        }
        LOG_DEBUG("写入进程/线程信息数据: {} 字节, {} 个事件", 
                  procinfo_size_, procinfo_count_);
    }
    
    // ========================================================================
    // 3. 写入主调度数据 (MainDataHeader + 数据，已按时间顺序)
    // ========================================================================
    
    MainDataHeader maindata_header{};
    maindata_header.magic = constants::MAINDATA_MAGIC;
    maindata_header.event_count = static_cast<uint32_t>(main_event_count);
    maindata_header.reserved = 0;
    
    if (::write(fd, &maindata_header, sizeof(maindata_header)) != sizeof(maindata_header)) {
        LOG_ERROR("写入主调度数据头失败");
        close(fd);
        return -1;
    }
    LOG_DEBUG("写入主调度数据头: {} 字节", sizeof(maindata_header));
    
    if (main_data && main_data_size > 0) {
        ssize_t written = ::write(fd, main_data.get(), main_data_size);
        if (written != static_cast<ssize_t>(main_data_size)) {
            LOG_ERROR("写入主调度数据失败: 期望 {}, 实际 {}", main_data_size, written);
            close(fd);
            return -1;
        }
    }
    
    LOG_DEBUG("写入主调度数据: {} 字节", main_data_size);
    
    // ========================================================================
    // 完成
    // ========================================================================
    
    close(fd);
    
    LOG_INFO("保存完成:");
    LOG_INFO("  文件头: {} 字节", sizeof(FileHeader));
    LOG_INFO("  进程/线程信息: {} + {} = {} 字节 ({} 个事件)", 
             sizeof(ProcInfoHeader), procinfo_size_, 
             sizeof(ProcInfoHeader) + procinfo_size_, procinfo_count_);
    LOG_INFO("  主调度数据: {} + {} = {} 字节 ({} 个事件)",
             sizeof(MainDataHeader), main_data_size,
             sizeof(MainDataHeader) + main_data_size, main_event_count);
    LOG_INFO("  文件总大小: {} 字节", 
             sizeof(FileHeader) + sizeof(ProcInfoHeader) + procinfo_size_ + 
             sizeof(MainDataHeader) + main_data_size);
    LOG_INFO("  sync_cycles: {} (最后事件的 cycles)", sync_cycles);
    
    data_buffer_.setState(State::Idle);
    
    return 0;
}

} // namespace data
} // namespace qst

