#include "input_result_zmq.h"
#include <log/logging.h>
#include <data/data_info.pb.h>
#include <data/qnxproc.pb.h>
#include <google/protobuf/text_format.h>
#include <message/messager.hpp>
#include <utils/app_preference.hpp>

// TODO: 死循环, CPU占用问题
InputResultZmqElement::InputResultZmqElement() : basic::PipeElement(false, "InputResultZmq") {
    auto input_result_url = appPref.get_string_data("zmq.input_result_url");
    _zmq_subscriber.subscribe(input_result_url);
    _zmq_subscriber.set_recv_timeout(100);
    _zmq_buffer.resize(200 * 1024);
}

void InputResultZmqElement::thread_func()
{
    int size = _zmq_subscriber.receive(_zmq_buffer);
    if (size > 0) {
        message::BytesItem bytes;
        std::string sub_str = _zmq_buffer.substr(0, size);
        bytes.ParseFromString(sub_str);
        if (bytes.id() == 3001) {
            std::string data = bytes.data();
            Messager::publish("log_remote", "[remote]" + data);
        } else if (bytes.id() == 3002) {
            QnxProcList result;
            result.ParseFromString(bytes.data());
            Messager::publish("performance_result", result);
        }
    }
}

InputResultZmqElement::~InputResultZmqElement() {
}

void InputResultZmqElement::thread_closing()
{
    _zmq_subscriber.shutdown();
}
