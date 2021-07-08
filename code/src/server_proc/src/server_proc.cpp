#include "server_proc.h"
#include <diag/diagnose.hpp>
#include <utils/app_preference.hpp>

void ProcUtils::get_proc_list(QnxProcList &list) {
    parse_procfs(list);
}

void ProcUtils::get_proc_list_str(std::string &list_str) {
    QnxProcList list;
    parse_procfs(list);
    proc_list_to_string(list, list_str, -1, -1);
}

void ProcUtils::get_proc_list_str_byid(std::string &list_str, int id) {
    QnxProcList list;
    parse_procfs(list);
    proc_list_to_string(list, list_str, id, -1);
}

void ProcUtils::get_current_proc_list_str(std::string &list_str) {
    QnxProcList list;
    parse_procfs(list);
    proc_list_to_string(list, list_str, getpid(), -1);
}

