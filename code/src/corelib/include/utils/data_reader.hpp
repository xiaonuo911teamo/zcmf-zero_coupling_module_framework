/*
文件读取器(待优化，暂不放入代码库)

使用方法：
file文件内容： xiaonuo beijing 19960515 25   # 分别表示 姓名 住址 生日 年龄

std::string name;
std::string address;
std::string birthday;
int age;
DataReader reader;
reader.read(name, address, birthday, age);
if (reader.is_finished())
    std::cout << "name: " << name << "address: " << address << "birthday: " << birthday << "age: " << age;

*/
#include <log/logging.h>
#include <fstream>

class DataReader {
public:
    DataReader(const std::string& path, char spliter = '\t') {

        file.open(path);
        FATAL_IF_NOT(file.is_open()) << "open file failed! path: " << path;
    }

    ~DataReader() {
    }

    template<typename T>
    DataReader& read(T& t) {
        file >> t;
        return *this;
    }

    template<typename T, typename... Args>
    DataReader& read(T& t, Args& ... args) {
        file >> t;
        read(args...);
        return *this;
    }

    bool is_finished() {
        return file.peek() == EOF;
    };
private:
    std::ifstream file;
};
