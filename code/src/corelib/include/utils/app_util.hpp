/*
整个程序通用的工具函数集合
*/
#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <memory>
#include <vector>
#include <regex>
#include <chrono>
#include <iomanip>
#include <thread>
#include <fstream>
#include <strstream>
#include <sys/time.h>
#include <assert.h>
#include <cstdarg>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef __QNXNTO__
//#include <nbutil.h>         // for getprogname()
#include <sys/neutrino.h>   // for ThreadCtl()
#include <sys/procmgr.h>    // for procmgr_ability()
#else
#include <unistd.h>
#include <sys/syscall.h>    // for syscall()
#include <cerrno>           // for program_invocation_short_name
#endif

// 获取文件路径的文件夹目录
#define __FOLDER__  std::string(__FILE__).erase(std::string(__FILE__).find_last_of('/'))

// 在一个循环调用的模块中使用FREQUENCE(freq), 可以计算出这部分的执行频率，保存在freq中
// 注： freq不需要提前声明，其在内部声明，在此部分之后，可以直接使用
#define FREQUENCE(v) static int __count = 0;\
            static uint64_t __last_time = 0;\
            static double v = 0;\
            uint64_t __time = AppUtil::get_current_ms();\
            if (__time > __last_time + 3000) { \
                v = 1000.0 * __count / (__time - __last_time);\
                __last_time = __time;\
                __count = 0;\
            }\
            __count++;
// 控制执行频率，间隔至少interval
#define TIME_LIMIT_EXEC(interval) \
            static uint64_t __last_time = 0;\
            uint64_t __time = AppUtil::get_current_ms();\
            bool do_exec = false;\
            if (__time > __last_time + interval) { \
                __last_time = __time;\
                do_exec = true;\
            }\
            if (do_exec)

#define TIMESTAMP_LIMIT_EXEC(interval, timpstamp) \
            static int64_t __last_time = 0;\
            int64_t __time = timpstamp;\
            bool do_exec = false;\
            if (abs(__time - __last_time) > interval) { \
                __last_time = __time;\
                do_exec = true;\
            }\
            if (do_exec)

// AppUtil 工具类函数的集合
class AppUtil {
public:

    static void sleep_ms(int value) {
        std::this_thread::sleep_for(std::chrono::milliseconds(value));
    }

    static void sleep_us(int value) {
        std::this_thread::sleep_for(std::chrono::microseconds(value));
    }

    // 获取当前的微秒 毫秒 秒级时间戳
    static int64_t get_current_us() {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::
                        duration_cast<std::chrono::microseconds>(now.time_since_epoch());
        return duration.count();
    }
    
    static int64_t get_current_ms() {
        return get_current_us() / 1000;
    }

    static int64_t get_current_sec() {
        return get_current_us() / 1000000;
    }

    // 读取文件path，以string输出，string格式限制大小<2G
    static std::string get_file_text(const std::string& path) {
        std::ifstream ifs(path);
        std::stringstream ss;
        if (!ifs.is_open()) {
            throw "config file open failed, path: ";
        }

        std::string line;
        while (getline(ifs, line)) {
            ss << line;
        }
        return ss.str();
    }

    // 去掉前后的空格和回车
    static std::string string_trim(const std::string& str) {
        std::string s(str);
        s.erase(0, s.find_first_not_of(" \t\n\r\v\f"));
        s.erase(s.find_last_not_of(" \t\n\r\v\f") + 1);
        return s;
    }

    // 按delim分割字符串src
    static std::vector<std::string> string_split(const std::string& src, const std::string &delim) {
        assert(!delim.empty());

        std::vector<std::string> list;
        if (src.empty()) {
            return list;
        }

        std::string s = src;
        std::string token;

        size_t pos = s.find(delim);
        while (pos < s.length()) {
            token = s.substr(0, pos);
            if (token.empty() == false) {
                list.emplace_back(token);
            }
            s = s.substr(pos + delim.length());
            pos = s.find(delim);
        }
        if (s.empty() == false) {
            list.emplace_back(s);
        }
        return list;
    }

    // 将char数组buff转成string
    static std::string char_to_string(const char* buff, const size_t buff_size) {
        std::string str(buff, buff_size);
        return str;
    }

