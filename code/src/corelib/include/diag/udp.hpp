// TODO: 补充注释和说明

#pragma once
#include <log/logging.h>
#include <sys/socket.h>
#include <arpa/inet.h>

class UdpInterface {
public:
    ~UdpInterface() {
    }

protected:
    UdpInterface() {
        _fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

public:
    void set_send_timeout(int time_ms) {
        int rc = 0;
        struct timeval tv;
        tv.tv_sec = time_ms / 1000;
        tv.tv_usec = time_ms % 1000 * 1000;
        rc = setsockopt(_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        FATAL_IF(rc != 0) << "setsockopt faild! rc = " << rc << "; errno = " << errno;
        _send_time_out = time_ms;
    }

    void set_recv_timeout(int time_ms) {
        int rc = 0;
        struct timeval tv;
        tv.tv_sec = time_ms / 1000;
        tv.tv_usec = time_ms % 1000 * 1000;
        rc = setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        FATAL_IF(rc != 0) << "setsockopt faild! rc = " << rc << "; errno = " << errno;
        _recv_time_out = time_ms;
    }

protected:
    int _fd = -1;
    int _recv_time_out = -1;
    int _send_time_out = -1;
};

class UdpResponser : public UdpInterface {
public:
    UdpResponser() {
        _recv_buffer.resize(200 * 1024);
    }

    void set_reponse_func(const std::function<std::string(const std::string&)>& func) {
        _rep_func = func;
    }

    void register_responser(int port) {
        _port = port;
        _address.sin_family = AF_INET;
        _address.sin_port = htons(_port);
        inet_aton("0.0.0.0", &_address.sin_addr);
        _address_len = sizeof(struct sockaddr_in);
        if (bind(this->_fd, (struct sockaddr*)&_address, _address_len) == -1) {
            FATAL() << "[listen_on_port] with [port=" << port << "] Cannot bind socket";
        }
    }

    void receive_and_respose() {
        struct sockaddr_in address;
        int size = recvfrom(this->_fd, (char*)_recv_buffer.c_str(), _recv_buffer.size(),
                                 0, (struct sockaddr*)&address, &_address_len);
        if (size > 0 && _rep_func) {
            auto rep_str = _rep_func(_recv_buffer.substr(0, size));
            sendto(this->_fd, rep_str.data(), rep_str.length(), 0, (struct sockaddr*) &address, _address_len);
        }
    }

private:
    std::function<std::string(const std::string&)> _rep_func;
    std::string _recv_buffer;
    int _port;
    struct sockaddr_in _address;
    socklen_t _address_len;
};

class UdpRequester : public UdpInterface {
public:
    UdpRequester() {
        _recv_buffer.resize(200 * 1024);
    }

    void register_requester(const std::string& ip, int port) {
        _port = port;
        _ip = ip;
    }

    bool request_and_receive(const std::string& req_data, std::string& rep_data) {
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(_port);
        inet_aton(_ip.c_str(), &address.sin_addr);
        socklen_t address_len = sizeof (address);
        int size = sendto(this->_fd, req_data.data(), req_data.length(), 0, (struct sockaddr*) &address, address_len);
        if (size > 0) {
            size = recvfrom(this->_fd, (char*)_recv_buffer.c_str(), _recv_buffer.size(),
                            0, (struct sockaddr*)&address, &address_len);
            if (size > 0) {
                rep_data = _recv_buffer.substr(0, size);
            }
        }
        return size > 0;
    }

private:
    std::string _recv_buffer;
    std::string _ip;
    int _port;
};
