#pragma once

#include <atomic>
#include <mutex>

// 自带有线程保护的模板结构
template <class T>
class DoubleBufferData
{
public:
    DoubleBufferData():
        _data_buffer_updated(false), _has_data(false) {}

    DoubleBufferData(const T& data):
        _data_buffer_updated(false) {
        set_data(data);
    }

    DoubleBufferData& operator=(const T& data) {
        set_data(data);
        return *this;
    }

    void set_data(const T& data) {
        _data_mutex.lock();
        _data_buffer = data;
        _data_buffer_updated = true;
        _has_data = true;
        _data_mutex.unlock();
    }

    const T get_data() {
        T data;
        _data_mutex.lock();
        data = _data_buffer;
        _data_buffer_updated = false;
        _data_mutex.unlock();
        return data;
    }

    const T peek_data() {
        T data;
        _data_mutex.lock();
        data = _data_buffer;
        _data_mutex.unlock();
        return data;
    }
    
    // 数据是否有更新
    bool is_updated() {
        return _data_buffer_updated;
    }
    
    // 是否有载入数据
    bool has_data() {
        return _has_data;
    }

private:
    T _data_buffer;
    std::mutex _data_mutex;
    std::atomic_bool _data_buffer_updated;
    std::atomic_bool _has_data;
};

