#pragma once

#include <vector>
#include <pipe/pipe_element.hpp>
#include <memory>

// 模块控制器，可同时控制多个模块启动，停止
class PipeController {
public:
    // 注册模块，到_pipe_elements
    void register_element(std::shared_ptr<basic::PipeElement> element) {
        _pipe_elements.push_back(element);
    }

    // 加入模块，到_pipe_elements，与register_element是两种方式
    template<class T>
    void add_element() {
        auto element = std::make_shared<T>();
        _pipe_elements.push_back(element);
    }

    template<class T>
    void add_element(T *ele) {
        std::shared_ptr<T> sele(ele);
        _pipe_elements.push_back(sele);
    }

    void start() {
        for (auto iter : _pipe_elements) {
            iter->start();
        }
    }
    void stop() {
        for (auto iter : _pipe_elements) {
            iter->stop();
        }
    }
    void wait() {
        for (auto iter : _pipe_elements) {
            iter->wait();
        }
    }

    // 创建全局的模块, 加入global()中
    template<class T>
    static std::shared_ptr<T> create_global_element() {
        auto element = std::make_shared<T>();
        global().register_element(element);
        return element;
    }

    static PipeController& global() {
        static PipeController controller;
        return controller;
    }

private:
    // 保存多个element, 进行同步管理
    std::vector<std::shared_ptr<basic::PipeElement>> _pipe_elements;
};

