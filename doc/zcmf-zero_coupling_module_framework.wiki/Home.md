这是一个零耦合的模块工程架构。模块间的交互操作通过查询在静态存储区存放的注册函数的地址，传入相应参数，完成调用交互，从而达到所有模块均不互相依赖的目的。该架构适用于多任务并行执行，并且多有交互的情景，多用于C++后台服务和界面程序的底层架构。


## 本文使用的术语

| 编号 |   术语和缩写 | 解释 |
| --- | ---------  | -----|
| 1   | 模块 | 用于描述一个整体功能模块, 通常模块内都会有一个pulgin.cpp文件,去控制模块的启动和停止|

## 框架背景以及原理

有时候，我们会因为功能的划分，将一部分内容拆分成多个模块，但是由于其内部有具有一定的关联性，如相互之间的函数调用等。在单独编译各个模块的时候，仍然需要对方的头文件，以及运行时对so的依赖。这样的一种设计，我们称其为耦合设计。通常情况下，我们并不喜欢这种强制的耦合关系，它导致我们的系统依赖关系复杂，不够灵活。

所以，产生了很多低耦合，零耦合的设计，而我们的框架也是其中一种。

我们框架的最基础原理是巧妙地利用了，同一个线程具有相同的静态存储区的性质，在这块相同的区域内支持各个模块的请求与响应，从而做到各个模块间的通信。这样做的好处是，在编译时，其他模块间都不需要相互依赖了，仅需要依赖基础的头文件即可。在运行时，不会去检查对方的so是否存在，而是启动后，去静态存储区查找，如果查找无果，则通信失败。

在框架中，我们提供了一种服务机制，一种消息机制。

服务机制介绍：服务由一方提供，并将其函数地址存储在静态存储区，任意模块（包括自身）都可以作为客户从静态存储区中获得该地址，补充适当的参数即可运行。
消息机制介绍：消息由一方订阅，一方推送(可相同)。订阅方提供消息处理函数，推送方直接将消息推送出，并触发消息处理函数（可以是多个）。

不同点：处理函数的实现位置是不同的，可以引用的类成员变量也是不同的。
相同点：实际上都是在客户或者推送方去提供参数，调用处理函数执行的。


## 框架现有的功能

1. 服务机制&消息机制
   服务机制和消息机制在messager中实现, [详细介绍](https://github.com/xiaonuo911teamo/zcmf-zero_coupling_module_framework/wiki/messager-%E6%A8%A1%E5%9D%97%E8%AF%B4%E6%98%8E)
2. 线程安装的变量封装
   现已提供线程安全的变量封装以及vector/queue封装.[详细介绍](https://github.com/xiaonuo911teamo/zcmf-zero_coupling_module_framework/wiki/%E8%87%AA%E5%AE%9A%E4%B9%89%E6%95%B0%E6%8D%AE%E7%B1%BB%E5%9E%8B%E4%BB%8B%E7%BB%8D)
3. 模块线程管理
   每个模块使用独立线程，模块运行模式提供2+1的方案。触发式调度模式和定时循环式处理模式+任务池模式。[详细介绍](https://github.com/xiaonuo911teamo/zcmf-zero_coupling_module_framework/wiki/pipe-%E6%A8%A1%E5%9D%97%E8%AF%B4%E6%98%8E)
4. 标准化log输出
   提供六种等级的log输出模式，适用于不同场景，依次为DEBUG/INFO/WARNING/ERROR/FATAL/DIRECT。[详细介绍](https://github.com/xiaonuo911teamo/zcmf-zero_coupling_module_framework/wiki/log%E6%A8%A1%E5%9D%97%E8%AF%B4%E6%98%8E)
5. 类似与电信号中断机制的信号诊断功能 
   通过模拟电信号中**中断触发方式**, 对连续信号提供诊断支持.[详细介绍](https://github.com/xiaonuo911teamo/zcmf-zero_coupling_module_framework/wiki/%E8%AF%8A%E6%96%AD%E6%A8%A1%E5%9D%97%E8%AF%B4%E6%98%8E)
6. 框架配置介绍
   框架内的各个模块可以通过AppConfig中的静态函数, 获取指定类型的配置. 这些配置统一通过 DlUtils::initial_path(argv) 进行获取读入.[详细介绍](https://github.com/xiaonuo911teamo/zcmf-zero_coupling_module_framework/wiki/utils%E6%A8%A1%E5%9D%97%E8%AF%B4%E6%98%8E)
7. 进程CPU 内存实时记录
   server_proc模块可以通过实时记录/proc下的进程信息, 保存进程实时使用CPU和内存情况.[详细介绍](code/src/server_proc/README.md)
8. 时序ulog存储
   Ulogger是一种高效的时间序列数据记录格式.可以实时存储数据,生成ulog文件.通过其他软件的解析,可以绘制出数据变化的曲线图.[详细介绍](code/src/corelib/include/ulg/README.md)
9. 日志落盘
   server_log模块可以将INFO等日志信息输出到文件中.[详细介绍](code/src/server_log/README.md)
10. 使用protobuf作为通用通信格式
   [详细介绍](code/src/proto_data/README.md)

## 框架后续的发展规划

1. 框架中仍然有部分逻辑过于复杂，后续考虑进行简明化
2. 框架中仍然有部分的效率没有做到最高，后续考虑更好的优化策略
3. 框架中缺少一种自定义的异常处理机制
4. 框架中缺少单元测试模块
5. 线程调度方面有待完善
6. 获取上一条已输出日志的接口,用于自测时

## 框架使用建议

1. 尽量不要自启线程, 框架内包含对线程的调度部分, 可以满足大部分需求.

## 简单使用步骤

> 适用平台：linux
> 目标：通过server_proc模块，记录进程的CPU和内存信息

1. `git clone` 将代码迁移到本地
2. `./build.sh -nd` 下载linux-x86_64版本依赖
3. `./build.sh -j2 -nbi` 编译，并产出到output目录
4. `cd output/linux-x86_64` 进入产出目录
5. `./run.sh -r test` 执行测试运行命令，等待10秒左右，**Ctrl+C** 退出。
6. `vim log/*.log` 查看输出的log信息, 有两类INFO和ERROR. 
7. `vim log/proc/*.proc` 查看生成的cpu信息和内存信息

## 目录说明

-- build: 编译时自动生成, 里面存放临时文件
-- cmake: cmake文件的存放位置, 用于对三方库的查找
-- code: 源代码存放位置
-- doc: 文档存放位置
-- inner-depend: 存放三方库的位置
-- opt: 存放配置文件, 执行脚本的位置, 同时, 编译的产出也会放在对应的目录下
-- output: 产出的最终安装位置, 将此目录拷贝到其他机器上, 应可以直接运行.
-- build.sh: 编译脚本, 具体使用方法, 通过./build.sh -h 查看
-- integration.py: 用于下载三方库的脚本
-- LICENSE: license文件
-- README.md: 文档说明
-- version_depend.json: 版本控制文件

## 特殊情况使用指导

### 1.单个模块需要阻塞调用

有时,我们会使用到someip ros这类的通讯机制, 其主要通过回调函数完成, 但是都需要调用一个阻塞线程, 用于监听. 这时, 建议不要在模块内自启线程, 而是直接让阻塞线程在 thread_func 函数内阻塞住. 同时, 重载基类函数stop,  实现阻塞线程的退出.

