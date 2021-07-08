#pragma once
#include <utils/app_util.hpp>
#include <log/logging.h>
#include <ostream>

// File logger
// 文件处理类，用于将log保存在 日期-时间 的文件中
class FLogger
{
public:
    FLogger(const std::string& dir);
    ~FLogger() {

    };

    // 将字符串log写入文件_error_ostream 
    void error(const std::string& log);
    // 将字符串log写入文件_info_ostream 
    void info(const std::string& log);
    // 刷新缓冲区，立即将ostream中得内容写入到文件中
    void flush();

    inline uint64_t max_size() const;

    void set_max_size(const uint64_t &max_size);

    inline uint64_t max_num() const;

    void set_max_num(const uint64_t &max_num);

private:

    void remove_oldest_log();

    bool check_error_size();

    bool check_info_size();

    void check_file_num();

private:
    std::string _dir;
    std::ofstream _error_ostream;
    std::ofstream _info_ostream;
    uint64_t _error_size = 0;
    uint64_t _info_size = 0;
    uint64_t _max_size = 1024 * 100;
    uint64_t _max_num = 10;
};
