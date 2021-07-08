#pragma once

#include <map>
#include <functional>
#include <string>
#include <vector>

class Messager {
public:
    Messager() = delete;
    static void subcribe(const std::string &key, std::function<void()> func) {
        auto &messager_map = get_messager_map();
        auto &funcs = messager_map[key];
        funcs.push_back(func);
    }

    template<typename T>
    static void subcribe(const std::string &key, std::function<void(const T &)> func) {
        auto &messager_map = get_messager_map<T>();
        auto &funcs = messager_map[key];
        funcs.push_back(func);
    }

    template<typename T0, typename T1>
    static void subcribe(const std::string &key, std::function<void(const T0 &, const T1 &)> func) {
        auto &messager_map = get_messager_map<T0, T1>();
        auto &funcs = messager_map[key];
        funcs.push_back(func);
    }

    template<typename T0, typename T1, typename T2>
    static void subcribe(const std::string &key, std::function<void(const T0 &, const T1 &, const T2 &)> func) {
        auto &messager_map = get_messager_map<T0, T1, T2>();
        auto &funcs = messager_map[key];
        funcs.push_back(func);
    }

    template<typename T0, typename T1, typename T2, typename T3>
    static void
    subcribe(const std::string &key, std::function<void(const T0 &, const T1 &, const T2 &, const T3 &)> func) {
        auto &messager_map = get_messager_map<T0, T1, T2, T3>();
        auto &funcs = messager_map[key];
        funcs.push_back(func);
    }

    template<typename T0, typename T1, typename T2, typename T3, typename T4>
    static void subcribe(const std::string &key,
                         std::function<void(const T0 &, const T1 &, const T2 &, const T3 &, const T4 &)> func) {
        auto &messager_map = get_messager_map<T0, T1, T2, T3, T4>();
        auto &funcs = messager_map[key];
        funcs.push_back(func);
    }

    static void publish(const std::string &key) {
        auto &messager_map = get_messager_map();
        auto &funcs = messager_map[key];

        for (const auto &func : funcs) {
            func();
        }
    }

    template<typename T>
    static void publish(const std::string &key, const T &value) {
        auto &messager_map = get_messager_map<T>();
        auto &funcs = messager_map[key];

        for (const auto &func : funcs) {
            func(value);
        }
    }

    template<typename T0, typename T1>
    static void publish(const std::string &key, const T0 &value0, const T1 &value1) {
        auto &messager_map = get_messager_map<T0, T1>();
        auto &funcs = messager_map[key];

        for (const auto &func : funcs) {
            func(value0, value1);
        }
    }

    template<typename T0, typename T1, typename T2>
    static void publish(const std::string &key, const T0 &value0, const T1 &value1, const T2 &value2) {
        auto &messager_map = get_messager_map<T0, T1, T2>();
        auto &funcs = messager_map[key];

        for (const auto &func : funcs) {
            func(value0, value1, value2);
        }
    }

    template<typename T0, typename T1, typename T2, typename T3>
    static void
    publish(const std::string &key, const T0 &value0, const T1 &value1, const T2 &value2, const T3 &value3) {
        auto &messager_map = get_messager_map<T0, T1, T2, T3>();
        auto &funcs = messager_map[key];

        for (const auto &func : funcs) {
            func(value0, value1, value2, value3);
        }
    }

    template<typename T0, typename T1, typename T2, typename T3, typename T4>
    static void publish(const std::string &key, const T0 &value0, const T1 &value1, const T2 &value2, const T3 &value3,
                        const T4 &value4) {
        auto &messager_map = get_messager_map<T0, T1, T2, T3, T4>();
        auto &funcs = messager_map[key];

        for (const auto &func : funcs) {
            func(value0, value1, value2, value3, value4);
        }
    }

    template<typename T>
    static void add_server_func(const std::string &key, std::function<T> func) {
        auto &server_func = get_server_func<T>(key);
        if (server_func) {
             publish("log_fatal", "server_func is already exists, key: " + key);
             throw std::bad_exception();
        }
        server_func = func;
    }

    template<typename T>
    static bool has_server(const std::string &key) {
        auto &server_func = get_server_func<T>(key);
        if (server_func) {
            return true;
        } else {
            return false;
        }
    }

    template<typename T>
    static void remove_server_func(const std::string &key) {
        auto &server_func = get_server_func<T>(key);
        server_func = std::function<T>();
    }

    template<typename T>
    static std::function<T> &get_server_func(const std::string &key) {
        auto & server_func_map = get_server_map<T>();
        return server_func_map[key];
    }

public:
    template<typename T>
    static void register_server_map() {
        get_server_map<T>();
    }

    static void register_data_map() {
        get_messager_map();
    }

    template<typename T>
    static void register_data_map() {
        get_messager_map<T>();
    }

    template<typename T0, typename T1>
    static void register_data_map() {
        get_messager_map<T0, T1>();
    }

    template<typename T0, typename T1, typename T2>
    static void register_data_map() {
        get_messager_map<T0, T1, T2>();
    }

    template<typename T0, typename T1, typename T2, typename T3>
    static void register_data_map() {
        get_messager_map<T0, T1, T2, T3>();
    }

    template<typename T0, typename T1, typename T2, typename T3, typename T4>
    static void register_data_map() {
        get_messager_map<T0, T1, T2, T3, T4>();
    }

    template<typename T>
    static std::vector<std::string> get_server_list() {
        std::vector<std::string> keys;
        auto& server_map = get_server_map<T>();
        for (auto& server : server_map) {
            if (server.second) {
                keys.push_back(server.first);
            }
        }
        return keys;
    }

private:
    template<typename T>
    static std::map<std::string, std::function<T>> &get_server_map() {
        static std::map<std::string, std::function<T>> server_func_map;
        return server_func_map;
    }

    static std::map<std::string, std::vector<std::function<void()>>> &get_messager_map() {
        static std::map<std::string, std::vector<std::function<void()>>> messager_map;
        return messager_map;
    }

    template<typename T>
    static std::map<std::string, std::vector<std::function<void(const T &)>>> &get_messager_map() {
        static std::map<std::string, std::vector<std::function<void(const T &)>>> messager_map;
        return messager_map;
    }

    template<typename T0, typename T1>
    static std::map<std::string, std::vector<std::function<void(const T0 &, const T1 &)>>> &get_messager_map() {
        static std::map<std::string, std::vector<std::function<void(const T0 &, const T1 &)>>> messager_map;
        return messager_map;
    }

    template<typename T0, typename T1, typename T2>
    static std::map<std::string, std::vector<std::function<void(const T0 &, const T1 &, const T2 &)>>> &
    get_messager_map() {
        static std::map<std::string, std::vector<std::function<void(const T0 &, const T1 &, const T2 &)>>> messager_map;
        return messager_map;
    }

    template<typename T0, typename T1, typename T2, typename T3>
    static std::map<std::string, std::vector<std::function<void(const T0 &, const T1 &, const T2 &, const T3 &)>>> &
    get_messager_map() {
        static std::map<std::string, std::vector<std::function<void(const T0 &, const T1 &, const T2 &,
                                                                    const T3 &)>>> messager_map;
        return messager_map;
    }

    template<typename T0, typename T1, typename T2, typename T3, typename T4>
    static std::map<std::string, std::vector<std::function<void(const T0 &, const T1 &, const T2 &, const T3 &,
                                                                const T4 &)>>> &
    get_messager_map() {
        static std::map<std::string, std::vector<std::function<void(const T0 &, const T1 &, const T2 &, const T3 &,
                                                                    const T4 &)>>> messager_map;
        return messager_map;
    }
};
