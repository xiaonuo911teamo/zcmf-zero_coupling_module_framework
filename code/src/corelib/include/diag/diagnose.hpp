#pragma once

/******************************************************************************
  模拟中断触发方式

  学电路的同学会比较熟悉，中断触发方式分为电平触发和边沿触发。百度一下可以知道，电平触发
  是判断到电信号为0或1时，触发中断，处理中断函数。而边沿触发是接收到从0到1或从1到0的跳变，
  触发中断，处理中断函数。

******************************************************************************/

#include <message/messager.hpp>
#include <regex>
#include <log/logging.h>

// 触发方式
// 这里将电平触发和边沿触发, 拆分为 高电平触发、低电平触发、上升沿触发、下降沿触发、跳变触发
enum TriggerType{
    TRUE,       // 高电平触发, 当信号为1时触发
    FALSE,      // 低电平触发, 当信号为0时触发
    UP,         // 上升沿触发, 当信号从0变为1时触发
    DOWN,       // 下降沿触发, 当信号从1变为0时触发
    UPDOWN      // 跳变触发, 上升沿和下降沿均可触发
};

class Diagnose{
public:
    // 功能等同于Message中的add_server_func
    // 此处register_server的含义是为了注册一个用于诊断的服务，所以内部自动在服务名称后面追加了 "_diag"
    static void register_server(const std::string& name, std::function<std::string(const std::string&)> func) {
        if (Messager::has_server<std::string(const std::string&)>(name + "__diag")) {
            WARNING() << " diag server named " << name << " is already exist!";
        } else {
            Messager::add_server_func(name + "__diag", func);
        }
    }

    // 注册以name为名的action，以trigger_type方式触发中断函数func，为了方式触发过于频繁，可以设置触发间隔trigger_times
    template<class T>
    static void register_diag_action(const std::string& name,
                              std::function<void(const T&)> func,
                              TriggerType trigger_type = TRUE, int trigger_times = 1) {
        auto& trigger_times_map = get_trigger_times_map<T>();
        auto& condition_map = get_condition_map<T>();
        auto& last_condition_map = get_last_condition_map<T>();
        static int trigger_times_id = 0;
        trigger_times_id++;
        int trigger_times_id_captured = trigger_times_id;

        auto trigger_func = [=, &trigger_times_map](const T& data, bool is_change) {
            auto& func_trigger_times = trigger_times_map[trigger_times_id_captured];
            if (is_change) {
                func_trigger_times = 0;
            }
            if (++func_trigger_times >= trigger_times) {
                func(data);
                func_trigger_times = 0;
            }
        };

        Messager::subcribe<bool, T>(name + "__diag", [=, &condition_map, &last_condition_map](bool condition, const T& data) {

            auto& last_condition = condition_map[name];
            bool is_changed_true_false = last_condition ^ condition;
            bool has_last_last_condition = last_condition_map.find(name) != last_condition_map.end();
            bool is_changed_up_down = false;
            if (has_last_last_condition) {
                is_changed_up_down = !last_condition_map[name] ^ last_condition;
            }

            switch (trigger_type) {
                case TRUE:
                    if (condition) {
                        trigger_func(data, is_changed_true_false);
                    }
                    break;
                case FALSE:
                    if (!condition) {
                        trigger_func(data, is_changed_true_false);
                    }
                    break;
                case UP:
                    if (!last_condition && condition) {
                       trigger_func(data, is_changed_up_down);
                    }
                    break;
                case DOWN:
                    if (last_condition && !condition) {
                        trigger_func(data, is_changed_up_down);
                    }
                    break;
                case UPDOWN:
                    if (last_condition ^ condition) {
                        trigger_func(data, is_changed_up_down);
                    }
                    break;
                default:;
            }
        });
    }

    static void register_diag_action(const std::string& name,
                              std::function<void()> func,
                                     TriggerType trigger_type = TRUE, int trigger_times = 1) {
        register_diag_action<bool>(name, [func](const bool&) {
            func();
        }, trigger_type, trigger_times);
    }

    // 激活以name为名的action，信号量为condition，就是0或1，data对应处理函数中需要的参数
    template<class T>
    static void fire_diag_condition(const std::string& name, bool condition, const T& data) {
        auto& condition_map = get_condition_map<T>();
        auto& last_condition_map = get_last_condition_map<T>();
        if (condition_map.find(name) != condition_map.end()) {
            Messager::publish(name + "__diag", condition, data);
        }
        last_condition_map[name] = condition_map[name];
        condition_map[name] = condition;
    }

    static void fire_diag_condition(const std::string& name, bool condition) {
        fire_diag_condition(name, condition, true);
    }

    // 调用以name为名的服务，以value为参数，使用data为返回值
    // 服务name使用register_server进行注册
    // 此处只实现了string类型的特化版本，用于输出诊断监控的数据信息，详情可看test
    static bool diag_call(const std::string& name, const std::string& value, std::string& data) {
        return diag_call("name:" + name + ";value:" + value, data);
    }
    // 从字串 'name:***;value:***'，解析出name和value，调用diag(name, value, data)
    static bool diag_call(const std::string& input, std::string& data) {
        static std::regex regex("\\s*name:\\s*(\\w*)\\s*;\\s*value:\\s*(\\w*)\\s*");
        std::smatch result;
        bool retval = false;
        if (!input.empty() && std::regex_match(input, result, regex)) {
            std::string name = result[1];
            std::string value = result[2];

            auto func = Messager::get_server_func<std::string(const std::string&)>(name + "__diag");
            if (func) {
                data = func(value);
                retval = true;
            } else {
                data = "diag name not found: ";
                data += name;
            }
        } else {
            data = input + "-->input format error! example: name: name1; value: [123]";
        }
        return retval;
    }

public:
    // 手动注册diag_call中调用服务的储存结构，不手动调用时，会自动注册
    static void register_diag_server_map() {
        Messager::register_server_map<std::string(const std::string&)>();
    }

    // 手动注册模板为T，中断触发系统，不手动调用时，会自动注册
    template<class T>
    static void register_condition_data_map() {
        Messager::register_data_map<bool, T>();
        get_condition_map<T>();
        get_last_condition_map<T>();
        get_trigger_times_map<T>();
    }

private:
    template<class T>
    static std::map<std::string, bool> &get_condition_map() {
        static std::map<std::string, bool> condition_map;
        return condition_map;
    }

    template<class T>
    static std::map<std::string, bool> &get_last_condition_map() {
        static std::map<std::string, bool> condition_map;
        return condition_map;
    }

    template<class T>
    static std::map<int, int> &get_trigger_times_map() {
        static std::map<int, int> trigger_times_map;
        return trigger_times_map;
    }
};
