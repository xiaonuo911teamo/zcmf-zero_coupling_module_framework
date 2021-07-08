/*************************************************************************
 * proc 测试程序
/*************************************************************************/
#include <proc.h>
#include <utils/app_util.hpp>

// 命令行参数
struct CommandParams {
    std::string mode;   // 执行模式,暂时只支持local
    int64_t interval;   // 查询间隔
    int64_t output;     // 最大查询数量, 设置后将只查询前output个
    pid_t pid;          // 查询的进程id
};

const std::string help_context =    "  *** sysproc 该程序用于获取进程信息, 实时CPU使用率, 实时占用内存等 ***\n\n"
                                    " 使用方法: sysproc [arguments]\n\n"
                                    " Arguments:\n"
                                    "    -p     设置监控进程的pid\n"
                                    "    -i     设置查询间隔\n"
                                    "    -n     最多输出多少个进程\n";

int main(int argc, char** argv) {
    CommandParams cmd;
    cmd.mode = "local";
    cmd.interval = 1000;
    cmd.output = 0;
    cmd.pid = -1;
    auto opt = getopt(argc, argv, "li:n:p:");
    while (opt != -1) {
        switch (opt) {
            case 'l':
                cmd.mode = "local";
                break;
            case 'i':
                cmd.interval = safe_stoll(optarg);
                if (cmd.interval == 0) {
                    cmd.interval = 1000;
                }
                break;
            case 'n':
                cmd.output = safe_stoll(optarg);
                break;
            case 'p':
                cmd.pid = safe_stoll(optarg);
            break;
            case 'h':
            case '?':
            default:
                std::cout << help_context << std::endl;
                exit(-1);
        }
        opt = getopt(argc, argv, "li:n:p:");
    }

    QnxProcList info;

    while (true) {
#ifdef __QNXNTO__
        if (parse_procfs(info)) {
#else
        if (parse_procfs(info, cmd.pid)) {
#endif
            local_logging(info, cmd.pid, cmd.output);
        } else {
            std::cout << "parse /proc error" << std::endl;
        }
        AppUtil::sleep_ms(cmd.interval);
    }

    return 0;
}
