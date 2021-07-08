#pragma once
#include <core/double_buffered_vector.hpp>
#include <message/messager.hpp>
#include <diag/frequence.hpp>
#include <diag/diagnose.hpp>
#include <pipe/timer_element.hpp>
#include "flogger.h"
class ServerLogElement : public TimerElement
{

public:
    ServerLogElement();

public:
    static void initial();

    const static ServerLogElement& instance() {
        static ServerLogElement l_instance;
        return l_instance;
    }

private:
    void error(const std::string& log);
    void info(const std::string& log);

private:
    virtual void timer_func() override;

private:
    DoubleBufferedVector<std::string> _buffer_error;
    DoubleBufferedVector<std::string> _buffer_info;
    FLogger _floger;
};


