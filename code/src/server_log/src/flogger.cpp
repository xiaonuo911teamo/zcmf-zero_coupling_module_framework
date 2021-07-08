#include "flogger.h"
#include <stdio.h>
#include <sys/stat.h>

FLogger::FLogger(const std::string &dir) : _dir(dir) {
    mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

void FLogger::error(const std::string &log) {
    _error_size += log.size();
    if(check_error_size()) {
        _error_size = log.size();
    }
    _error_ostream << log;
}

void FLogger::info(const std::string &log) {
    _info_size += log.size();
    if(check_info_size()) {
        _info_size = log.size();
    }
    _info_ostream << log;
}

void FLogger::flush() {
    _error_ostream.flush();
    _info_ostream.flush();
}

void FLogger::remove_oldest_log() {
    auto files = AppUtil::get_file_list(_dir, ".*\\.log");
    if (!files.empty()) {
        std::string file = _dir + "/" + files.front();
        std::remove(file.c_str());
        bool failed = std::ifstream(file).is_open();
        if (failed) {
            ERROR() << "remove error log file faild: " << file;
        }
    }
}

// 检查当前ERROR.log文件的写入大小, 超过上限值, 会写入新文件
// 同时, 会检查最多保留的log个数, INFO+ERROR log总个数有一个上限值 _max_num
bool FLogger::check_error_size() {
    bool res = _error_size > _max_size;
    if (res || !_error_ostream.is_open()) {
        _error_ostream = std::ofstream(_dir + "/" + AppUtil::now_date() + "_" + AppUtil::now_time() + "_ERROR.log");
        check_file_num();
    }
    return res;
}

bool FLogger::check_info_size() {
    bool res = _info_size > _max_size;
    if (res|| !_info_ostream.is_open()) {
        _info_ostream = std::ofstream(_dir + "/" + AppUtil::now_date() + "_" + AppUtil::now_time() + "_INFO.log");
        check_file_num();
    }
    return res;
}

void FLogger::check_file_num() {
    auto files = AppUtil::get_file_list(_dir, ".*\\.log");
    if (files.size() > max_num()) {          // 使用 max_num() 并不比直接使用_max_num效率差, 编译优化后, 效率是相同的.  
        int i = files.size() - max_num();
        for (; i > 0; i--) {
            remove_oldest_log();
        }
    }
}

uint64_t FLogger::max_num() const
{
    return _max_num;
}

void FLogger::set_max_num(const uint64_t &max_num)
{
    _max_num = max_num;
}

uint64_t FLogger::max_size() const
{
    return _max_size;
}

void FLogger::set_max_size(const uint64_t &max_size)
{
    _max_size = max_size;
}
