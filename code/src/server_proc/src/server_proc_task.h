#pragma once

#include <pipe/timer_element.hpp>
#include <core/double_buffer_data.hpp>
#include <data/qnxproc.pb.h>
#include <zmq/zmq.hpp>
class ServerProcTask : public TimerElement{
public:
    ServerProcTask();

    // TimerElement interface
private:
    void timer_func();

private:
    DoubleBufferData<std::string> _proc_text;
    DoubleBufferData<QnxProcList> _proc_list;
    std::string _proc_log_dir;
    bool _proc_log;
    bool _proc_remote;
};
