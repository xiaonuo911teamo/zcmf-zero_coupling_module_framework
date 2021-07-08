#include "input_result_zmq.h"
#include <pipe/pipe_controller.hpp>

PipeController g_controllor;

extern "C"{

void LOAD_PLUGIN() {
    g_controllor.add_element<InputResultZmqElement>();
}

void RUN_PLUGIN() {
    g_controllor.start();
}

void UNLOAD_PLUGIN() {
    g_controllor.stop();
    g_controllor.wait();
}

}
