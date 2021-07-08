/*************************************************************************************************
 * 频率计算模块
 * 
 * 这是一个单例模块, 用于对各种周期信号的频率计算.
 * Frequence 内部通过map结构保存对象的, 使用<name, frequeue>的方式进行保存, 所以每种信号都必须有独立的名字
 * 
 * 使用方法:
 * 
 * thread1: {
 *      Frequence::instance().trigger_once("signal");
 * }
 * 
 * thread2: {
 *      auto freq = Frequence::instance().get_frequence("signal");
 *      std::cout << "signal's frequence: " << freq << std::endl;
 * }
 * 
/************************************************************************************************/

#pragma once

#include <pipe/timer_element.hpp>
#include <map>
#include <string>
#include <mutex>

// 
class Frequence : public TimerElement
{
private:
    struct FCounter
    {
        int counter;
        double frequence;           // Hz
    };

    std::map<std::string, FCounter> _counter_map;
    std::mutex _mutex;

public:
    ~Frequence() {
        stop();
        wait();
    }

public:
    static void trigger_once(const std::string& name) {
        instance().trigger_inner(name);
    }

    static double get_frequence(const std::string &name)
    {
        return instance().frequence_inner(name);
    }

    static void initail_instance() {
        instance();
    }

private:
    // 3秒累计一次频率
    Frequence(): TimerElement(3000, "Frequence") {
        start();
    }

    virtual void timer_func() override {
        std::lock_guard<std::mutex> lg(_mutex);
        for (auto &counter : _counter_map)
        {
            counter.second.frequence = 1000.0 * counter.second.counter / get_interval();
            counter.second.counter = 0;
        }
    }

    void trigger_inner(const std::string& name)
    {
        std::lock_guard<std::mutex> lg(_mutex);
        _counter_map[name].counter++;
    }

    double frequence_inner(const std::string &name)
    {
        std::lock_guard<std::mutex> lg(_mutex);
        double frequence = _counter_map[name].frequence;

        return frequence;
    }

    static Frequence& instance() {
        static Frequence frequence;
        return frequence;
    }
};
