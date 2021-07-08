//
// Created by nvidia on 10/17/19.
//

#ifndef PROJECT_ULOGGER_H
#define PROJECT_ULOGGER_H

#include <stdint.h>
#include "stdlib.h"
#include "messages.h"
#include <stdio.h>
#include <vector>
#include <map>
#include <mutex>

#include <thread>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstring>
#include <tuple>

#include <pipe/pipe_element.hpp>

struct UlgConfig {
    std::string folder = ""; //父目录
    std::string header = ""; //day header

    bool is_log_on = false;
    bool create_day_folder = false;
    bool is_file_ulimited = false;
    int64_t FILE_SIZE_MAX = 4096 * 8 * 4e3;
    int32_t ULG_NUM = 10;
    int32_t BATCH_NUM = 10;
    int32_t DAY_NUM = 10;
};

class MmapAsyncWrite : public basic::PipeElement{
public:
    typedef std::tuple<int, uint8_t*, int32_t, int64_t> ParamTuple;

    MmapAsyncWrite();
    bool initialize(const UlgConfig& i_conf);

    ~MmapAsyncWrite();
    virtual void thread_func() override;

    std::string get_ulg_file_name(const std::string& dir_name);

    bool MapRegion(int fd, uint64_t file_offset, uint8_t **base);

    void UnMapRegion(uint8_t* base);

    void write_data(uint8_t* data, int32_t len);

    bool write_ulg_headr(uint8_t* data, int32_t len);
    
    void sync_batch();

    void sync_write();

    bool is_newday_folder();

    std::string create_day_folder();

    std::string get_newday_folder();

    void set_config(const UlgConfig& conf);

public:
    static const int32_t MAP_SIZE = 4096 * 8;
    static const int32_t WRITE_SIZE = MAP_SIZE / 2;
    
private:
    UlgConfig _ulg_conf;
    uint8_t * _base;
    uint8_t * _base_buffer; //缓冲区，待写入的数据
    uint8_t * _cursor;
    uint8_t _ulg_header[WRITE_SIZE]; //记录ulg header

    std::string _file_name;
    std::string _batch_name;
    std::string _dir_name; 
    std::string _up_header;
    std::string _batch_folder;
    ParamTuple _param_pwrite;

    int32_t _mmap_offset;
    int64_t _file_offset;
    int32_t _header_offset;

    int _data_fd;
    int _batch_fd;

    bool _is_init;
};

enum LogType {
    NORMAL = 0,
    LOC = 1,
    SYS = 2,
    SENSOR = 3
};

class Ulogger {
public:
    Ulogger(LogType log_type = NORMAL);

    ~Ulogger();

    static Ulogger *instance(LogType log_type = NORMAL) {
        static Ulogger s_instance_normal(NORMAL); //
        static Ulogger s_instance_loc(LOC); // 
        static Ulogger s_instance_sys(SYS); //
        static Ulogger s_instance_sensor(SENSOR); // 

        Ulogger * ulg_ptr = NULL;

        switch (log_type) {
        case NORMAL:
            ulg_ptr = &s_instance_normal;
            break;
        case LOC:
            ulg_ptr = &s_instance_loc;
            break;
        case SYS:
            ulg_ptr = &s_instance_sys;
            break;
        case SENSOR:
            ulg_ptr = &s_instance_sensor;
            break;
        default:
            ulg_ptr = &s_instance_normal;
            break;
        }
        return ulg_ptr;
    }

private:
    MmapAsyncWrite _mmap_writer;
    UlgConfig _ulg_conf;
    FILE *_ulog_file;

    bool _file_initialized = false;
    bool _registered_topic_written = false;
    uint16_t _topic_name_id = 0;
    std::map<const char *, uint16_t> _topic_name_id_pair;
    std::mutex _ulogger_mutex;

