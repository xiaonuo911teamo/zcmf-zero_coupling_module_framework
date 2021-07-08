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
#include <utils/app_preference.hpp>
#include <utils/app_util.hpp>

namespace basic {
// 并发模块
class PipeElement {
public:
    // 构造PipeElement，循环处理模块
    // @_wait_cond 是否等待submit处理
    // @name 模块名称
    explicit PipeElement(bool _wait_cond = true, std::string name = "default") :
        _running(false), _busy(false),
        _wait_cond(_wait_cond),
        _submitted(false) , _thr_name(name) {
        DIRECT() << "Task " << name << " loaded!";
    }
    virtual ~PipeElement() = default;

    // 模块启动函数
    virtual void start() {
        if (_running) {
            ERROR() << "start pipe element failed, pipe element is already _running!";
            return;
        }

        _thd = std::thread(
        [&] {
            thread_initial();
            _running = true;

            if (appPref.has_string_key("affinity.CpuIndex")) {
                auto affinity_str = appPref.get_string_data("affinity.CpuIndex");
                AppUtil::set_thread_affinity(std::stoi(affinity_str));
            }

            auto affinity_key = "affinity." + _thr_name;
            if (appPref.has_string_key(affinity_key)) {
                auto affinity_str = appPref.get_string_data(affinity_key);
                AppUtil::set_thread_affinity(std::stoi(affinity_str));
            }


            while (_running) {
                if (_wait_cond && !_submitted) {
                    std::unique_lock<std::mutex> lck(_cv_mutex);
                    _func_cv.wait(lck);

                    if (!_running) {
                        break;
                    }
                }

                _busy = true;
                _submitted = false;
                this->thread_func();
                _busy = false;
            }
            thread_closing();
        });

        pthread_setname_np(_thd.native_handle(), _thr_name.c_str());
        auto prio_key = "_priority." + _thr_name;
        if (appPref.has_string_key(prio_key)) {
            auto prio_str = appPref.get_string_data(prio_key);
            set_pthd_schd_param(SCHED_RR, std::stoi(prio_str));
            INFO() << "Task " << _thr_name << " _priority: " << prio_str;
        } else {
            WARNING() << "Task " << _thr_name << " using default _priority";
        }
    }
    // 模块停止函数
    virtual void stop() {
        if (!_running) {
            ERROR() << "stop failed, pipe element is not _running!";
            return;
        }
        _running = false;
        submit();
    }
    
    // 等待模块退出函数
    void wait() {
        _thd.join();
    }
    // 提交函数，在wait_cond=true时，主动调用处理函数
    void submit() {
        _submitted = true;
        _func_cv.notify_one();
    }
    // 获取busy状态
    bool is_busy() {
        return _busy;
    }
    // 直接设置 _wait_cond 状态
    void set_wait_cond(bool value) {
        _wait_cond = value;
    }

    // 返回模块的运行状态
    bool is_running() const {
        return _running;
    }

    // 设置线程的优先级和优先策略
    void set_pthd_schd_param(int in_policy, int in_priority ) {
        sched_param param;
        param.sched_priority = in_priority;
        pthread_setschedparam(_thd.native_handle(), in_policy, &param);

        this->_policy = in_policy;
        this->_priority = in_priority;
    }

private:
    virtual void thread_func() {};

    virtual void thread_initial() {}

    virtual void thread_closing() {}

protected:
    std::thread _thd;
    std::condition_variable _func_cv;
    std::mutex _cv_mutex;
    std::atomic_bool _running;       // 运行状态
    std::atomic_bool _busy;
    std::atomic_bool _wait_cond;
    std::atomic_bool _submitted;
    std::string _thr_name;
    int _priority;                   // 优先级
    int _policy;                     // 优先策略
};
}
