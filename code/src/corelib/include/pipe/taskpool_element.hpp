#pragma once

#include <map>
#include <mutex>
#include<vector>
#include <thread>
#include <atomic>
#include <functional>
#include <log/logging.h>
#include <condition_variable>
#include <sched.h>
#include "pipe_element.hpp"
#include <queue>

namespace basic {
// 任务池模块，将待处理的任务一一加入任务池，会自动进行处理
class TaskPoolElement : public PipeElement {
public:
    explicit TaskPoolElement(const std::string& name) : PipeElement(false, name) {
        start();
    }

    ~TaskPoolElement() {
        stop();
        wait();
    }

    // 将func加入任务池
    void asysc(const std::function<void()>& func) {
        std::unique_lock<std::mutex> lk(_mutex);
        _funcs.push(func);

        lk.unlock();
        _cv.notify_one();
    }

    // 当前任务数量
    size_t task_size() {
        return _funcs.size();
    }

private:
    virtual void thread_func() override {
        std::unique_lock<std::mutex> lk(_mutex);
        _cv.wait(lk, [this]() {
            return !_funcs.empty();
        });
        auto func = _funcs.front();
        _funcs.pop();
        lk.unlock();

        func();
    };

private:
    std::mutex _mutex;
    std::condition_variable _cv;
    std::queue<std::function<void()>> _funcs;
};
}
