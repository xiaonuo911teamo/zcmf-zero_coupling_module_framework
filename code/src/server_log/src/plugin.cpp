#include "server_log.h"
#include <pipe/pipe_controller.hpp>

PipeController g_controllor;

extern "C"{

void LOAD_PLUGIN() {
    g_controllor.add_element<ServerLogElement>();
}

void RUN_PLUGIN() {
    g_controllor.start();
}

void UNLOAD_PLUGIN() {
    g_controllor.stop();
    g_controllor.wait();
}

}
