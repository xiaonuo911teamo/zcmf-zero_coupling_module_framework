#pragma once

#include <vector>
#include <atomic>
#include <mutex>

// 带有线程保护的vector
// 与deque不同, vector中的数据满了以后, 如果不执行clear get_data, 将不会再更新数据
template <class T>
class DoubleBufferedVector {
public:
    DoubleBufferedVector(int max_size = 10):
        _data_buffer_updated(false),
        _max_size(max_size) {
    }

    void push_data(const T& data) {
        std::lock_guard<std::mutex> lg(_data_mutex);
        _data_buffer.push_back(data);
        if (_data_buffer.size() > _max_size) {
            _data_buffer.pop_back();
            _overflowed = true;
        }
        _data_buffer_updated = true;
    }

    void push_data(const std::vector<T>& data) {
        std::lock_guard<std::mutex> lg(_data_mutex);
        _data_buffer.clear();
        for (auto& t : data) {
            _data_buffer.push_back(t);
            if (_data_buffer.size() > _max_size) {
                _data_buffer.pop_back();
                _overflowed = true;
                break;
            }
        }
        _data_buffer_updated = true;
    }

    // 区别与peek_data,  get_data会取出原数据, 而peek_data仅仅是复制出来
    const std::vector<T> get_data() {
        std::lock_guard<std::mutex> lg(_data_mutex);
        _data.clear();
        if (_data_buffer_updated) {
            _data.swap(_data_buffer);
            _data_buffer_updated = false;
            _overflowed = false;
        }
        return _data;
    }

    // 区别与get_data,  peek_data仅仅是复制出来, 而get_data会取出原数据
    const std::vector<T> peek_data() {
        std::vector<T> _data;
        if (_data_buffer_updated) {
            _data_mutex.lock();
            _data = _data_buffer;
            _data_mutex.unlock();
        }
        return _data;
    }

    void clear() {
        std::lock_guard<std::mutex> lg(_data_mutex);
        _data_buffer.clear();
        _overflowed = false;
        _data_buffer_updated = false;
    }

    // TODO: 
    // 1. 完成线程安全的双向交换
    // 2. 同时, 此结构中有定义最大元素个数, 直接交换存储区, 不能保证内部数据安全. 故后续将引入主动抛出异常的情况, 防止这类错误发生
    void swap(std::vector<T>& vec_data) {
        std::lock_guard<std::mutex> lg(_data_mutex);
        if (!_data_buffer_updated) {
            _data_buffer.clear();
        }
        _data_buffer.swap(vec_data);
    }

    // 虽然是get操作, 但是为了保持多线程的一致性, 内部仍然加了线程锁
    size_t size() {
        std::lock_guard<std::mutex> lg(_data_mutex);
        return _data_buffer.size();
    }

    bool is_updated() {
        return _data_buffer_updated;
    }
    bool is_overflowed() {
        return _overflowed;
    }
private:
    std::vector<T> _data;
    std::vector<T> _data_buffer;
    std::mutex _data_mutex;
    std::atomic<bool> _data_buffer_updated;
    int _max_size;
    bool _overflowed = false;
};

