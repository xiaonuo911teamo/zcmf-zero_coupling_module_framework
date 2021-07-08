## utilties 介绍

utilites 本身不是一个框架内的模块, 而是一个独立的进程, 可以通过命令行参数设置监控某个进程的资源使用情况, 包括内存和各线程的CPU使用.

<font color=yellow> 同时, 提供进程方面的操作, 对不同系统的适配封装, 只需要引用`proc.h`头文件即可. 如server_proc模块就是这样做的.</font>

### 测试程序说明

对于本进程, 编译后的名称为sysproc, 该程序用于获取进程信息, 实时CPU使用率, 实时占用内存等.

使用方法: sysproc [arguments]  
```
Arguments:  
   -p     设置监控进程的pid  
   -i     设置查询间隔  
   -n     最多输出多少个进程
```
