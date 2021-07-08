#pragma once
#include <utils/app_util.hpp>
#include <pipe/pipe_element.hpp>
#include <message/messager.hpp>

// TimerElement 模块的触发判断函数，设置其判断精度, 默认为2ms
class TimerTrigger : public basic::PipeElement {
public:
    TimerTrigger(int precision = 2000) : PipeElement(false, "TimerTrigger"), _precision(precision) {

    }
private:
    virtual void thread_func() {
        std::this_thread::sleep_for(std::chrono::microseconds(_precision));
        Messager::publish("timer_trigger");
    }

private:
    int64_t _precision;
};