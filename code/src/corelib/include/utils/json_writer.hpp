#pragma once
#include <fstream>
#include <jsoncpp/json/json.h>

class JsonWriter {
public:
    JsonWriter() {
        //jswBuilder["emitUTF8"] = true; // 直接输出 UTF-8 字符
        _jsb["indentation"] = ""; // 压缩格式，没有换行和不必要的空白字符
    }

    ~JsonWriter() {
        if (_file_os.is_open()) {
            _file_os.close();
        }
    }

    void open(const std::string& filename, std::ios_base::openmode mode) {
        _file_os.open(filename, mode);
    }

    void write(const Json::Value& value) {
        std::unique_ptr<Json::StreamWriter> jsw(_jsb.newStreamWriter());
        std::ostringstream os;
        jsw->write(value, &os); 
        _file_os << os.str();
        _file_os.flush();

    }

private:
    Json::StreamWriterBuilder _jsb;
    std::ofstream _file_os;
};
