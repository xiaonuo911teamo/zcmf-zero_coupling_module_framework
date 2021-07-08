#pragma once

#include <string>
#include <sys/time.h>
#include <time.h>
#include <iostream>
#include <message/messager.hpp>
#include <memory>
#include <mutex>
#include <utils/app_util.hpp>
#include <core/data_type.hpp>
#include <log/logging.h>

// TimerCounter 用于统计算法的用时，从构造函数开始计时，截止至析构函数，并输出中间运行时间
class TimerCounter {
public:
    // 构造 TimerCounter, 并开始计时
    // @flag 计时片段的代号
    // @enable_cout 是否在析构时自动输出
    // @threshold 超时阈值，如果超过threshold才输出用时, ms
    TimerCounter(const std::string& flag, bool enable_cout = false, long threshold = -1) :
        _enable_cout(enable_cout), _threshold(threshold) {
        _start = AppUtil::get_current_us();
        this->_flag = flag;

    }
    ~TimerCounter() {
        if (_enable_cout) {
            long delta = get_time_ms_elapsed();
            if (delta > _threshold) {
                //INFO() << _flag << " Time elapsed: " << delta << "ms";
            }
        }
    }

    // 在flag后追加key
    void addkey(const std::string& key) {
        this->_flag += key;
    }

    // 获取截止到当前，TimerCounter中统计的时间
    long get_time_ms_elapsed() {
        long end = AppUtil::get_current_us();
        long n = (end - _start) / 1000;
        return n;
    }

private:
    long _start;        // 开始计时的时间
    bool _enable_cout;  // 是否在析构时自动输出
    std::string _flag;  // 计时片段的代号, 析构时可以输出
    long _threshold;    // 自动输出用时的阈值
};
