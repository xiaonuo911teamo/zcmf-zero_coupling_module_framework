## output_result_zmq 模块说明

此模块订阅日志消息, 进程信息, 并通过zmq发送出去. 

### 配置说明

对于`config/ini/zmqio.ini`而言:

```
[zmq]
server_diag_url = tcp://*:9900
input_sensor_url = tcp://192.168.0.100:9901
output_sensor_url = tcp://*:9901
input_result_url = tcp://192.168.0.100:9902
output_result_url = tcp://*:9902
```

当前模块将向本机的9902端口发送数据, 其他机器可以连接到本机器的9902端口, 获取数据.

### 使用注意事项

1. 本模块支持增加对其他信息的发送, 只是需要注意id不能相同.
2. 本模块通常和[input_result_zmq](../input_result_zmq/README.md)模块配合使用, 其中的解析也是与其对应的.
3. 如果`input_result_zmq`与`output_result_zmq`模块不能连接, 可以通过如下方式排查解决:
    - 在`output_result_zmq`运行的机器上, 执行`netstat -at | grep 9902`, 看一下是否可以得到一条对应端口的输出. 连接前只有一行**LISTEN**, 连接后会多出一行**ESTABLISHED**的信息. 
        - 如果没有输出任何信息, 需要查看配置文件zmqio.ini是不是在指定位置, 并配置正确. output_result_zmq模块有没有被加载.
        - 如果只有**LISTEN**的一行, 则本机器上的配置是没有问题的.
    - 在`input_result_zmq`运行的机器上, 需要确认配置文件`zmqio.ini`是不是在指定位置, 并配置ip地址正确. output_result_zmq模块有没有被加载.
    - 前两种都符合, 则需要进行网络上的分析. `sudo iptables -L`查看防火墙配置, INPUT或OUTPUT策略中, 是不是有将连接禁用的情况. 有的话, 需要解除禁用. 具体操作, 这里不进行赘述.