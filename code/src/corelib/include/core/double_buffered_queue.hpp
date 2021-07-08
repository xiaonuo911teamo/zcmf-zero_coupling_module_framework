#pragma once

#include <deque>
#include <atomic>
#include <mutex>

// 带有线程保护的queue
template <class T>
class DoubleBufferedQueue {
public:
    DoubleBufferedQueue(int max_size = 10):
        _data_buffer_updated(false),
        _max_size(max_size) {
    }

    void push_data(const T& data) {
        std::lock_guard<std::mutex> lg(data_mutex);
        _data_buffer.push_back(data);
        _data_buffer_updated = true;

        if (_data_buffer.size() > _max_size) {
            _data_buffer.pop_front();
            _overflowed = true;
        }
    }

    void push_data(const std::deque<T>& data) {
        std::lock_guard<std::mutex> lg(data_mutex);
        _data_buffer.clear();
        for (auto& t : data) {
            _data_buffer.push_back(t);
            _data_buffer_updated = true;

            if (_data_buffer.size() > _max_size) {
                _data_buffer.pop_front();
                _overflowed = true;
            }
        }
        _data_mutex.unlock();
    }

    // 区别与peek_data,  get_data会取出原数据, 而peek_data仅仅是复制出来 
    const std::deque<T> get_data() {
        if (_data_buffer_updated) {
            _data_mutex.lock();
            _data.clear();
            _data.swap(_data_buffer);
            _data_buffer_updated = false;
            _overflowed = false;
            _data_mutex.unlock();
        }

        return _data;
    }

    // 区别与get_data,  peek_data仅仅是复制出来, 而get_data会取出原数据
    const std::deque<T> peek_data() {
        std::deque<T> data;
        if (_data_buffer_updated) {
            _data_mutex.lock();
            data = _data_buffer;
            _data_mutex.unlock();
        }
        return data;
    }

    void clear() {
        _data.clear();
        _data_mutex.lock();
        _data_buffer.clear();
        _overflowed = false;
        _data_buffer_updated = false;
        _data_mutex.unlock();
    }

    // 与std中deque不同, 此swap只能做到单向交换, 可以通过swap的方式将数据取出, 但是外部的数据不会进入到_data_buffer
    // 即连续使用两次 swap 并不会等于无操作.
    // TODO: 
    // 1. 完成线程安全的双向交换
    // 2. 同时, 此结构中有定义最大元素个数, 直接交换存储区, 不能保证内部数据安全. 故后续将引入主动抛出异常的情况, 防止这类错误发生
    void swap(std::deque<T>& que_data) {
        get_data();
        _data.swap(que_data);
    }

    bool is_updated() {
        return _data_buffer_updated;
    }

    bool is_overflowed() {
        return _overflowed;
    }

private:
    std::deque<T> _data;
    std::deque<T> _data_buffer;
    std::mutex _data_mutex;
    std::atomic<bool> _data_buffer_updated;
    int _max_size;
    bool _overflowed = false;
};

