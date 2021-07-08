
log模块中定义了 DEBUG/INFO/WARNING/ERROR/FATAL/DIRECT 几种输出方式，同时还会publish每个消息，
log_debug/log_info/log_warning/log_error/log_fatal/log_direct，可以通过subscribe获得这些消息。

日志的输出格式如：`[INFO tid:1988 05/15 17:51 file.cpp:348] Hello World!`
如果定义USE_GLOG，则使用glog的实现方式，否则使用iv_log中的实现。


### 使用方式

0. 引入logging.hpp, 对应库目录
1. SET_LOG_LEVEL(level)：设置日志的输出等级。Debug < INFO < WARNING < ERROR < FATAL < DIRECT
2. 使用你希望的输出方式，输出log。如INFO，ERROR_IF, DIRECT_NOT_IF

### 几种方式介绍

- DEBUG(): 用于非常细致的日志输出，可以控制调试的日志输出
- INFO(): 用于普通的日志输出, 可以是稍微重要一些的状态更新
- WARNING(): 用于警告级别的日志输出
- ERROR(): 用于一般错误的输出，即不影响程序运行的错误
- FATAL(): 用于严重错误的输出，即影响严重错误的错误
- DIRECT(): 最高级别的输出方式
- SET_LOG_LEVEL(level): 设置日志的输出等级
- DATAINFO(name, value): 用于输出一个变量的值。 
- **_IF(condition): 如果 condition==true，则输出内容
- **_IF_NOT(condition): 如果 condition==false，则输出内容

### 注意事项

1. 通常与utils模块中的TIME_LIMIT_EXEC(time_ms)连用。可以用于控制日志的输出频率。如：
```
// 每秒输出一条 Hello World!
TIME_LIMIT_EXEC(1000) {
    INFO() << "Hello World!"；
}
```
2. DEBUG/INFO/DIRECT 都是标准输出，ERROR/FATAL 是标准错误输出。