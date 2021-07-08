#pragma once
#include <pipe/pipe_element.hpp>
#include <utils/app_util.hpp>

// 定时处理模块
class TimerElement : public basic::PipeElement {
public:
    // 构造TimerElement
    // @interval 定时模块处理间隔
    // @name 模块名称
    TimerElement(int interval_ms, std::string name = "default") :
        PipeElement(true, name), _interval(interval_ms * 1000) {
        }

    virtual ~TimerElement() = default;

    virtual void start() {
        if (_running) {
            ERROR() << "start pipe element failed, pipe element is already running!";
            return;
        }

        Messager::subcribe ( 
            "timer_trigger", 
            [this]() { 
                uint64_t time = AppUtil::get_current_us();
                bool do_exec = false;
                if (time >= _last_time + _interval) { 
                    _last_time = time;
                    do_exec = true;
                }
                if (do_exec) {
                    submit();
                }
        });
        PipeElement::start();
    }
    
    // 获取处理间隔
    int get_interval() const {
        return _interval / 1000;
    }

    // 设置处理间隔
    void set_interval(int value) {
        _interval = value * 1000;
    }

private:
    virtual void thread_func() {
        timer_func();
    }

    virtual void timer_func() {}

protected:
    int _interval;
    uint64_t _last_time = 0;
};