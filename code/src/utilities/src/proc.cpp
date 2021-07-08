#include <proc.h>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <stdarg.h>
#include <utils/app_util.hpp>
#include <log/logging.h>

// 获取 topic 中主体的名字
// 如: [iv_task] -> iv_task  # 去掉特殊字符
std::string thread_name_filter(const std::string& topic)
{
    std::string rostopic;
    for (auto& c : topic) {
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') {
            rostopic += c;
        }
    }
    return rostopic;
}

int64_t safe_stoll(const std::string &str)
{
    int64_t res = 0;
    try
    {
        res = std::stoll(str);
    }
    catch (std::out_of_range &e)
    {
        res = 0;
    }
    catch (std::invalid_argument &e)
    {
        res = 0;
    }
    // MISRA C++ 2008: 6-6-5
    return res;
}

// 当前线程阻塞 ms 毫秒
// 注: 多线程执行时,可能导致有略微的延迟
void sleep_ms(const int64_t ms)
{
    auto now = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_until(now + std::chrono::milliseconds(ms));
}

// 解析格式化字符串到 string
std::string string_vsnprintf(const std::string& format, va_list args) {
    // try first
    int32_t try_size = 512;
    char try_buff[try_size];
    memset(try_buff, 0, try_size);

    std::string res;
    int32_t size_needed = std::vsnprintf(try_buff, try_size, format.c_str(), args);
    if (size_needed < try_size) {
        res.assign(try_buff, size_needed);
    } else {
        // try again
        char buff_needed[size_needed+1];
        memset(buff_needed, 0, size_needed+1);
        int32_t size = std::vsnprintf(buff_needed, size_needed+1, format.c_str(), args);
        if (size >= 0) {
            res.assign(buff_needed, size_needed);
        } else {
            res.assign("", 0);
        }
    }

    // MISRA C++ 2008: 6-6-5
    return res;
}
// 同上
std::string string_sprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::string buff = string_vsnprintf(format, args);
    va_end(args);
    return buff;
}

// 返回当前的毫秒时间戳
int64_t now_ms()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto time_point = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = time_point.time_since_epoch();
    return epoch.count();
}

// system clock now in yyyy-mm-dd.hh:mm:ss.ms
std::string now_datetime_ms()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    int64_t ms = tv.tv_usec / 1000;
    if (ms >= 1000)
    {
        ms -= 1000;
        tv.tv_sec++;
    }

    struct tm _tm;
    std::string datetime_str;
    if (localtime_r(&tv.tv_sec, &_tm) != nullptr)
    {
        datetime_str = string_sprintf("%04d-%02d-%02d.%02d:%02d:%02d.%03ld",
                                      _tm.tm_year + 1900, _tm.tm_mon + 1, _tm.tm_mday,
                                      _tm.tm_hour, _tm.tm_min, _tm.tm_sec, ms);
    }
    else
    {
        datetime_str = "1970-01-01.00:00:00.000";
    }

    return datetime_str;
}

class GlobalVariable {
public:
    ProcList prev_list;
};
GlobalVariable g;

//////////////////////////////////////////////////////////////////////////////////
// parse qnx thread info
std::string get_thread_name(const pid_t pid, const pthread_t tid) {
    std::string name = std::to_string(tid);
    if ((pid == 0) || (tid == 0)) {
        return name;
    }

#ifdef __QNXNTO__
    char threadname[_NTO_THREAD_NAME_MAX];
    int ret = __getset_thread_name(pid, tid, nullptr, -1, threadname, _NTO_THREAD_NAME_MAX);
    if ((ret == EOK) && (strlen(threadname) > 0)) {
        name = threadname;
    }

#else
    std::string line;
    std::vector<std::string> tokens;
    std::string stat_file = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(tid) + "/stat";
    std::ifstream stream(stat_file);
    if (stream.is_open()) {
        std::getline(stream, line);
        std::istringstream linestream(line);
        std::string token;
        while(linestream >> token) {
            tokens.push_back(token);
        }
    }
    stream.close();
    if (tokens.size() > 1) {
    //TODO: name = name.empty() ? "" : name.substr(1, name.size() - 2);
        name = tokens[1];
    }
#endif

    return name;
}

