#include "server_proc_task.h"
#include "server_proc.h"
#include "server_proc_task.h"

#include <diag/diagnose.hpp>
#include <utils/app_preference.hpp>
#include <utils/data_recoder.hpp>

ServerProcTask::ServerProcTask() : TimerElement(1000, "ServerProcTask") {
    _proc_log_dir = appPref.get_string_data("log.log_dir") + "/proc/";
    // Linux 中创建文件夹的函数，但是没有找到对应 -p 的参数
    if (0 != mkdir(_proc_log_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
        FATAL() << "创建文件目录失败，请检查是否有权限或足够的内存空间：" << _proc_log_dir;
    }

    _proc_log = std::stod(appPref.get_string_data("log.proc_log")) > 0;
    _proc_remote = std::stod(appPref.get_string_data("proc.is_remote")) > 0;

    Diagnose::register_server("get_proc", [&](const std::string&) {
        return _proc_text.get_data();
    });

    Diagnose::register_server("get_proc_pb", [&](const std::string&) {
        return _proc_list.get_data().SerializeAsString();
    });

    if (_proc_remote) {
        set_interval(get_interval() / 2);
        Messager::subcribe<QnxProcList>(
            "performance_result",
            [this](const QnxProcList& data) {
                _proc_list = data;
            });
    }
}

void ServerProcTask::timer_func()
{
    static int i = 0;
    std::cout << "-------------------------------: " << i++ << std::endl;
    QnxProcList list;
    if (_proc_remote) {
        if (!_proc_list.is_updated()) {
            return;
        }
        list = _proc_list.get_data();
    }else{
        ProcUtils::get_proc_list(list);
        _proc_list = list;
        Messager::publish("performance_result", list);
    }
    std::string list_str;
    proc_list_to_string(list, list_str, -1, -1);
    _proc_text = list_str;
    if (_proc_log) {
        // TODO: 修改命名规则，带有':'的文件名，不方便处理
        static std::string log_path = _proc_log_dir + AppUtil::now_date()
                + "_" + AppUtil::now_time() + ".proc";
        static DataRecoder recoder(log_path);
        recoder.record(list_str);
        recoder.save();
    }
}
