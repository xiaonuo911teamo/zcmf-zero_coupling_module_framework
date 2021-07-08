#pragma once

#ifdef __QNXNTO__
#include <devctl.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>
#else
#include <bits/posix2_lim.h>
#endif

#include <data/qnxproc.pb.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/procfs.h>
#include <sys/mman.h>
#include <string>
#include <stdint.h>
#include <sys/time.h>
#include <ctime>
#include <thread>

struct ProcStatus {
    int64_t pid;
    int64_t tid;
    int64_t cpuid;
    int64_t priority;
    std::string policy;
    int64_t memory;
    int64_t sutime;
    int64_t used;
    std::string proc_name;
    std::string thread_name;
};

using ProcList = std::map<std::string, ProcStatus>;

int64_t safe_stoll(const std::string &str);

bool parse_threads(const int32_t pid, const std::string& proc_name, const int64_t proc_mem, ProcList& list);

std::string get_proc_exefile(const int32_t pid);

int64_t get_proc_mem(const int32_t pid);

int32_t get_cpu_num();

int64_t get_mem_avail();

#ifdef __QNXNTO__
bool parse_procfs(QnxProcList& info);
#else
bool parse_procfs(QnxProcList& info, pid_t pid = -1);
#endif

std::string short_proc_name(const std::string& proc_name);

void local_logging(const QnxProcList& info, pid_t pid, const int64_t output_size);

void proc_list_to_string(const QnxProcList& info, std::string& proc_str, pid_t pid, const int64_t output_size);
