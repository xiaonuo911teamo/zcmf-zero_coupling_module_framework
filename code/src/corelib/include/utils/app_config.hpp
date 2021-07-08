/*
此文件中定义与程序配置相关的内容。

使用方式:
1. load_ini_config 加载满足条件的配置文件。如：load_ini_config("/home/nvidia/.../config/ini/*.ini");
2. 使用get_int32等获取对应字段的值。

注意事项:
1. load_ini_config 加载的路径需要是绝对路径
*/

#pragma once
#include <utils/app_util.hpp>
#include <utils/app_preference.hpp>
#include <assert.h>
#include <glob.h>

class AppConfig {
public:
    static bool has_key(const std::string& key) {
        return appPref.has_string_key(key);
    }
    // 根据 组名.字段名 获取其对应的int32数据
    // 如:  [Log]
    //      log_level: 1
    // @key=Log.log_level
    // @rel=1
    static int32_t get_int32(const std::string& key) {
        std::string value = appPref.get_string_data(key);
        return AppUtil::safe_stoi(value);
    }
    // 根据 组名.字段名 获取其对应的int64数据
    // 如:  [Log]
    //      log_level: 1
    // @key=Log.log_level
    // @rel=1
    static int64_t get_int64(const std::string& key) {
        std::string value = appPref.get_string_data(key);
        return AppUtil::safe_stoll(value);
    }
    // 根据 组名.字段名 获取其对应的double 数组数据
    // 如:  [Log]
    //      log_file: file_2021.log
    // @key=Log.log_file
    // @rel=file_2021.log
    static double get_double(const std::string& key) {
        std::string value = appPref.get_string_data(key);
        return AppUtil::safe_stod(value);
    }
    // 根据 组名.字段名 获取其对应的double 数组数据
    // 如:  [Log]
    //      log_file: file_2021.log
    // @key=Log.log_file
    // @rel=file_2021.log
    static std::vector<double> get_double_vector(const std::string& key) {
        std::string value = appPref.get_string_data(key);
        return AppUtil::safe_stodv(value);
    }
    // 根据 组名.字段名 获取其对应的string数据
    // 如:  [Num]
    //      numbers: [1.0,2.0,3.0,4.0,5.0]
    // @key=Num.numbers
    // @rel=[1.0,2.0,3.0,4.0,5.0]
    static std::string get_str(const std::string& key) {
        return appPref.get_string_data(key);
    }
    // 读取满足pattern格式的路径，按ini格式加载其中文件, 将其中的字段值保存到全局变量appPref中
    // @pattern 可带有通配符的路径，如 /home/nvidia/.../config/ini/*.ini
    // 
    // ini文件的编写规则另见其他说明
    static void load_ini_config(const std::string& config_pattern) {
        std::vector<std::string> config_files;
        if (glob_files(config_pattern, config_files) == true) {
            for (auto& config_file : config_files) {
                parse_ini(config_file);
            }
        }
    }

private:
    // 获取所有满足pattern格式的路径
    // @pattern 可带有通配符的路径，如 /home/nvidia/.../config/ini/*.ini
    // @files 一一保存所有路径
    static bool glob_files(const std::string& pattern, std::vector<std::string>& files) {
        glob_t glob_result;
        memset(&glob_result, 0, sizeof(glob_result));
        int res = glob(pattern.c_str(), GLOB_TILDE, nullptr, &glob_result);
        if (res != 0) {
            globfree(&glob_result);
            return false;
        }
        for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
            files.emplace_back(std::string(glob_result.gl_pathv[i]));
        }
        globfree(&glob_result);
        return true;
    }
    // 解析ini文件，并将其中的字段值保存到全局变量appPref中
    // @file ini文件的路径
    // 
    // ini文件的编写规则另见其他说明
    static void parse_ini(const std::string& file) {
        std::string text;
        AppUtil::string_load(file, text);
        std::stringstream ss(text);
        std::string curr_section = "";
        std::string line;
        while (std::getline(ss, line, '\n')) {
            line = AppUtil::string_trim(line);
            if (line.empty()) {
                continue;
            }
            // comment
            if ((line[0] == '#') || (line[0] == ';')) {
                continue;
            }
            // new section
            if ((line[0] == '[') && (line[line.length() - 1] == ']')) {
                line = AppUtil::string_trim(line.substr(1, line.length() - 2));
                if (!line.empty()) {
                    curr_section = line + ".";
                }
                continue;
            }
            auto pos = line.find("=");
            // unknown
            if (pos == line.npos) {
                continue;
            }
            // name=value
            std::string name = AppUtil::string_trim(line.substr(0, pos));
            std::string value = AppUtil::string_trim(line.substr(pos + 1));
            pos = value.find("#");
            value = AppUtil::string_trim(value.substr(0, pos));
            if (name.empty() || value.empty()) {
                // invalid
                continue;
            }
            // section-name = value
            appPref.set_string_data(curr_section + name, value);
        }
    }
    // 从key中解析出组名和字段值
    // @key 字符串如 Log.log_level
    // @section 组名如 Log
    // @name 字段值如 log_level
    static void get_section_and_name_from_key(const std::string& key, std::string& section,
            std::string& name) {
        std::regex reg("(\\w+).(\\w+)");
        std::smatch sm;

        if (key.empty() || !std::regex_match(key, sm, reg)) {
            FATAL() << "get_str format error, example is \"section.name\"; key: " << key;
        }

        section = sm[1];
        name = sm[2];
    }
};
