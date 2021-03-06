
这个模块定时保存(/proc/[当前进程id])目录的快照，用于保存当前进程的内存信息和当前进程及内部所有线程的CPU信息。

如下图所示:

<dl>
<p align="center">
<img alt="进程信息" src="../../../doc/images/processing_info.png" width="480"/>
</p>
</dl>

这样的文件可以通过其他工具进行处理, 显示出进程的曲线图, 如下所示.

<dl>
<p align="center">
<img alt="进程曲线图" src="../../../doc/images/processing_curve.png" width="1080"/>
</p>
</dl>

注: 该工具可联系我付费领取.

### 模块使用说明

直接添加到 `opt/run.sh` 中的启动命令后, 执行相应指令, 即可使用此模块记录当前进程中, 所有线程的CPU使用情况以及当前进程的内存变化曲线. (单独的线程是没法计算内存的哦!)

### 模块详细设计

#### 功能要求

1. 一秒记录一次当前进程的进程信息.
2. 保存进程信息到指定目录下, 并以规范的名称命名.
3. 进程信息使用qnxproc.proto中定义的格式
4. 提供获取当前进程信息的服务 get_proc和get_proc_pb
5. 当进行跨机通信时, A机器作为运行主机, B机器作为监控主机. 应当让B机器同时可以进行落盘A机器上的进程信息. 

#### 详细设计

1. 模块分为两部分, A部分对进程信息处理的封装并提供简单使用的函数接口, B部分继承模块体系, 按1Hz的频率执行记录进程信息的任务.
2. A部分涉及对进程的处理, 在utilities模块内, 已经实现了许多处理函数, 所以A部分只是在其基础上又加了一层适配层.
3. B部分继承框架中的TimerElement体系, 重载了`timer_func`函数, 实现循环处理的操作.
4. 读取`proc.is_remote`配置, 需要从远端获取进程信息时, 则订阅`performance_result`消息, 并更新`_proc_list`, 否则, 将自己生成`_proc_list`, 然后推送`performance_result`消息.
5. 构造函数中进行配置的读取, 以及初始化处理. TODO: 后面应该尽量简化构造函数中的内容, 构造函数应当是noexcept的, 初始化, 可以重载基类中的start函数.