bool parse_threads(const int32_t pid, const std::string& proc_name, const int64_t proc_mem, ProcList& list) {
    assert(pid > 0);

#ifdef __QNXNTO__
    std::string proc_ctl = string_sprintf("/proc/%d/ctl", pid);
    int fd = open(proc_ctl.c_str(), O_RDONLY);
    if (fd == -1) {
        return false;
    }

    procfs_status status;
    status.tid = 1;
    while (true) {
        if (devctl(fd, DCMD_PROC_TIDSTATUS, &status, sizeof(status), 0) != EOK) {
            break; // finish
        } else {
            ProcStatus proc;
            proc.pid            = status.pid;
            proc.tid            = status.tid;
            proc.cpuid          = status.last_cpu;
            proc.priority       = unsigned(status.priority);

            switch (unsigned(status.policy)) {
                case SCHED_FIFO:        proc.policy = "F"; break;
                case SCHED_RR:          proc.policy = "R"; break;
                case SCHED_OTHER:       proc.policy = "O"; break;
                case SCHED_SPORADIC:    proc.policy = "S"; break;
                default: proc.policy = "U";
            }

            proc.memory         = proc_mem;
            proc.sutime         = status.sutime;
            proc.used           = 0; // update late
            proc.proc_name      = proc_name;
            proc.thread_name    = get_thread_name(status.pid, status.tid);

            std::string key = string_sprintf("%d-%d-%ld", status.pid, status.tid, status.start_time);
            list[key] = proc;

            status.tid++;
        }
    }
    close(fd);
#endif

    return true;
}

// 获取pid进程的启动指令.一般在退出时打印. 可以很快捷的看到是不是启动命令的错误
std::string get_proc_exefile(const int32_t pid) {
    assert(pid > 0);
    char buf[256];
    memset(buf, 0, 256);
#ifdef __QNXNTO__
    std::string proc_exefile = string_sprintf("/proc/%d/exefile", pid);
    FILE* fp = fopen(proc_exefile.c_str(), "rb");
    if (fp != nullptr) {
        fread(buf, 1, 256, fp);
        fclose(fp);
    }
    return std::string(buf);
#else
    std::string cmd;
    std::ifstream stream("/proc/" + std::to_string(pid) + "/cmdline");
    if (stream.is_open()) {
      std::getline(stream, cmd);
    }
    stream.close();
    return cmd;
#endif
}

// 获取pid进程当前占用的内存
int64_t get_proc_mem(const int32_t pid) {
    assert(pid > 0);
    int64_t proc_mem = 0;

#ifdef __QNXNTO__
    std::string proc_as = string_sprintf("/proc/%d/as", pid);
    int fd = open(proc_as.c_str(), O_RDONLY);
    if (fd == -1) {
        return 0;
    }

    int32_t map_count = 0;
    if (devctl(fd, DCMD_PROC_MAPINFO, 0, 0, &map_count) != EOK) {
        close(fd);
        return 0;
    }

    procfs_mapinfo* map = nullptr;
    size_t map_size = sizeof(*map) * map_count;

    map = (procfs_mapinfo*)malloc(map_size);
    if (map == nullptr) {
        close(fd);
        return 0;
    }

    if (devctl(fd, DCMD_PROC_MAPINFO, map, map_size, &map_count) != EOK) {
        free(map);
        close(fd);
        return 0;
    }

    struct MapDebug {
		procfs_debuginfo debug;
		char path[1024];
	} map_debug;

    for (int32_t i = 0; i < map_count; ++i) {
        map_debug.debug.vaddr = map[i].vaddr;
        if (devctl(fd, DCMD_PROC_MAPDEBUG, &map_debug, sizeof(map_debug), 0) < 0) {
            continue;
        }
        if (map[i].flags & MAP_SYSRAM) {
            proc_mem += map[i].size;
        }
    }

    free(map);
    close(fd);
#else
    std::string line;
    std::string key;
    std::string value;
    float mem1 = 0;
    float mem2 = 0;
    std::ifstream filestream("/proc/" + std::to_string(pid) +  "/status");
    if (filestream.is_open()) {
        while (std::getline(filestream, line)) {
            std::istringstream linestream(line);
            linestream >> key >> value;
            if (key == "VmData:") {
                mem1 = stof(value);
                mem2 = mem1 * 1024;
            }
        }
    }
    filestream.close();
    return mem2;

#endif
    return proc_mem;
}

// 获取CPU数量
int32_t get_cpu_num() {
    int32_t cpus = 0;
#ifdef __QNXNTO__
    cpus = _syspage_ptr->num_cpu;
#else
    cpus = sysconf( _SC_NPROCESSORS_CONF);
#endif
    return cpus;
}

