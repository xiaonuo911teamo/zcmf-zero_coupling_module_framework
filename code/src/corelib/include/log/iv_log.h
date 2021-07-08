#pragma once
#include "log_interface.h"
#include <message/messager.hpp>

namespace iv_log {

class Debug : public LogInterface{
public:
    Debug(const std::string& file, int line) : LogInterface([&](const std::stringstream& ss) {
        Messager::publish("log_debug", ss.str());
        std::cout << ss.str();
    }, file, line, DEBUG) {
    }
};

class Info : public LogInterface{
public:
    Info(const std::string& file, int line) : LogInterface([&](const std::stringstream& ss) {
        Messager::publish("log_info", ss.str());
        std::cout << ss.str();
    }, file, line, INFO) {
    }
};

class Warning : public LogInterface{
public:
    Warning(const std::string& file, int line) : LogInterface([&](const std::stringstream& ss) {
        Messager::publish("log_warning", ss.str());
        std::cerr << ss.str();
    }, file, line, WARNING) {
    }
};

class Error : public LogInterface{
public:
    Error(const std::string& file, int line) : LogInterface([&](const std::stringstream& ss) {
        Messager::publish("log_error", ss.str());
        std::cerr << ss.str();
    }, file, line, ERROR) {
    }
};

class Fatal : public LogInterface{
public:
    Fatal(const std::string& file, int line) : LogInterface([&](const std::stringstream& ss) {
        Messager::publish("log_fatal", ss.str());
        std::cerr << ss.str();
        throw std::bad_exception();
    }, file, line, FATAL) {
    }
};

class Direct : public LogInterface{
public:
    Direct(const std::string& file, int line) : LogInterface([&](const std::stringstream& ss) {
        Messager::publish("log_direct", ss.str());
        std::cerr << ss.str();
    }, file, line, DIRECT) {
    }
};
}
