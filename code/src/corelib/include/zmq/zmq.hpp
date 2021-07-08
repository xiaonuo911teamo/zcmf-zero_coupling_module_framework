/*
zmq的封装
zmq适配层ZmqInterface，zmq发送端ZmqPublisher，zmq响应端ZmqResponser

使用方式：

ZmqPublisher zmq_publisher;
zmq_publisher.register_publisher(output_sensor_url);
zmq_publisher.publish(std::string("hello"));
zmq_publisher.shutdown();

TODO:ZmqResponser
TODO: 空格类的不规范
*/

#pragma once

#include <zmq.h>
#include <string>
#include <log/logging.h>
#include <functional>

class ZmqInterface {
public:
    ~ZmqInterface() {
        shutdown();
    }

protected:
    ZmqInterface() {
        _zmq_context = zmq_ctx_new();
    }

public:
    void set_send_timeout(int time_ms) {
        int rc = 0;
        rc = zmq_setsockopt (_zmq_socket, ZMQ_SNDTIMEO, &time_ms, sizeof(time_ms));
        FATAL_IF(rc != 0) << "zmq_setsockopt faild! rc = " << rc << "; errno = " << errno;
        _send_time_out = time_ms;
    }

    void set_recv_timeout(int time_ms) {
        int rc = 0;
        rc = zmq_setsockopt (_zmq_socket, ZMQ_RCVTIMEO, &time_ms, sizeof(time_ms));
        FATAL_IF(rc != 0) << "zmq_setsockopt faild! rc = " << rc << "; errno = " << errno;
        _recv_time_out = time_ms;
    }

    void set_send_queue_size(int size) {
        int rc = 0;
        rc = zmq_setsockopt(_zmq_socket, ZMQ_SNDHWM, &size, sizeof(size));
        FATAL_IF(rc != 0) << "zmq_setsockopt faild! rc = " << rc << "; errno = " << errno;
    }

    void set_send_buffer_size(int size) {
        int rc = 0;
        rc = zmq_setsockopt(_zmq_socket, ZMQ_SNDBUF, &size, sizeof(size));
        FATAL_IF(rc != 0) << "zmq_setsockopt faild! rc = " << rc << "; errno = " << errno;
    }

    int shutdown() {
        return zmq_ctx_shutdown(_zmq_context);
    }
protected:
    void bind(const std::string& url) {
        int rc = zmq_bind (_zmq_socket, url.c_str());
        FATAL_IF(rc != 0) << "zmq_bind faild! url: " << url << "; errno = " << errno;
    }

    void connect(const std::string& url) {
        int rc = zmq_connect(_zmq_socket, url.c_str());
        FATAL_IF(rc != 0) << "zmq_connect faild! url: " << url << "; errno = " << errno;
    }

protected:
    void* _zmq_context = nullptr;
    void* _zmq_socket = nullptr;
    int _recv_time_out = -1;
    int _send_time_out = -1;
};

class ZmqPublisher : public ZmqInterface{
public:
    ZmqPublisher() {
        _zmq_socket = zmq_socket(_zmq_context, ZMQ_PUB);
    }

    void register_publisher(const std::string& url) {
        bind(url);
    }

    int publish(const std::string& data) {
        int size = zmq_send(_zmq_socket, (char*)data.c_str(), data.size(), 0);
        return size;
    }

};

class ZmqSubscriber : public ZmqInterface{
public:
    ZmqSubscriber() {
        _zmq_socket = zmq_socket(_zmq_context, ZMQ_SUB);
        set_filter("");
    }

    void subscribe(const std::string& url) {
        connect(url);
    }

    int receive(std::string& buffer) {
        int size = zmq_recv(_zmq_socket, (char*)buffer.c_str(), buffer.size(), 0);
        return size;
    }

    void set_filter(const std::string& filter) {
        int rc = zmq_setsockopt(_zmq_socket, ZMQ_SUBSCRIBE, filter.c_str(), 0);
        FATAL_IF(rc != 0) << "zmq_setsockopt faild! rc = " << rc << "; errno = " << errno;
    }
};

class ZmqResponser : public ZmqInterface{
public:
    ZmqResponser() {
        _zmq_socket = zmq_socket(_zmq_context, ZMQ_REP);
        _recv_buffer.resize(200 * 1024);
    }

    void set_reponse_func(const std::function<std::string(const std::string&)>& func) {
        _rep_func = func;
    }

    void register_responser(const std::string& url) {
        bind(url);
    }

    void receive_and_respose() {
        int size = zmq_recv(_zmq_socket, (char*)_recv_buffer.c_str(), _recv_buffer.size(), 0);
        if (size > 0 && _rep_func) {
            auto rep_str = _rep_func(_recv_buffer.substr(0, size));
            zmq_send(_zmq_socket, rep_str.data(), rep_str.length(), 0);
        }
    }
private:
    std::function<std::string(const std::string&)> _rep_func;
    std::string _recv_buffer;
};

class ZmqRequester : public ZmqInterface{
public:
    ZmqRequester() {
        _zmq_socket = zmq_socket(_zmq_context, ZMQ_REQ);
        _recv_buffer.resize(200 * 1024);
    }

    void register_requester(const std::string& url) {
        connect(url);
        _url_list.push_back(url);
    }


    bool request_and_receive(const std::string& req_data, std::string& rep_data) {
        int size = zmq_send(_zmq_socket, req_data.data(), req_data.length(), 0);
        if (size > 0) {
            size = zmq_recv(_zmq_socket, (char*)_recv_buffer.c_str(), _recv_buffer.size(), 0);
            rep_data = _recv_buffer.substr(0, size);
        }
        return size >= 0;
    }

    void reconnect() {
        zmq_close(_zmq_socket);
        _zmq_socket = zmq_socket(_zmq_context, ZMQ_REQ);
        for (auto& url : _url_list) {
            connect(url);
        }
        set_recv_timeout(_recv_time_out);
        set_send_timeout(_send_time_out);

    }

private:
    std::string _recv_buffer;
    std::vector<std::string> _url_list;
};
