#pragma once
#include <sstream>
#include <iostream>
#include <functional>
#include <utils/app_util.hpp>
#include <pthread.h>

enum LogLevel{
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    FATAL = 4,
    DIRECT = 9999
};

class LogInterface{
public:
    LogInterface(const std::function<void(const std::stringstream&)> logging_func,
                 const std::string& file, int line, LogLevel level) {
        _logging_func = logging_func;
        _level = level;
        _ss << "[" << get_log_level_string() << " "
            << "tid:" << pthread_self() << " "
            << AppUtil::now_date() << " " << AppUtil::now_time()
            << " " << file.substr(file.find_last_of('/') + 1)
            << ":" << line << "] ";
    }

    template<class T>
    LogInterface& operator << (const T& t) {
        _ss << t;
        return *this;
    }
    LogInterface& operator<<(std::ostream& (*fun)(std::ostream&)) {
        fun(_ss);
        return *this;
    }

    ~LogInterface() {
        if (get_global_log_level() <= _level) {
            _ss << std::endl;
            _logging_func(_ss);
        }
    }
    static void set_log_level(LogLevel log_level) {
        get_global_log_level() = log_level;
    }

    static std::string log_level_to_string(LogLevel log_level) {
        switch (log_level) {
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        case WARNING:
            return "WARNING";
        case ERROR:
            return "ERROR";
        case FATAL:
            return "FATAL";
        case DIRECT:
            return "DIRECT";
        default: return "UNKNOWN";
        }
    }

private:
    static LogLevel& get_global_log_level() {
        static LogLevel log_level = INFO;
        return log_level;
    }
    std::string get_log_level_string() {
        return log_level_to_string(_level);
    }

private:
    LogInterface& operator = (const LogInterface&);

private:
    std::stringstream _ss;
    std::string _file;
    int _line;
    LogLevel _level = INFO;
    std::function<void(const std::stringstream&)> _logging_func;
};
