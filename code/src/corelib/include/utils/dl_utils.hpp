#pragma once
#include <string>
#include "dlfcn.h"
#include "app_preference.hpp"
#include "app_util.hpp"
#include "app_config.hpp"
#include <diag/diagnose.hpp>

typedef void (*VoidFunc)();

class DlUtils{
    DlUtils();

    static std::map<std::string, void*>& get_plug_map() {
        static std::map<std::string, void*> plug_map;
        return plug_map;
    }
public:

    static void initial_path(char* argv[]) {
        char absolute_path[10240];
        realpath(argv[0], absolute_path);
        std::string exe_file(absolute_path);
        std::string exe_name = exe_file.substr(exe_file.find_last_of('/') + 1);
        std::string app_folder = DlUtils::path_go_back(exe_file, 2);
        std::string root_folder = DlUtils::path_go_back(app_folder);
        std::string bin_folder = DlUtils::path_go_back(exe_file);
        std::string lib_folder = app_folder + "/lib";
        std::string config_folder = app_folder + "/config";
        auto config_dir = opendir(config_folder.c_str());
        if (config_dir) {
            closedir(config_dir);
        } else {
            config_folder = root_folder + "/config";
            config_dir = opendir(config_folder.c_str());
            if (config_dir) {
                closedir(config_dir);
            } else {
                FATAL() << "no config dir found!";
            }
        }
        std::string ini_folder = config_folder + "/ini";
        appPref.set_string_data("exe_file", exe_file);
        appPref.set_string_data("exe_name", exe_name);
        appPref.set_string_data("app_folder", app_folder);
        appPref.set_string_data("root_folder", root_folder);
        appPref.set_string_data("bin_folder", bin_folder);
        appPref.set_string_data("lib_folder", lib_folder);
        appPref.set_string_data("config_folder", config_folder);
        appPref.set_string_data("ini_folder", ini_folder);
        AppConfig::load_ini_config(ini_folder +"/*.ini");

        initial_log();

        Diagnose::register_server("load_plugin", [&](const std::string& name) {
            std::string message;
            if (DlUtils::try_load_plugin(name.c_str(), message)) {
                DlUtils::run_plugin(name.c_str());
            }
            return message;
        });
    }
    static bool try_load_plugin(const char* name, std::string& message) {
        std::string lib_folder = appPref.get_string_data("lib_folder");
        std::string lib_file = lib_folder + "/lib";
        lib_file += name;
        lib_file += ".so";
        std::stringstream ss;
        if (get_plug_map().find(name) != get_plug_map().end()) {
            ss << "load_plugin: plugin " << name << " is already loaded!";
            message = ss.str();
            return false;
        }
        auto plugin = dlopen(lib_file.c_str(), RTLD_LAZY);
        if (!plugin) {
            ss << "load_plugin: load library faild, error " << dlerror() << " path:" << lib_file;
            message = ss.str();
            return false;
        }
        std::string load_func_name = std::string("load_") + name;

        VoidFunc load = (VoidFunc)dlsym(plugin, load_func_name.c_str());
        if (!load) {
            ss << "load_plugin: load func exec faild, error " << dlerror() << " path:" << load_func_name;
            message = ss.str();
            return false;
        }
        load();
        get_plug_map()[name] = plugin;
        ss << name << " load successed!";
        message = ss.str();
        return true;
    }

    static void load_plugin(const char* name) {
        std::string message;
        bool res = try_load_plugin(name, message);
        FATAL_IF_NOT(res) << message;
    }

    static void run_plugin(const char* name) {
        std::string run_func_name = std::string("run_") + name;
        if (get_plug_map().find(name) == get_plug_map().end()) {
            FATAL() << "run_plugin: plugin " << name << " is not loaded!";
        }
        auto plugin = get_plug_map()[name];
        VoidFunc run = (VoidFunc)dlsym(plugin, run_func_name.c_str());
        if (!run) {
            FATAL() << "run_plugin: run func exec faild, error " << dlerror();
        }
        run();
    }

    static void unload_plugin(const char* name) {
        std::string unload_func_name = std::string("unload_") + name;
        if (get_plug_map().find(name) == get_plug_map().end()) {
            FATAL() << "unload_plugin: plugin " << name << " is not loaded!";
        }
        auto plugin = get_plug_map()[name];
        VoidFunc unload = (VoidFunc)dlsym(plugin, unload_func_name.c_str());
        if (!unload) {
            FATAL() << "unload_plugin: unload func exec faild, error " << dlerror();
        }
        unload();
    }

    static std::string path_go_back(const std::string& path, int times = 1) {
        std::string path_res = path;
        for (int i = 0; i < times; i++) {
            path_res = path_res.substr(0, path_res.find_last_of('/'));
        }
        return path_res;
    }

private:
    static void initial_log()
    {
        if (appPref.has_string_key("log.log_level")) {
            int log_level = std::stoi(appPref.get_string_data("log.log_level"));
            bool level_check = log_level < LogLevel::INFO || log_level > LogLevel::FATAL;
            log_level = level_check ? LogLevel::DIRECT : log_level;
            LogInterface::set_log_level((LogLevel)log_level);
            DIRECT() << "using log level: " << LogInterface::log_level_to_string((LogLevel)log_level);
        } else {
            WARNING() << "using default log level: INFO";
        }
    }
};
