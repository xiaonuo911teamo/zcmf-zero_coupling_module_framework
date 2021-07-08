// 待优化，暂不放入代码库

#pragma once
#include <log/logging.h>
#include <fstream>

class DataRecoder {
public:
    DataRecoder(const std::string& path, char spliter = '\t'): spliter(spliter) {

        file.open(path);
        FATAL_IF_NOT(file.is_open()) << "open file failed! path: " << path;
    }

    ~DataRecoder() {
        save();
    }

    template<typename T>
    DataRecoder& record(T t) {
        file << t << std::endl;
        return *this;
    }

    template<typename T, typename... Args>
    DataRecoder& record(T t, Args ... args) {
        file << t << spliter;
        record(args...);
        return *this;
    }

    void save() {
        file.flush();
    }

private:
    std::ofstream file;
    char spliter;
};