int64_t get_mem_total() {
    uint64_t mem = 0;
#ifdef __QNXNTO__
    char* str = SYSPAGE_ENTRY(strings)->data;
    struct asinfo_entry* as = SYSPAGE_ENTRY(asinfo);

    for (uint64_t i = _syspage_ptr->asinfo.entry_size / sizeof(*as); i > 0; --i) {
        if (strcmp(&str[as->name], "ram") == 0) {
            mem += as->end - as->start + 1;
        }
        ++as;
    }
#endif
    return mem;
}

// 获取RAM剩余空间
int64_t get_mem_avail() {
    /*
    The amount of free RAM is the size of "/proc"
    https://stackoverflow.com/questions/24040046/find-amount-of-installed-memory-on-qnx-system
    */

    struct stat buf;
    if (stat("/proc", &buf) != -1) {
        return buf.st_size;
    } else {
        return 0;
    }
}

#ifndef __QNXNTO__
// 获取系统上电, 到现在的时间
long uptime() {
    long system_time = 0;
    long idle_time = 0;
    std::string line;
    std::vector<std::string> tokens;
    std::ifstream stream("/proc/uptime");
    if (stream.is_open()) {
        std::getline(stream, line);
        std::istringstream linestream(line);
        linestream >> system_time >> idle_time;
    }
    stream.close();
    return system_time;
}

// 获取内存情况
// @fmemtotal 总内存
// @fmemfree  表示系统尚未使用的内存
// @fmemvalid 真正的系统可用内存, 包括一些可以回收的内存
// https://zhuanlan.zhihu.com/p/145524701
void memory_utilization(float& fmemtotal, float& fmemfree, float& fmemvalid) {
  std::string memtotal;
  std::string memfree;
  std::string memavail;
  std::string membufs;
  std::string key, value;
  std::string line;
  std::ifstream stream("/proc/meminfo");
  if (stream.is_open()) {
      while (std::getline(stream, line)) {
    std::istringstream linestream(line);
     while (linestream >> key >> value) {
        if (key == "MemTotal:") {
          fmemtotal = std::stof(value);
        } else if (key == "MemFree:") {
          fmemfree = std::stof(value);
        } else if (key == "MemAvailable:") {
            fmemvalid = std::stof(value);
        }
     }
    }
  }
  stream.close();
}

// 获取总的CPU使用率
float total_cpu_usage() {
    std::ifstream stream("/proc/stat");
    int user = 0;
    int nice = 0;
    int system = 0;
    int idle = 0;
    int iowait = 0;
    int irq = 0;
    int softirq = 0;
    int steal = 0;
    std::string str;
    stream >> str;
    stream >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    double usage = user + nice + system +irq + softirq + steal;
    static double last_usage = 0;
    long time = AppUtil::get_current_ms();
    static long last_time = 0;
    double retval = 0.1f * (last_usage - usage) / (last_time - time) * sysconf(_SC_CLK_TCK);;
    last_usage = usage;
    last_time = time;
    return retval;
}

// 获取pid进程的tid线程的调度优先级和运行的cpu id
void get_priority_and_core(pid_t pid, pid_t tid, int & prio, int& core) {
    std::string line;
    std::vector<std::string> tokens;
    std::string stat_file;
    stat_file = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(tid) + "/stat";
    std::ifstream stream(stat_file);
    if (stream.is_open()) {
      std::getline(stream, line);
      std::istringstream linestream(line);
      std::string token;
      while(linestream >> token) {
          tokens.push_back(token);
      }
    }
    stream.close();
    if (tokens.size() > 38) {
        prio = std::stod(tokens[17]);
        core = std::stod(tokens[38]);
    }
}

