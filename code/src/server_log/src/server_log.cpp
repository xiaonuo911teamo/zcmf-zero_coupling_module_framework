#include "server_log.h"
#include <utils/app_preference.hpp>

ServerLogElement::ServerLogElement() :
    TimerElement(1000, "ServerLog"),
    _buffer_error(1000), _buffer_info(1000),
    _floger(appPref.get_string_data("log.log_dir")) {
    _floger.set_max_num(AppUtil::safe_stoi(appPref.get_string_data("log.log_num")));
    _floger.set_max_size(AppUtil::safe_stoi(appPref.get_string_data("log.log_size")) * 1024);

    Messager::subcribe<std::string>("log_debug", [&](const std::string& log) {
        info(log);
    });
    Messager::subcribe<std::string>("log_info", [&](const std::string& log) {
        info(log);
    });
    Messager::subcribe<std::string>("log_direct", [&](const std::string& log) {
        info(log);
    });
    Messager::subcribe<std::string>("log_warning", [&](const std::string& log) {
        error(log);
    });
    Messager::subcribe<std::string>("log_error", [&](const std::string& log) {
        error(log);
    });
    Messager::subcribe<std::string>("log_fatal", [&](const std::string& log) {
        error(log);
    });
    Messager::subcribe<std::string>("log_remote", [&](const std::string& log) {
        info(log);
    });
}

void ServerLogElement::initial()
{
    ServerLogElement::instance();
}

void ServerLogElement::error(const std::string &log)
{
    _buffer_error.push_data(log);
}

void ServerLogElement::info(const std::string &log)
{
    _buffer_info.push_data(log);
}

void ServerLogElement::timer_func()
{
    if (_buffer_error.is_updated()) {
        auto errors = _buffer_error.get_data();
        for (auto error : errors) {
            _floger.error(error);
        }
    }
    if (_buffer_info.is_updated()) {
        auto infos = _buffer_info.get_data();
        for (auto info : infos) {
            _floger.info(info);
        }
    }
    _floger.flush();
}
