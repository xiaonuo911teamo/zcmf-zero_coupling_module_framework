#pragma once
#include <mutex>
#include <map>
#include <log/logging.h>
#define appPref AppPreference::get_instance()

class AppPreference {
public:
    static AppPreference& get_instance() {
        static AppPreference app_preference;
        return app_preference;
    }

protected:
    AppPreference() {
    };
private:
    std::map<std::string, std::string> strings;
    // std::map<std::string, int> ints;
    // std::map<std::string, long> longs;
    // std::map<std::string, float> floats;
    // std::map<std::string, double> doubles;
    // std::map<std::string, std::vector<double> > vec_doubles;
public:
    std::string get_string_data(const std::string& key) {
        if (strings.find(key) == strings.end()) {
            ERROR() << "String data has no key named " << key;
            throw std::runtime_error("key_not_found");
        }

        return strings[key];
    }
    // int get_int_data(const std::string& key) {
    //     if (ints.find(key) == ints.end()) {
    //         ERROR() << "Int data has no key named " << key;
    //         throw std::runtime_error("key_not_found");
    //     }

    //     return ints[key];
    // }
    // long get_long_data(const std::string& key) {
    //     if (longs.find(key) == longs.end()) {
    //         ERROR() << "Long data has no key named " << key;
    //         throw std::runtime_error("key_not_found");
    //     }

    //     return longs[key];
    // }

    // float get_float_data(const std::string& key) {
    //     if (floats.find(key) == floats.end()) {
    //         ERROR() << "Float data has no key named " << key;
    //         throw std::runtime_error("key_not_found");
    //     }

    //     return floats[key];
    // }

    // double get_double_data(const std::string& key) {
    //     if (doubles.find(key) == doubles.end()) {
    //         ERROR() << "Double data has no key named " << key;
    //         throw std::runtime_error("key_not_found");
    //     }

    //     return doubles[key];
    // }

    // std::vector<double> get_double_vector_data(const std::string& key) {
    //     if (vec_doubles.find(key) == vec_doubles.end()) {
    //         ERROR() << "Double vector data has no key named " << key;
    //         throw std::runtime_error("key_not_found");
    //     }
    //     return vec_doubles[key];
    // }

    void set_string_data(const std::string& key, const std::string& value) {
        strings[key] = value;
    }
    // void set_int_data(const std::string& key, int value) {
    //     ints[key] = value;
    // }
    // void set_long_data(const std::string& key, long value) {
    //     longs[key] = value;
    // }
    // void set_float_data(const std::string& key, float value) {
    //     floats[key] = value;
    // }
    // void set_double_data(const std::string& key, double value) {
    //     doubles[key] = value;
    // }
    // void set_double_vector_data(const std::string& key, std::vector<double> value) {
    //     vec_doubles[key] = value;
    // }

    bool has_string_key(std::string key) {
        return strings.find(key) != strings.end();
    }
    // bool has_int_key(std::string key) {
    //     return ints.find(key) != ints.end();
    // }
    // bool has_long_key(std::string key) {
    //     return longs.find(key) != longs.end();
    // }
    // bool has_float_key(std::string key) {
    //     return floats.find(key) != floats.end();
    // }
    // bool has_double_key(std::string key) {
    //     return doubles.find(key) != doubles.end();
    // }
    // bool has_double_vector_key(std::string key) {
    //     return vec_doubles.find(key) != vec_doubles.end();
    // }
};