// 返回线程或进程的cpu利用率
// 获取进程的CPU利用率时, tid直接使用默认值
float cpu_utilization(pid_t pid, pid_t tid = -1) {
    std::string line;
    std::vector<std::string> tokens;
    std::string stat_file;
    bool is_thread = (tid > 0);
    if (is_thread) {
        stat_file = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(tid) + "/stat";
    }else {
        stat_file = "/proc/" + std::to_string(pid) + "/stat";
    }
    std::ifstream stream(stat_file);
    if (stream.is_open()) {
        std::getline(stream, line);
        std::istringstream linestream(line);
        std::string token;
        while(linestream >> token) {
            tokens.push_back(token);
        }
        stream.close();
    }
    float cpu_usage = 0;
    if (tokens.size() > 14) {
        long utime = stol(tokens[13]);
        long stime = stol(tokens[14]);
        long ustime = utime + stime;
        long time = AppUtil::get_current_ms();
        if (is_thread) {
            static std::map<pid_t, long> last_ustime_map;
            static std::map<pid_t, long> last_time_map;
            cpu_usage =0.1f * (last_ustime_map[tid] - ustime) / (last_time_map[tid] - time) * sysconf(_SC_CLK_TCK);
            last_ustime_map[tid] = ustime;
            last_time_map[tid] = time;
        } else {
            static long last_ustime = 0;
            static long last_time = 0;
            cpu_usage =0.1f * (last_ustime - ustime) / (last_time - time) * sysconf(_SC_CLK_TCK);
            last_ustime = ustime;
            last_time = time;
        }
    }
    return cpu_usage;
}

