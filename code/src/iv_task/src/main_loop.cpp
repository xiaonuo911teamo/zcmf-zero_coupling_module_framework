#include "main_loop.h"
#include <diag/diagnose.hpp>

MainLoop::MainLoop() : TimerElement(1000, "MainLoop")
{
    Diagnose::register_server("exec_cmd", [&](const std::string& cmd) {
        std::string message;
        if (cmd == "quit") {
            stop();
        } else if (cmd == "restart") {
            stop();
            _is_restart = true;
        } else {
            message += "cmd ";
            message += cmd + " not found!";
        }
        message = "exec successed!";
        return message;
    });
}

void MainLoop::timer_func()
{
    
}

bool MainLoop::is_restart() const
{
    return _is_restart;
}
