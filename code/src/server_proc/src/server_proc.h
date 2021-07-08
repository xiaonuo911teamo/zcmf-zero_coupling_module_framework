#pragma once

#include <proc.h>

class ProcUtils
{
private:
    ProcUtils();
public:
    static void get_proc_list(QnxProcList& list);

    static void get_proc_list_str(std::string& list_str);

    static void get_proc_list_str_byid(std::string& list_str, int id);

    static void get_current_proc_list_str(std::string& list_str);

};