#endif
#ifdef __QNXNTO__
bool parse_procfs(QnxProcList& info) {
    DIR *dir = opendir("/proc");
    if (dir == nullptr) {
        return false;
    }

    ProcList next_list;
    int64_t cpu_total = 0;

    // get current status
    dirent* dirent = nullptr;
    while ((dirent = readdir(dir)) != nullptr) {
        if (isdigit(dirent->d_name[0])) {
            int32_t pid = atoi(dirent->d_name);
            if (pid <= 0) {
                WARNING() << "pid <= 0: pid " << pid;
                continue;
            }
            std::string proc_name = get_proc_exefile(pid);
            int64_t proc_mem = get_proc_mem(pid);
            parse_threads(pid, proc_name, proc_mem, next_list);
        }
    }
    closedir(dir);

    // merge and update
    ProcList temp_list;
    for (auto& proc : next_list) {
        auto iter = g.prev_list.find(proc.first);
        if (iter != g.prev_list.end()) {
            // exist proc, update sutime
            proc.second.used = proc.second.sutime - iter->second.sutime;
        } else {
            // new proc
            proc.second.used = proc.second.sutime;
        }

        temp_list.insert(std::make_pair(proc.first, proc.second));
        cpu_total += proc.second.used;

        // drop exited proc
    }

    // replace prev list
    g.prev_list = temp_list;

    // convert g.prev_list to QnxProcList
    info.Clear();

    info.set_timestamp(now_ms());
    int32_t cpus = get_cpu_num();
    int64_t cpu_user = 0;
    int64_t cpu_kernel = 0;
    for (auto& proc : g.prev_list) {
        if ((proc.second.pid == 1) && (proc.second.tid <= cpus)) {
            // qnx idle
            info.add_cpu_idle((float)proc.second.used / (float)(cpu_total / cpus));
        } else {
            // user/kernel times
            if (proc.second.pid == 1) {
                cpu_kernel += proc.second.used;
            } else {
                cpu_user += proc.second.used;
            }

            // tasks
            auto p = info.add_proc_list();
            p->set_pid(proc.second.pid);
            p->set_tid(proc.second.tid);
            p->set_cpuid(proc.second.cpuid);
            p->set_priority(proc.second.priority);
            p->set_policy(proc.second.policy);
            p->set_mem(proc.second.memory);
            p->set_cpu_used((float)proc.second.used / (float)cpu_total);
            std::string sproc_name = short_proc_name(proc.second.proc_name).c_str();
            p->set_proc_name(sproc_name);
            p->set_thread_name(thread_name_filter(proc.second.thread_name));
        }
    }

    // user/kernel
    info.set_cpu_user((float)cpu_user / (float)cpu_total);
    info.set_cpu_kernel((float)cpu_kernel / (float)cpu_total);

    // mem avail/total
    info.set_mem_avail(get_mem_avail());
    info.set_mem_total(get_mem_total());
#else
// 将pid进程信息填入info
bool parse_procfs(QnxProcList& info, pid_t pid) {
    info.Clear();
    pid = pid != -1 ? pid : getpid();
    float fmemtotal = 0;
    float fmemfree = 0;
    float fmemvalid = 0;
    memory_utilization(fmemtotal, fmemfree, fmemvalid);
    info.set_mem_avail(fmemvalid * 1024);
    info.set_mem_total(fmemtotal * 1024);
    float cpu_usage = total_cpu_usage();
    info.set_cpu_user(cpu_usage);
    info.set_cpu_kernel(0);
    info.add_cpu_idle(1 * get_cpu_num() - cpu_usage);
    info.set_timestamp(AppUtil::get_current_ms());
    DIR *dir = opendir(("/proc/" + std::to_string(pid) + "/task").c_str());
    if (dir != nullptr) {
        dirent* dirent = nullptr;
        while ((dirent = readdir(dir)) != nullptr) {
            if (isdigit(dirent->d_name[0])) {
                int32_t tid = atoi(dirent->d_name);
                std::string proc_name = get_proc_exefile(pid);
                int64_t proc_mem = get_proc_mem(pid);
                float cpu = cpu_utilization(pid, tid);
                auto p = info.add_proc_list();
                p->set_pid(pid);
                p->set_tid(tid);
                p->set_mem(proc_mem);
                p->set_cpu_used(cpu);
                std::string thread_name = get_thread_name(pid, tid);
                std::string sproc_name = short_proc_name(proc_name).c_str();
                bool ptname_is_equal = thread_name == sproc_name;
                bool ptid_is_equal = pid == tid;
                thread_name = ptname_is_equal && !ptid_is_equal ? thread_name + std::to_string(tid) : thread_name;
                p->set_proc_name(sproc_name);
                p->set_thread_name(thread_name_filter(thread_name));
                int prio = 0;
                int core = -1;
                get_priority_and_core(pid, tid, prio, core);
                p->set_cpuid(core);
                p->set_priority(prio);
            }
        }
    }
    closedir(dir);
#endif
    // sort by load
    std::sort(
        info.mutable_proc_list()->begin(),
        info.mutable_proc_list()->end(),
        [](const QnxProcStatus& a, const QnxProcStatus& b) {
            return a.cpu_used() > b.cpu_used();
        });

    return true;
}

// 获取位于 '/' 之后的程序名  
std::string short_proc_name(const std::string& proc_name) {
    auto pos = proc_name.find_last_of('/');
    if (pos != std::string::npos) {
        return proc_name.substr(pos + 1);
    } else {
        return proc_name;
    }
}

// 将pid进程的QnxProcList信息输出到proc_str中, 最多处理行数
// 当pid小于等于0时, 输出所有的进程信息
void proc_list_to_string(const QnxProcList& info, std::string& proc_str, pid_t pid, const int64_t output_size) {
    char buffer[2000];
    std::stringstream ss;
    // timestamp
    sprintf(buffer, "\n%s %ld\n", now_datetime_ms().c_str(), info.timestamp());
    ss << buffer;
    // cpu
    sprintf(buffer, "cpu user/kernel %.2f%% %.2f%%\n",
        100 * info.cpu_user(), 100 * info.cpu_kernel());
    ss << buffer;

    sprintf(buffer, "cpu idle");
    ss << buffer;
    auto idle_list = info.cpu_idle();
    for (auto iter = idle_list.begin(); iter != idle_list.end(); ++iter) {
        sprintf(buffer, " %.2f%%", 100 * (*iter));
        ss << buffer;
    }
    sprintf(buffer, "\n");
    ss << buffer;

    // mem
    sprintf(buffer, "mem avail/total %ldM %ldM\n\n",
        info.mem_avail() / 1048576, // 1024*1024
        info.mem_total() / 1048576);
    ss << buffer;

    // proc list
    sprintf(buffer, "      pid   tid     cpu     mem prio core task (thread)\n");
    ss << buffer;
    int64_t i = 0;
    auto proc_list = info.proc_list();
    for (auto iter = proc_list.begin(); iter != proc_list.end(); ++iter) {
        if (pid > 0 && iter->pid() != pid) {
            continue;
        }
        sprintf(buffer,
            "%9ld %5ld %6.2f%% %6ldK %3ld%s %4ld %s (%s)\n",
            iter->pid(), iter->tid(), 100 * iter->cpu_used(),
            iter->mem() / 1024, iter->priority(), iter->policy().c_str(), iter->cpuid(),
            short_proc_name(iter->proc_name()).c_str(), iter->thread_name().c_str());
        ss << buffer;
        ++i;
        if ((output_size > 0) && (i >= output_size)) {
            break;
        }
    }
    proc_str = ss.str();
}

// 本地输出logging, 使用 std::cout
// @proc_list_to_string
void local_logging(const QnxProcList& info, pid_t pid, const int64_t output_size) {
    std::string list_str;
    proc_list_to_string(info, list_str, pid, output_size);
    std::cout << list_str;
}