    // 解释格式化字符串到string
    static std::string string_sprintf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        std::string buff = string_vsnprintf(format, args);
        va_end(args);
        return buff;
    }

    // 解释格式化字符串到string
    static std::string string_vsnprintf(const std::string& format, va_list args) {
        // try first
        int32_t try_size = 512;
        char try_buff[try_size];
        memset(try_buff, 0, try_size);

        std::string res;
        int32_t size_needed = std::vsnprintf(try_buff, try_size, format.c_str(), args);
        if (size_needed < try_size) {
            res = try_buff;
        } else {
            // try again
            char buff_needed[size_needed+1];
            memset(buff_needed, 0, size_needed+1);
            int32_t size = std::vsnprintf(buff_needed, size_needed+1, format.c_str(), args);
            if (size >= 0) {
                res = std::string(buff_needed);
            } else {
                res = std::string("");
            }
        }
        return res;
    }

    // 解释格式化字符串后，追加到file中
    static void _string_append(const std::string& file, const char* format, va_list ap) {
        assert(!file.empty());

        FILE* fp = nullptr;
        if (file == "stdout") {
            fp = stdout;
        } else if (file == "stderr") {
            fp = stderr;
        } else {
            fp = fopen(file.c_str(), "a");
        }

        if (fp != nullptr) {
            std::vfprintf(fp, format, ap);

            if ((file != "stdout") && (file != "stderr")) {
                fclose(fp);
                fp = nullptr;
            }
        }
    }

    static void string_append(const std::string& file, const char* format, ...) {
        assert(!file.empty());

        va_list ap;
        va_start(ap, format);
        _string_append(file, format, ap);
        va_end(ap);
    }

    // 读取file到content
    static bool string_load(const std::string& file, std::string& content) {
        assert(!file.empty());

        FILE* fp = fopen(file.c_str(), "rb");
        if (fp == nullptr) {
            return false;
        }

        // check size
        fseek(fp, 0, SEEK_END);
        size_t bufsize = ftell(fp);
        rewind(fp);

        // malloc buffer
        char buf[bufsize + 1];
        memset(buf, 0, bufsize + 1);
        size_t res = fread(buf, 1, bufsize, fp);
        fclose(fp);

        if (res <= 0) {
            return false;
        }

        // to string
        content = char_to_string(buf, bufsize);
        return true;
    }

    // 格式转换部分
    static int32_t bool_to_int(const bool b) {
        int32_t i = 0;
        if (b == true) {
            i = 1;
        }
        return i;
    }
    // zero_as_false: true i不为0，则返回true
    // zero_as_false: false i恒为0，则返回true
    static bool int_to_bool(const int32_t i, bool zero_as_false) {
        bool res = false;
        if (zero_as_false && (i != 0)) {
            res = true;
        } else if ((!zero_as_false) && (i == 0)) {
            res = true;
        }
        // MISRA C++ 2008: 6-6-5
        return res;
    }

    static int32_t safe_stoi(const std::string& str) {
        int32_t res = 0;
        try {
            res = std::stoi(str);
        } catch (std::out_of_range& e) {
            res = 0;
        } catch (std::invalid_argument& e) {
            res = 0;
        }
        // MISRA C++ 2008: 6-6-5
        return res;
    }

    static int64_t safe_stoll(const std::string& str) {
        int64_t res = 0;
        try {
            res = std::stoll(str);
        } catch (std::out_of_range& e) {
            res = 0;
        } catch (std::invalid_argument& e) {
            res = 0;
        }
        // MISRA C++ 2008: 6-6-5
        return res;
    }

    static double safe_stod(const std::string& str) {
        double res = 0.0;
        try {
            res = std::stod(str);
        } catch (std::out_of_range& e) {
            res = 0.0;
        } catch (std::invalid_argument& e) {
            res = 0.0;
        }
        // MISRA C++ 2008: 6-6-5
        return res;
    }

    static std::vector<double> safe_stodv(const std::string& s) {
        std::vector<double> res;
        int left_i = s.find('[');
        int right_i = s.find(']');
        if (left_i >= 0 && right_i >= 0 && left_i < right_i) {
            std::string sub = s.substr(left_i + 1, right_i - left_i - 1);
            std::vector<std::string> str_vec = string_split(sub, ",");
            for (auto v : str_vec) {
                res.push_back(safe_stod(v));
            }
        }
        return res;
    }

    // 返回当前时间，格式为 tm_hour-tm_min-tm_sec-ms
    static std::string now_time() {
        auto now = std::chrono::system_clock::now();
        auto _time = std::chrono::system_clock::to_time_t(now);
        struct tm _tm;
        std::string time_str;
        if (localtime_r(&_time, &_tm) != nullptr) {
            auto duration = std::chrono::
            duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
            int64_t ms = duration.count() % 1000;
            time_str = string_sprintf("%02d-%02d-%02d-%03d", _tm.tm_hour, _tm.tm_min, _tm.tm_sec, ms);
        } else {
            time_str = "00-00-00";
        }
        return time_str;
    }

    // 返回当前日期，格式为 tm_year-tm_mon-tm_mday
    static std::string now_date() {
        auto now = std::chrono::system_clock::now();
        auto _time = std::chrono::system_clock::to_time_t(now);
        struct tm _tm;
        std::string date_str;
        if (localtime_r(&_time, &_tm) != nullptr) {
            date_str = string_sprintf("%04d-%02d-%02d", _tm.tm_year + 1900, _tm.tm_mon + 1, _tm.tm_mday);
        } else {
            date_str = "1970-01-01";
        }
        return date_str;
    }

    // 获取当前文件夹的文件列表，忽略 . ..
    static std::vector<std::string> get_file_list(const std::string& path, const std::string& filter = ".*")
    {
        char cur_dir[] = ".";
        char up_dir[] = "..";
        std::vector<std::string> file_list;
        DIR *dirp;
        struct dirent *dp;
        dirp = opendir(path.c_str());
        while ((dp = readdir(dirp)) != NULL) {
            if(std::regex_match(dp->d_name, std::regex(filter))) {
                //忽略 . 和 ..
                if ((0 == strcmp(cur_dir, dp->d_name)) || (0 == strcmp(up_dir, dp->d_name)) ) {
                    continue;
                }
                file_list.push_back(std::string(dp->d_name ));
            }
        }
        (void) closedir(dirp);
        std::sort(file_list.begin(), file_list.end(), [](const std::string& s1, const std::string& s2) {
            return s1.compare(s2) < 0;
        });
        return file_list;
    }

    // 返回文件大小
    static int64_t get_file_size(const std::string& path) {
        std::ifstream in(path);
        in.seekg(0, std::ios::end);
        return in.tellg();
    }

    /*
    @return: 1　create success ; 2 exist ;  -1 create failed
    */
    static int make_dir(const std::string& path) {
        if (access(path.c_str(), 0) == -1) {
            int ret = mkdir(path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR 
                                            | S_IRGRP | S_IWGRP | S_IXGRP 
                                            | S_IROTH | S_IXOTH );
            if (ret == -1) {
                return -1;
            } else {
                return 1;
            }
        }
        return 2;
    }

    static int make_file(const std::string& file_name) {
        mode_t f_attrib = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH; 
        int fd = open(file_name.c_str (), O_RDWR | O_CREAT , f_attrib);
        return fd;
    }

    // 删除文件或文件夹, 成功返回0, 失败返回-1
    static int remove_rf(const char *dir) {
        char cur_dir[] = ".";
        char up_dir[] = "..";
        DIR *dirp;
        struct dirent *dp;
        struct stat dir_stat;

        if (access(dir, F_OK) != 0) { //文件不存在
            return 0;
        }

        if (stat(dir, &dir_stat) < 0) { //获取文件状态
            return -1;
        }

        if (S_ISREG(dir_stat.st_mode)) {  //普通文件
            std::remove(dir);
        } else if (S_ISDIR(dir_stat.st_mode)) {   //目录文件
            dirp = opendir(dir);
            while ((dp=readdir(dirp)) != NULL) {
                //忽略 . 和 ..
                if ((0 == strcmp(cur_dir, dp->d_name)) || (0 == strcmp(up_dir, dp->d_name)) ) {
                    continue;
                }
                std::string dir_name = std::string(dir) + "/" + std::string(dp->d_name);
                remove_rf(dir_name.c_str());
            }
            closedir(dirp);
            rmdir(dir);
        } else {
            return -1;  
        }
        return 0;
    }

    // 检查dir_name文件夹下面的满足筛选条件filter的文件个数是否已经超过上限值num_limited，并将超过的文件删掉
    // 如：:check_file_num(".", ".*\\.ulg", 10); # 当前路径下只保存10个ulog文件，其他的都删除掉
    static void check_file_num(const std::string& dir_name, const std::string& filter, int32_t num_limited) {
        std::vector<std::string> files = get_file_list(dir_name, filter);
        int file_size = files.size();

        for (int index = 0; file_size > num_limited; index++, file_size--) {
            std::string file =  dir_name + "/" + files[index];
            remove_rf(file.c_str());
        }
    }

    static int copy_file(const char* src, const char* des) {
        int ret = 0;
        FILE* psrc = NULL, *pdes = NULL;
        psrc = fopen(src, "r");
        pdes = fopen(des, "w+");
    
        if (psrc && pdes) {
            int nlen = 0;
            char sz_buf[1024] = {0};
            while((nlen = fread(sz_buf, 1, sizeof(sz_buf), psrc)) > 0) {
                fwrite(sz_buf, 1, nlen, pdes);
            }
            fflush(pdes);
        } else {
            ret = -1;
        }
            
        if (psrc) {
            fclose(psrc);
        }
        if (pdes) {
            fclose(pdes);
        }
        return ret;
    }
    // 绑定当前线程到指定的CPU上
    // 比如将计算需求高的线程绑定在计算能力好的CPU上
    static int set_thread_affinity(const size_t cpu_index) {
    #ifdef __QNXNTO__
        // QNX
        if (cpu_index > 0) {
            int32_t mask = 1;
            size_t index = cpu_index - 1;
            if (index > 0) {
                mask = mask << index; // bit pos = cpu index
            }
            return ThreadCtl(_NTO_TCTL_RUNMASK, (void*)(intptr_t)mask); // must be void*
        }

    #else
        // Linux
        if (cpu_index > 0) {
            size_t cpus = std::thread::hardware_concurrency();
            cpu_set_t set;
            CPU_ZERO(&set);
            size_t index = cpu_index - 1; // cpu index from 0
            CPU_SET((index % cpus), &set);
            return pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
        }

    #endif
        return 0;
    }
};

