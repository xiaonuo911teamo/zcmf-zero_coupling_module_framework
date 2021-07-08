## server_log模块说明

server_log模块将订阅log_debug/log_info/log_warning/log_error/log_fatal/log_direct, 并将其分类写入到两个log文件中。

分类规则: 
1. log_debug&&log_info&&log_direct 写入到 *_INFO.log 文件中.
2. log_warning&&log_error&&log_fatal 写入到 *_ERROR.log 文件中.

### 使用方式

1. 正常加载模块即可, 加载方式参见 TODO: 

### 几种方式介绍

- DEBUG(): 用于非常细致的日志输出，可以控制调试的日志输出
- INFO(): 用于普通的日志输出, 可以是稍微重要一些的状态更新
- WARNING(): 用于警告级别的日志输出
- ERROR(): 用于一般错误的输出，即不影响程序运行的错误
- FATAL(): 用于严重错误的输出，即影响严重错误的错误
- DIRECT(): 最高级别的输出方式
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