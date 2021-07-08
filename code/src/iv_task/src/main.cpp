#include <dlfcn.h>
#include <map>
#include <log/logging.h>
#include <pipe/pipe_controller.hpp>
#include <pipe/timer_trigger.hpp>
#include <utils/app_config.hpp>
#include <utils/app_preference.hpp>
#include <message/messager.hpp>
#include <diag/diagnose.hpp>
#include "main_loop.h"
#include<unistd.h>
#include <utils/dl_utils.hpp>

#include <sys/wait.h>
#include <signal.h>

MainLoop main_loop;
TimerTrigger g_timer_trigger;

void stop(int) {
    main_loop.stop();
    g_timer_trigger.stop();
}

int main(int argc, char *argv[])
{
    DlUtils::initial_path(argv);
    std::stringstream ss;
    ss << appPref.get_string_data("exe_file") << " ";
    main_loop.start();
    for (int i = 1; i < argc; i++) {
        DlUtils::load_plugin(argv[i]);
        ss << argv[i] << " ";
    }
    g_timer_trigger.start();
    for (int i = 1; i < argc; i++) {
        DlUtils::run_plugin(argv[i]);
    }
    signal(SIGINT, &stop);
    main_loop.wait();
    g_timer_trigger.wait();
    for (int i = 1; i < argc; i++) {
        DlUtils::unload_plugin(argv[i]);
    }
    ss << " &";
    if (main_loop.is_restart()) {
        system(ss.str().c_str());
    }
}