    // store last value of integers
    std::map<const char *, std::vector<bool>> _bool_name_value_pair;
    std::map<const char *, std::vector<char>> _char_name_value_pair;
    std::map<const char *, std::vector<int8_t>> _int8_name_value_pair;
    std::map<const char *, std::vector<uint8_t>> _uint8_name_value_pair;
    std::map<const char *, std::vector<int16_t>> _int16_name_value_pair;
    std::map<const char *, std::vector<uint16_t>> _uint16_name_value_pair;
    std::map<const char *, std::vector<int32_t>> _int32_name_value_pair;
    std::map<const char *, std::vector<uint32_t>> _uint32_name_value_pair;
    std::map<const char *, std::vector<int64_t>> _int64_name_value_pair;
    std::map<const char *, std::vector<uint64_t>> _uint64_name_value_pair;

    void initialize_uloger(const char *file_path);

    /**
     * write the file header with file magic and timestamp.
     */
    void write_header(void);

    void write_time_series_data_format(const char *topic_name);

    void write_version(void);

    void write_info(const char *name, const char *value);

    void write_info_multiple(const char *name, const char *value, bool is_continued);

    void write_info(const char *name, int32_t value);

    void write_info(const char *name, uint32_t value);

    template<typename T>
    void write_info_template(const char *name, T value, const char *type_str);

    void write_parameters(void);

    bool write_message(void *ptr, size_t size, bool record_as_headr = false);

public:
    bool register_time_series_data(const char *type_name, const char *topic_name);

    bool write_all_registered_time_series_data_format(void);

    bool write_data(const char *topic_name, uint64_t time_stamp_us, double variable_value);

    bool write_data(const char *topic_name, uint64_t time_stamp_us, uint8_t variable_value);

    bool write_data(const char *topic_name, uint64_t time_stamp_us, int8_t variable_value);

    bool write_data(const char *topic_name, uint64_t time_stamp_us, int16_t variable_value);

    bool write_data(const char *topic_name, uint64_t time_stamp_us, uint16_t variable_value);

    bool write_data(const char *topic_name, uint64_t time_stamp_us, int32_t variable_value);

    bool write_data(const char *topic_name, uint64_t time_stamp_us, uint32_t variable_value);

    bool write_data(const char *topic_name, uint64_t time_stamp_us, int64_t variable_value);

    bool write_data(const char *topic_name, uint64_t time_stamp_us, uint64_t variable_value);

    bool write_data(const char *topic_name, uint64_t time_stamp_us, std::vector<std::vector<double>> lanes);

    // ONLY FOR INT TYPE, EFFICIENT BUT EXPENSIVE
    bool write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, int8_t variable_value);

    bool write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, uint8_t variable_value);

    bool write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, int16_t variable_value);

    bool write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, uint16_t variable_value);

    bool write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, int32_t variable_value);

    bool write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, uint32_t variable_value);

    bool write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, int64_t variable_value);

    bool write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, uint64_t variable_value);

    bool write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, bool variable_value);

    void log_printf(uint64_t time_stamp_us, const char *format, ...);

    void sync_batch();

    void sync_write();

    void set_log_ulimited();

    void reset_log_config();

    template<class T>
    bool write_data_t(const char *topic_name, const T& i_data) {
        if (!_ulg_conf.is_log_on) {
            return false;
        }
        if (_topic_name_id_pair.empty()) {
            return false;
        }
        if (!_registered_topic_written) {
            _registered_topic_written = write_all_registered_time_series_data_format();
        }

        _ulogger_mutex.lock();
        auto variable_count = sizeof(T);
        auto msg_size = sizeof(ulog_message_data_header_s) + variable_count;
        auto it = _topic_name_id_pair.find(topic_name);
        if (it != _topic_name_id_pair.end()) {
            ulog_message_data_header_s msg = {};
            uint8_t buffer[msg_size];
            msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
            msg.msg_id = it->second;
            memcpy(buffer, (uint8_t*)&msg, sizeof(ulog_message_data_header_s));
            memcpy(buffer + sizeof(ulog_message_data_header_s), (uint8_t*)&i_data, variable_count);
            write_message(buffer, msg_size);
            _ulogger_mutex.unlock();
            return true;
        } else {
            _ulogger_mutex.unlock();
            return false;
        }
    }
};


#endif //PROJECT_ULOGGER_H
