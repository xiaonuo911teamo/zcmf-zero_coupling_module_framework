## utils模块说明

这个模块中集成多种工具函数，详细见如下。

### 目录说明

-- app_preference: 单例AppPreference，用于保存整个程序使用的配置，按 std::map<group.key, value> 分类保存
-- app_config: APP配置相关的函数
-- app_util: 通用工具函数
-- dl_utils: 框架模块加载方式封装
-- timer_counter: 计时工具
-- math_util: 数学相关的工具函数, 平均值等
-- json_writer: json读写工具, 尚不完整, 后续补充
-- data_reader: 文件读取, 尚不完整, 后续补充
-- data_recoder: 文件写入, 尚不完整, 后续补充

### APP配置使用方式

1. 使用 load_ini_config 读取所有配置到 appPref 中
    ```
    load_ini_config("/home/.../config/ini/*.ini");
    ```
2. 使用 get_int32 从 appPref 中获取指定参数
    ```
    int32_t log_level = AppConfig::get_int32("Log.log_level");
    ```
### app_util通用工具使用说明

1. __FOLDER__ 获取文件路径的文件夹目录, 返回值是std::string类型

2. FREQUENCE(v) 可以记录当前位置的调用频率, 频率保存在double变量v中. (v不需要外面定义, 宏定义中已有) 统计精度为3s, 即3秒内, 平均1s的调用次数.
    ```
    while (1) {
        static int i=0;
        FREQUENCE(freq);    // 注意: 上面是没有定义freq变量的
        INFO() << "current freq: " << freq;
        sleep_ms(i++);
    }
    ```
3. TIME_LIMIT_EXEC(interval) 可以控制下属代码块的执行频率. 控制间隔至少为interval(ms)
    ```
    while (1) {
        static int i=0;
        FREQUENCE(freq);    
        TIME_LIMIT_EXEC(3000) { // 上面的输出频率太高了.这里设置3s输出一次
            INFO() << "current freq: " << freq;
        }
        sleep_ms(i++);
    }
    ```
4. TIMESTAMP_LIMIT_EXEC(interval, timpstamp), 与TIME_LIMIT_EXEC类似, 可以自己设置时间等级. 如进行微秒上的精度控制.
    ```
    while (1) {
        static int i=0;
        FREQUENCE(freq);    
        TIMESTAMP_LIMIT_EXEC(3e6, AppUtil::get_current_us()) { // 代码层次上可支持us级的输出频率控制
            INFO() << "current freq: " << freq;
        }
        sleep_ms(i++);
    }
    ```
    <font color=red>注: TIME_LIMIT_EXEC和TIMESTAMP_LIMIT_EXEC, 任意组合都不能在同一个代码块中使用两次.</font>

5. sleep_ms(int value) 当前线程睡眠value毫秒; sleep_us(int value) 当前线程睡眠value微秒; 秒级的系统函数已提供

6. get_current_us/get_current_ms/get_current_sec 获取当前的(微秒/毫秒/秒)级时间戳.

7. get_file_text(const std::string& path) 读取文件path，以string输出，<font color=red>string格式限制大小<2G. </font>

8. string_trim(const std::string& str) 去掉str前后的空格和回车

9. string_split(const std::string& src, const std::string &delim) 按delim分割字符串src, <font color=red>delim是string类型的形参.</font>

10. string_sprintf(const char* format, ...) 解释格式化字符串到string

11. string_vsnprintf(const std::string& format, va_list args) 解释格式化字符串到string

12. _string_append(const std::string& file, const char* format, va_list ap) 解释格式化字符串后，追加到file中

13. string_append(const std::string& file, const char* format, ...) 解释格式化字符串后，追加到file中

14. string_load(const std::string& file, std::string& content) 读取file到content, <font color=blue>与get_file_text不同,string_load内部使用的是C语言的FILE相关函数实现, 而 get_file_text是通过C++中的ifstream实现

15. 格式转换部分, 具体查看[app_util.hpp](./app_util.hpp)

16. now_time() 返回当前时间，格式为 tm_hour-tm_min-tm_sec-ms

17. now_date() 返回当前日期，格式为 tm_year-tm_mon-tm_mday

18. get_file_list(const std::string& path, const std::string& filter = ".*") 获取当前文件夹的文件列表，忽略 . ..

19. get_file_size(const std::string& path) 返回文件大小

20. make_dir(const std::string& path) 创建文件夹, @return: 1　create success ; 2 exist ;  -1 create failed

21. make_file(const std::string& file_name) 创建文件, <font color=red>内部没有任何错误检查, 创建失败返回-1</font>

22. remove_rf(const char *dir) 删除文件或文件夹, 成功返回0, 失败返回-1

23. check_file_num(const std::string& dir_name, const std::string& filter, int32_t num_limited) 检查dir_name文件夹下面的满足筛选条件filter的文件个数是否已经超过上限值num_limited，并将超过的文件删掉
. 如：:check_file_num(".", ".*\\.ulg", 10); # 当前路径下只保存10个ulog文件，其他的都删除掉

24. copy_file(const char* src, const char* des) 拷贝文件

25. set_thread_affinity(const size_t cpu_index) 绑定当前线程到指定的CPU上, 比如将计算需求高的线程绑定在计算能力好的CPU上

### dl_utils 使用

> 具体使用方法参照 iv_task 即可.

1. get_plug_map 保存动态库名称和入口地址的map<name, ptr>
2. initial_path 初始化环境, argv[0]为运行目录, 该目录下应该有 "lib" "config/ini" 文件夹, 用于模块的读取与配置文件的装载.
3. try_load_plugin(const char* name, std::string& message) 加载name模块, 返回加载日志
4. load_plugin 对try_load_plugin 进行封装, 只输出加载错误的日志
5. run_plugin(const char* name) 调用动态库中的 run_$name 函数, 参照一般模块中plugin.cpp 文件的 RUN_PLUGIN() 函数
6. path_go_back 回退路径函数, 应该放到utils函数中

### math_util 使用

1. math::average 求数组内数组的平均值
2. math::variance 求数组内数组的方差
3. math::standar_deviation 求数组内数组的标准差

### TimerCounter 使用

TimerCounter 用于统计算法的用时，从构造函数开始计时，截止至析构函数，并输出中间运行时间

1. 定义 TimerCounter
```
// 计算输出一条log所需要的时间
{
    TimerCounter counter("INFO", true, 1000);
    INFO() << "Hello world!";
}
```  

### 待优化项

1. json_writer json文件处理, 补充完整
2. data_reader 文件读取, 补充完整
3. data_recoder 文件写入, 补充完整
4. path_go_back 回退路径函数, 应该放到utils函数中
