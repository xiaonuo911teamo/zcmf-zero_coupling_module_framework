#include "output_result_zmq.h"
#include <log/logging.h>
#include <data/qnxproc.pb.h>
#include <google/protobuf/text_format.h>
#include <message/messager.hpp>
#include <utils/app_preference.hpp>

OutputResultZmqElement::OutputResultZmqElement() : basic::PipeElement(true, "OutputResultZmq") {
    auto output_result_url = appPref.get_string_data("zmq.output_result_url");
    _zmq_publisher.register_publisher(output_result_url);

    auto log_func = [&](const std::string& log) {
        auto data_info = std::make_shared<message::BytesItem>();
        data_info->set_data(log);
        data_info->set_id(3001);
        _buffered_data.push_data(data_info);
        submit();
    };

    Messager::subcribe<std::string>("log_debug", log_func);
    Messager::subcribe<std::string>("log_info", log_func);
    Messager::subcribe<std::string>("log_warning", log_func);
    Messager::subcribe<std::string>("log_error", log_func);
    Messager::subcribe<std::string>("log_fatal", log_func);
    Messager::subcribe<std::string>("log_direct", log_func);

    Messager::subcribe<QnxProcList>(
        "performance_result",
        [this](const QnxProcList &data) {
            auto data_info = std::make_shared<message::BytesItem>();
            auto data_str = data.SerializeAsString();
            data_info->set_data(data_str);
            data_info->set_id(3002);
            _buffered_data.push_data(data_info);
            submit();
        });
}

void OutputResultZmqElement::thread_func()
{
    auto buffer_data = _buffered_data.get_data();
    for (const auto& data : buffer_data) {
        std::string buffer;
        data->SerializeToString(&buffer);
        _zmq_publisher.publish(buffer);
    }
}

OutputResultZmqElement::~OutputResultZmqElement() {
}


void OutputResultZmqElement::thread_closing()
{
    _zmq_publisher.shutdown();
}
