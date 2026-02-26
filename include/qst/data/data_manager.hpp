/**
 * @file data/data_manager.hpp
 * @brief QSchedTracer - 数据管理器
 * 
 * @details
 * 负责数据落盘和进程信息采集。
 * 文件名固定格式: trace_{timestamp}.qst
 * 
 * @author QSchedTracer Team
 * @date 2026-01-26
 */

#pragma once

#include "qst/core/data_buffer.hpp"
#include "qst/types.hpp"
#include <string>
#include <memory>
#include <cstdint>

namespace qst {
namespace data {

/**
 * @brief 数据管理器
 * 
 * 负责：
 * 1. 采集进程/线程信息 (落盘前)
 * 2. 保存数据到 .qst 文件
 * 3. 生成固定格式的文件名
 */
class DataManager {
public:
    /**
     * @brief 构造函数
     * @param data_buffer 数据缓冲区引用
     */
    explicit DataManager(DataBuffer& data_buffer);
    
    /**
     * @brief 析构函数
     */
    ~DataManager();
    
    // 禁止拷贝
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;
    
    /**
     * @brief 设置进程/线程信息缓冲区
     * 
     * 由 TracerEngine 采集后传入。
     * 
     * @param buffer 包含进程/线程信息的缓冲区
     */
    void setProcInfoBuffer(DataBuffer&& buffer);
    
    /**
     * @brief 保存数据到文件
     * 
     * 文件名自动生成: trace_{timestamp}.qst
     * 
     * @return 0 成功, -1 失败
     */
    int save();
    
    /**
     * @brief 保存数据到指定文件
     * @param filename 文件名
     * @return 0 成功, -1 失败
     */
    int saveTo(const std::string& filename);
    
    /**
     * @brief 获取最后保存的文件名
     * @return 文件名
     */
    const std::string& lastFilename() const { return last_filename_; }
    
    /**
     * @brief 获取进程信息数据大小
     * @return 字节数
     */
    size_t procInfoSize() const { return procinfo_size_; }
    
    /**
     * @brief 获取进程信息事件数量
     * @return 事件数
     */
    size_t procInfoCount() const { return procinfo_count_; }

private:
    /**
     * @brief 生成时间戳文件名
     * @return 文件名 (trace_YYYYMMDD_HHMMSS.qst)
     */
    static std::string generateFilename();
    
    /**
     * @brief 写入文件
     * @param filename 文件名
     * @return 0 成功, -1 失败
     */
    int writeToFile(const std::string& filename);

private:
    DataBuffer& data_buffer_;                       ///< 主数据缓冲区引用
    
    // 进程/线程信息缓冲区
    std::unique_ptr<uint8_t[]> procinfo_buffer_;    ///< 进程信息缓冲区
    size_t procinfo_size_{0};                       ///< 进程信息数据大小
    size_t procinfo_count_{0};                      ///< 进程信息事件数量
    
    std::string last_filename_;                     ///< 最后保存的文件名
};

} // namespace data
} // namespace qst

