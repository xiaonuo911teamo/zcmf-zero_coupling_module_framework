# ULogger

## 简介
Ulogger是一种高效的时间序列数据记录格式

```

----------------------
|       Header       |
----------------------
|    Definitions     |
----------------------
|        Data        |
----------------------

```


数据以*.ulg格式保存

## 使用步骤

## 1.**包含头文件**
```
//home/nvidia/vehicle/l3-localization/src/localization/src/ulogger
#include "../../ulg/ulg.h"
//请按照相对路径修改
```
## 2.**注册数据类型**(请在各个模块的**类构造函数**或类构造函数里调用的**初始化类成员函数**中注册)
```
        Ulogger::instance()->register_time_series_data("double", "roll");
        Ulogger::instance()->register_time_series_data("double", "pitch");
        Ulogger::instance()->register_time_series_data("double", "yaw");
        Ulogger::instance()->register_time_series_data("double", "velocity_e");
        Ulogger::instance()->register_time_series_data("double", "velocity_n");
        Ulogger::instance()->register_time_series_data("double", "velocity_u");
        Ulogger::instance()->register_time_series_data("double", "position_e");
        Ulogger::instance()->register_time_series_data("double", "position_n");
        Ulogger::instance()->register_time_series_data("double", "position_u");
        Ulogger::instance()->register_time_series_data("double", "gyro_bias_x");
        Ulogger::instance()->register_time_series_data("double", "gyro_bias_y");
        Ulogger::instance()->register_time_series_data("double", "gyro_bias_z");
        Ulogger::instance()->register_time_series_data("double", "accel_bias_x");
        Ulogger::instance()->register_time_series_data("double", "accel_bias_y");
        Ulogger::instance()->register_time_series_data("double", "accel_bias_z");
        Ulogger::instance()->register_time_series_data("double", "odom_scale");
```
## 3.**周期写入数据**

### 写入时间序列数据：（数据需要带时间戳us单位）
```
        Ulogger::instance()->write_data("roll", _state_struct.time_us, _state_struct.quat.toRotationMatrix().to_euler_rpy().x());
        Ulogger::instance()->write_data("pitch", _state_struct.time_us, _state_struct.quat.toRotationMatrix().to_euler_rpy().y());
        Ulogger::instance()->write_data("yaw", _state_struct.time_us, _state_struct.quat.toRotationMatrix().to_euler_rpy().z());
        Ulogger::instance()->write_data("velocity_e", _state_struct.time_us, _state_struct.vel.x());
        Ulogger::instance()->write_data("velocity_n", _state_struct.time_us, _state_struct.vel.y());
        Ulogger::instance()->write_data("velocity_u", _state_struct.time_us, _state_struct.vel.z());
        Ulogger::instance()->write_data("position_e", _state_struct.time_us, _state_struct.pos.x());
        Ulogger::instance()->write_data("position_n", _state_struct.time_us, _state_struct.pos.y());
        Ulogger::instance()->write_data("position_u", _state_struct.time_us, _state_struct.pos.z());
        Ulogger::instance()->write_data("gyro_bias_x", _state_struct.time_us, _state_struct.gyro_bias.x());
        Ulogger::instance()->write_data("gyro_bias_y", _state_struct.time_us, _state_struct.gyro_bias.y());
        Ulogger::instance()->write_data("gyro_bias_z", _state_struct.time_us, _state_struct.gyro_bias.z());
        Ulogger::instance()->write_data("accel_bias_x", _state_struct.time_us, _state_struct.accel_bias.x());
        Ulogger::instance()->write_data("accel_bias_y", _state_struct.time_us, _state_struct.accel_bias.y());
        Ulogger::instance()->write_data("accel_bias_z", _state_struct.time_us, _state_struct.accel_bias.z());
        Ulogger::instance()->write_data("odom_scale", _state_struct.time_us, _state_struct.odom_scale);
```
#### 特别的： 

```
写入整型或bool型数据请调用以下接口：
Ulogger::instance()->write_data_if_value_changed("velocity_e", _state_struct.time_us, _state_struct.vel.x());
```
> 数据变化时写入，会节省空间，但一定程度上会增加调用开销



### 写入时间序列文本：(支持格式化写入，也可仅写入普通文本打印log，数据需要带时间戳us单位)

```
        Ulogger::instance()->log_printf(_state_struct.time_ns, "vehicle_speed=%lf", _odom_data_new.speed);
```
## 4.**使用LocalizationAnalysis**工具查看*.ulg数据
>使用分析工具打开```2019-10-23-13:46:18.ulg```
