//
// Created by nvidia on 10/17/19.
//

#include "ulg/ulg.h"
#include "ulg/messages.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <stdarg.h>
#include <sys/utsname.h>

#include <utils/app_config.hpp>

MmapAsyncWrite::MmapAsyncWrite() : PipeElement(true, "MmapAsyncWrite") {
    _is_init = false;
}

// 生成ulog文件名，自动包含当前系统时间
std::string MmapAsyncWrite::get_ulg_file_name(const std::string& dir_name) {
    auto tt = std::chrono::system_clock::to_time_t
                (std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d-%02d-%02d-%02d",
            (int) ptm->tm_year + 1900, (int) ptm->tm_mon + 1, (int) ptm->tm_mday,
            (int) ptm->tm_hour, (int) ptm->tm_min, (int) ptm->tm_sec);

    std::string file_name = dir_name + "/" + _ulg_conf.header + std::string(date);
    return file_name;
}

// 使用i_conf对系统进行初始化
bool MmapAsyncWrite::initialize(const UlgConfig& i_conf) {
    _ulg_conf = i_conf;
    
    _dir_name = AppConfig::get_str("ulogger.ulog_write_folder") + "/" + _ulg_conf.folder;
    if (AppUtil::make_dir(_dir_name) == -1) {
        ERROR() << "open ulg folder error " ;
        return false;
    }

    _batch_folder = _dir_name + "/batch"; // 默认建立batch文件，作为临时共享交换区
    
    if (AppUtil::make_dir(_batch_folder) == -1) {
        ERROR() << "open ulg folder error " ;
        return false;
    }

    if (_ulg_conf.create_day_folder) {
        std::string up_dir = _dir_name;
        _dir_name += "/" + _ulg_conf.header + AppUtil::now_date();
        if (AppUtil::make_dir(_dir_name) == -1) {
            ERROR() << "open ulg header folder error " ;
            return false;
        }
        AppUtil::check_file_num(up_dir, _ulg_conf.header + ".*", _ulg_conf.DAY_NUM);
    }

    _file_name = get_ulg_file_name(_dir_name);
    _batch_name = get_ulg_file_name(_batch_folder);
    std::string fname = _file_name + ".ulg";
    std::string batch = _batch_name + ".batch";
    
    _base = nullptr;
    _base_buffer = nullptr;
    _cursor = nullptr ;
    _mmap_offset = 0;
    _file_offset = 0;
    _header_offset = 0;

    memset(_ulg_header, 0, WRITE_SIZE);
    _data_fd = AppUtil::make_file(fname);
    _batch_fd = AppUtil::make_file(batch);

    if (_data_fd < 0 || _batch_fd < 0) {
        _is_init = false;
        ERROR() << "open ulg file error " ;
        return _is_init;
    }

    // 文件数量控制，超出上限的文件将被删除
    if (!_ulg_conf.is_file_ulimited) {
        AppUtil::check_file_num(_dir_name, ".*\\.ulg", _ulg_conf.ULG_NUM);
        AppUtil::check_file_num(_batch_folder, ".*\\.batch", _ulg_conf.BATCH_NUM);
    }
    
    // 为batch文件预分配MAP_SIZE大小的空间
    if (posix_fallocate(_batch_fd, 0, MAP_SIZE) != 0) {
        _is_init = false;
        ERROR() << "allocate batch file error " ;
        return _is_init;
    }
    // 将batch文件的处理权映射给_base，可以减少read write操作
    if (!MapRegion (_batch_fd, 0 , &_base)) {
        _is_init = false;
        ERROR() << "mmap batch file error " ;
        return _is_init;
    }

    _base_buffer = _base + WRITE_SIZE;
    _cursor = _base;
    _is_init = true;
    start();
    return _is_init;
}

MmapAsyncWrite::~MmapAsyncWrite() {
    stop();
    // 将_base(batch)中的内容写入到_data_fd中
    pwrite(_data_fd, _base, _mmap_offset, _file_offset);
    memset(_base, 0, MAP_SIZE);     // _base(batch) 内存段置0
    UnMapRegion(_base);             // 解除_base和batch的关联
    close(_data_fd);                // 关闭_data_fd
    close(_batch_fd);               // 关闭_batch_fd
    wait();                         // 等待处理模块退出
}

void MmapAsyncWrite::set_config(const UlgConfig& conf) {
    _ulg_conf = conf;
}

void MmapAsyncWrite::thread_func() {
    pwrite(std::get<0>(_param_pwrite), std::get<1>(_param_pwrite), 
            std::get<2>(_param_pwrite), std::get<3>(_param_pwrite));

    bool is_newday = is_newday_folder();

    if ((std::get<2>(_param_pwrite) + std::get<3>(_param_pwrite) > _ulg_conf.FILE_SIZE_MAX
            && !_ulg_conf.is_file_ulimited) || is_newday) {
        if (is_newday) {
            _dir_name = create_day_folder();
        }

        _file_name = get_ulg_file_name(_dir_name);
        std::string new_file = _file_name +".ulg";
        int new_fd = AppUtil::make_file(new_file);
        if (new_fd >= 0 && _header_offset <= WRITE_SIZE) {
            pwrite(new_fd, _ulg_header, _header_offset, 0);

            close(_data_fd);
            _data_fd = new_fd;
            _file_offset = _header_offset;
        }
        AppUtil::check_file_num(_dir_name, ".*\\.ulg", _ulg_conf.ULG_NUM);
    }
}

std::string MmapAsyncWrite::get_newday_folder() {
    std::string father_folder = AppConfig::get_str("ulogger.ulog_write_folder") + "/" + _ulg_conf.folder;
    std::string new_folder = father_folder + "/" + _ulg_conf.header + AppUtil::now_date();
    return new_folder;
}

std::string MmapAsyncWrite::create_day_folder() {
    std::string new_dir;
    if (_ulg_conf.create_day_folder) {
        std::string up_dir = AppConfig::get_str("ulogger.ulog_write_folder") + "/" + _ulg_conf.folder;
        new_dir = get_newday_folder();
        if (new_dir.compare(_dir_name) != 0) {
            AppUtil::make_dir(new_dir);
            AppUtil::check_file_num(up_dir, _ulg_conf.header + ".*", _ulg_conf.DAY_NUM);
        } else {
            new_dir = "";
        }
    }
    return new_dir;
}

bool MmapAsyncWrite::is_newday_folder() {
    bool ret = false;
    if (_ulg_conf.create_day_folder) {
        std::string newdir = get_newday_folder();
        ret = (_dir_name.compare(newdir) != 0);
    }
    return ret;
}

bool MmapAsyncWrite::MapRegion(int fd, uint64_t file_offset, uint8_t **base) {
    void* ptr = mmap(nullptr, MAP_SIZE, PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, file_offset); 
    if (ptr == MAP_FAILED) {
        *base = nullptr;
        return false;
    }
    *base = reinterpret_cast<uint8_t*>(ptr);
    return true;
}

// 移除base和文件的关联,并刷新输出缓冲区
void MmapAsyncWrite::UnMapRegion(uint8_t* base) {
    munmap(base, MAP_SIZE);
}

void MmapAsyncWrite::write_data(uint8_t* data, int32_t len) {
    if (!_is_init) {
        return;
    }

    if (_mmap_offset + len > WRITE_SIZE) {
        sync_batch();
    }

    // 将data拷贝到batch当前_cursor位置
    memcpy(_cursor, data, len);
    _mmap_offset += len;
    _cursor += len;             // 更新cursor指针
}

// 将_base(batch)中的内容填入_data_fd, 类似于std::fflush
void MmapAsyncWrite::sync_batch() {
    memset(_base_buffer, 0, WRITE_SIZE);
    memcpy(_base_buffer, _base, _mmap_offset);
    _param_pwrite = ParamTuple(_data_fd, _base_buffer, _mmap_offset, _file_offset);
    _file_offset += _mmap_offset;
    submit(); //触发异步写, 调用thread_func

    memset(_base, 0, WRITE_SIZE);
    _cursor = _base;    //  _cursor 表示batch实时的写入位置, _base一直是文件头,不移动
    _mmap_offset = 0;
}

//只在需要同步写时使用，例如：需要拷贝当前文件．
void MmapAsyncWrite::sync_write() {
    memset(_base_buffer, 0, WRITE_SIZE);
    memcpy(_base_buffer, _base, _mmap_offset);
    _param_pwrite = ParamTuple(_data_fd, _base_buffer, _mmap_offset, _file_offset);

    pwrite(std::get<0>(_param_pwrite), std::get<1>(_param_pwrite), 
            std::get<2>(_param_pwrite), std::get<3>(_param_pwrite));

    _file_offset += _mmap_offset;
    memset(_base, 0, WRITE_SIZE);
    _cursor = _base;
    _mmap_offset = 0;
}

// 将data写入到_ulg_header
bool MmapAsyncWrite::write_ulg_headr(uint8_t* data, int32_t len) {
    if (_header_offset + len > WRITE_SIZE) {
        ERROR() << "_header_offset overflow : " << _header_offset + len;
        return false;
    }

    memcpy(_ulg_header + _header_offset, data, len);
    _header_offset += len;
    return true;
}

Ulogger::Ulogger(LogType log_type) {
    _file_initialized = false;

    switch (log_type) {
        case NORMAL: //默认调试ulg配置
            _ulg_conf.is_log_on = AppConfig::get_int32("ulogger.is_ulogger_on") == 1;
            _ulg_conf.is_file_ulimited = AppConfig::get_int32("ulogger.is_file_ulimited")  == 1;
            break;
        //以下配置为量产日志业务需求
        case LOC:
            _ulg_conf.is_log_on = AppConfig::get_int32("ulogger.is_onboard_log")  == 1;
            _ulg_conf.folder = AppConfig::get_str("ulogger.loc_folder");
            _ulg_conf.create_day_folder = true;
            _ulg_conf.header = "LOC_";
            _ulg_conf.DAY_NUM = AppConfig::get_int32("ulg_loc.day_num");
            _ulg_conf.ULG_NUM = AppConfig::get_int32("ulg_loc.ulg_num");
            _ulg_conf.FILE_SIZE_MAX = 4096 * 8 * 32; //限制1MB
            break;
        case SYS:
            _ulg_conf.is_log_on = AppConfig::get_int32("ulogger.is_onboard_log")  == 1;
            _ulg_conf.folder = AppConfig::get_str("ulogger.sys_folder");
            _ulg_conf.create_day_folder = true;
            _ulg_conf.header = "SYS_";
            _ulg_conf.DAY_NUM = AppConfig::get_int32("ulg_sys.day_num");
            _ulg_conf.ULG_NUM = AppConfig::get_int32("ulg_sys.ulg_num");
            _ulg_conf.FILE_SIZE_MAX = 4096 * 8 * 32;
            break;
        case SENSOR:
            _ulg_conf.is_log_on = AppConfig::get_int32("ulogger.is_onboard_log")  == 1;
            _ulg_conf.folder = AppConfig::get_str("ulogger.sensor_folder");
            _ulg_conf.header = "SENSOR_";
            _ulg_conf.ULG_NUM = AppConfig::get_int32("ulg_sensor.ulg_num");
            _ulg_conf.FILE_SIZE_MAX = 4096 * 8 * 32; //限制1MB
            break;
        default:
            _ulg_conf.is_log_on = AppConfig::get_int32("ulogger.is_ulogger_on") == 1;
            _ulg_conf.is_file_ulimited = AppConfig::get_int32("ulogger.is_file_ulimited") == 1;
            break;
    }
}

Ulogger::~Ulogger() {
}

void Ulogger::initialize_uloger(const char *file_path) {
    if (_file_initialized) {
        return;
    }
    std::string str_file_path(file_path);
    if (!_mmap_writer.initialize(_ulg_conf)) {
        ERROR() << "Error initialize ulg file!";
        assert(false);
    }
    _file_initialized = true;
}

void Ulogger::sync_batch() {
    _ulogger_mutex.lock();
    _mmap_writer.sync_batch();
    _ulogger_mutex.unlock();
}

void Ulogger::sync_write() {
    _ulogger_mutex.lock();
    _mmap_writer.sync_write();
    _ulogger_mutex.unlock();
}

void Ulogger::set_log_ulimited() {
    _ulogger_mutex.lock();
    UlgConfig tmp_conf = _ulg_conf;
    tmp_conf.is_file_ulimited = true;
    _mmap_writer.set_config(tmp_conf);
    _ulogger_mutex.unlock();
}

void Ulogger::reset_log_config() {
    _ulogger_mutex.lock();
    _mmap_writer.set_config(_ulg_conf);
    _ulogger_mutex.unlock();
}

/*
 * ALL REGISTER TYPE SHOULD WITHIN LISTED TYPE
 */
bool Ulogger::register_time_series_data(const char *type_name, const char *topic_name) {
    if (!_ulg_conf.is_log_on) {
        return false;
    }
    if (!_file_initialized) {
        std::stringstream ulog_file_name;
        ulog_file_name << AppConfig::get_str("ulogger.ulog_write_folder");
        initialize_uloger(ulog_file_name.str().c_str());
        write_header();
        write_version();
    }
    _ulogger_mutex.lock();
    ulog_message_format_s msg = {};
    auto *buffer = reinterpret_cast<uint8_t *>(&msg);
    int format_len = 0;
    const char *variable_name = " ";

    if (strcmp(type_name, "int8_t") == 0) {
        std::vector<int8_t> int8_v;
        _int8_name_value_pair[topic_name] = int8_v; // magic number
        format_len = snprintf(msg.format, sizeof(msg.format), "%s:uint64_t timestamp;%s %s;", topic_name, type_name,
                              variable_name);
    } else if (strcmp(type_name, "uint8_t") == 0) {
        std::vector<uint8_t> uint8_v;
        _uint8_name_value_pair[topic_name] = uint8_v; // magic number
        std::cout << "magic uint8_t size=" << _uint8_name_value_pair.size() << std::endl;
        format_len = snprintf(msg.format, sizeof(msg.format), "%s:uint64_t timestamp;%s %s;", topic_name, type_name,
                              variable_name);
    } else if (strcmp(type_name, "int16_t") == 0) {
        std::vector<int16_t> int16_v;
        _int16_name_value_pair[topic_name] = int16_v; // magic number
        format_len = snprintf(msg.format, sizeof(msg.format), "%s:uint64_t timestamp;%s %s;", topic_name, type_name,
                              variable_name);
    } else if (strcmp(type_name, "uint16_t") == 0) {
        std::vector<uint16_t> uint16_v;
        _uint16_name_value_pair[topic_name] = uint16_v; // magic number
        format_len = snprintf(msg.format, sizeof(msg.format), "%s:uint64_t timestamp;%s %s;", topic_name, type_name,
                              variable_name);
    } else if (strcmp(type_name, "int32_t") == 0) {
        std::vector<int32_t> int32_v;
        _int32_name_value_pair[topic_name] = int32_v; // magic number
        format_len = snprintf(msg.format, sizeof(msg.format), "%s:uint64_t timestamp;%s %s;", topic_name, type_name,
                              variable_name);
    } else if (strcmp(type_name, "uint32_t") == 0) {
        std::vector<uint32_t> uint32_v;
        _uint32_name_value_pair[topic_name] = uint32_v; // magic number
        format_len = snprintf(msg.format, sizeof(msg.format), "%s:uint64_t timestamp;%s %s;", topic_name, type_name,
                              variable_name);
    } else if (strcmp(type_name, "int64_t") == 0) {
        std::vector<int64_t> int64_v;
        _int64_name_value_pair[topic_name] = int64_v; // magic number
        format_len = snprintf(msg.format, sizeof(msg.format), "%s:uint64_t timestamp;%s %s;", topic_name, type_name,
                              variable_name);
    } else if (strcmp(type_name, "uint64_t") == 0) {
        std::vector<uint64_t> uint64_v;
        _uint64_name_value_pair[topic_name] = uint64_v; // magic number
        format_len = snprintf(msg.format, sizeof(msg.format), "%s:uint64_t timestamp;%s %s;", topic_name, type_name,
                              variable_name);
    } else if (strcmp(type_name, "float") == 0) {
        format_len = snprintf(msg.format, sizeof(msg.format), "%s:uint64_t timestamp;%s %s;", topic_name, type_name,
                              variable_name);
    } else if (strcmp(type_name, "double") == 0) {
        format_len = snprintf(msg.format, sizeof(msg.format), "%s:uint64_t timestamp;%s %s;", topic_name, type_name,
                              variable_name);
    } else if (strcmp(type_name, "bool") == 0) {
        std::vector<bool> bool_v;
        _bool_name_value_pair[topic_name] = bool_v; // magic number
        format_len = snprintf(msg.format, sizeof(msg.format), "%s:uint64_t timestamp;%s %s;", topic_name, type_name,
                              variable_name);
    } else if (strcmp(type_name, "char") == 0) {
        std::vector<char> char_v;
        _char_name_value_pair[topic_name] = char_v; // magic number
        format_len = snprintf(msg.format, sizeof(msg.format), "%s:uint64_t timestamp;%s %s;", topic_name, type_name,
                              variable_name);
    } else if (strcmp(type_name, "lanes") == 0) {
        format_len = snprintf(msg.format, sizeof(msg.format),
                              "%s:"
                              "uint64_t timestamp;"
                              "double[6] leftleft_cubic;"
                              "double[6] left_cubic;"
                              "double[6] right_cubic;"
                              "double[6] rightright_cubic;",
                              topic_name);
    } else if (strcmp(type_name, "io_hz") == 0) {
        format_len = snprintf(msg.format, sizeof(msg.format),
                              "%s:"
                                "uint64_t timestamp;"
                                "int8_t imu_hz;"
                                "int8_t gps_hz;"
                                "int8_t vision_hz;"
                                "int8_t speed_hz;"
                                "int8_t fusion_hz;"
                                "int8_t match_hz;"
                                "int8_t road_hz;"
                                "int8_t pilot_hz;",
                              topic_name);
    } else if (strcmp(type_name, "ehpout_state") == 0) {
        format_len = snprintf(msg.format, sizeof(msg.format),
                              "%s:"
                                "uint64_t timestamp;"
                                "int8_t is_geofencing;"
                                "int8_t scene_detection;"
                                "int32_t longitude;"
                                "int32_t latitude;"
                                "float heading;"
                                "uint64_t linkid;"
                                "uint64_t laneid;"
                                "int8_t laneseq;"
                                "int8_t loc_state;"
                                "int8_t me_match_state;"
                                "int8_t speed_state;"
                                "int8_t imu_state;"
                                "int8_t gps_state;"
                                "int8_t hdmap_state;"
                                "int8_t other_state;",
                              topic_name);
    } else if (strcmp(type_name, "fused_state") == 0) {
        format_len = snprintf(msg.format, sizeof(msg.format),
                              "%s:"
                                "uint64_t timestamp;"
                                "int8_t ahrs_status;"
                                "int8_t state_1;"
                                "int8_t state_2;"
                                "int8_t state_3;",
                              topic_name);
    } else if (strcmp(type_name, "roadloc_state") == 0) {
        format_len = snprintf(msg.format, sizeof(msg.format),
                              "%s:"
                                "uint64_t timestamp;"
                                "uint32_t lon;"
                                "uint32_t lat;"
                                "uint64_t linkid;"
                                "int16_t linktype;"
                                "int16_t a_P;"
                                "int16_t n_P;"
                                "int16_t state_code;",
                              topic_name);
    } else if (strcmp(type_name, "lanematch_state") == 0) {
        format_len = snprintf(msg.format, sizeof(msg.format),
                              "%s:"
                                "uint64_t timestamp;"
                                "uint8_t scene;"
                                "int8_t lane_status;"
                                "int8_t matchvis_status;"
                                "int16_t loc_status;"
                                "int16_t matcher_status;",
                              topic_name);
    } else if (strcmp(type_name, "hdpilot_state") == 0) {
        format_len = snprintf(msg.format, sizeof(msg.format),
                              "%s:"
                                "uint64_t timestamp;"
                                "int8_t eva_status;"
                                "int8_t left_sequence;"
                                "int32_t lane_id;"
                                "int8_t check_link_delay;",
                              topic_name);
    } else if (strcmp(type_name, "gnss") == 0) {
        format_len = snprintf(msg.format, sizeof(msg.format),
                              "%s:"
                                "uint64_t timestamp;"
                                "uint64_t utc_stamp;"
                                "int8_t fixType;"
                                "int8_t posType;"
                                "int8_t numSV;"
                                "uint32_t lon;"
                                "uint32_t lat;"
                                "int16_t height;"
                                "float hAcc;"
                                "float vAcc;"
                                "float velN;"
                                "float velE;"
                                "float velD;"
                                "float headMot;",
                              topic_name);
    } else if (strcmp(type_name, "vision") == 0) {
        format_len = snprintf(msg.format, sizeof(msg.format),
                              "%s:"
                                "uint64_t timestamp;"
                                "double 00_c3;"
                                "double 00_c2;"
                                "double 00_c1;"
                                "double 00_c0;"
                                "float 00_start;"
                                "float 00_end;"
                                "int8_t 00_type;"
                                "int8_t 00_color;"
                                "int8_t 00_qual;"

                                "double 01_c3;"
                                "double 01_c2;"
                                "double 01_c1;"
                                "double 01_c0;"
                                "float 01_start;"
                                "float 01_end;"
                                "int8_t 01_type;"
                                "int8_t 01_color;"
                                "int8_t 01_qual;"

                                "double 02_c3;"
                                "double 02_c2;"
                                "double 02_c1;"
                                "double 02_c0;"
                                "float 02_start;"
                                "float 02_end;"
                                "int8_t 02_type;"
                                "int8_t 02_color;"
                                "int8_t 02_qual;"

                                "double 03_c3;"
                                "double 03_c2;"
                                "double 03_c1;"
                                "double 03_c0;"
                                "float 03_start;"
                                "float 03_end;"
                                "int8_t 03_type;"
                                "int8_t 03_color;"
                                "int8_t 03_qual;"

                                "double 04_c3;"
                                "double 04_c2;"
                                "double 04_c1;"
                                "double 04_c0;"
                                "float 04_start;"
                                "float 04_end;"
                                "int8_t 04_type;"
                                "int8_t 04_color;"
                                "int8_t 04_qual;"

                                "double 05_c3;"
                                "double 05_c2;"
                                "double 05_c1;"
                                "double 05_c0;"
                                "float 05_start;"
                                "float 05_end;"
                                "int8_t 05_type;"
                                "int8_t 05_color;"
                                "int8_t 05_qual;",
                              topic_name);
    } else if (strcmp(type_name, "imu") == 0) {
        format_len = snprintf(msg.format, sizeof(msg.format),
                              "%s:"
                                "uint64_t timestamp;"
                                "int8_t imu_status;"
                                "int8_t imu_calibed;"
                                "double linacc_x;"
                                "double linacc_y;"
                                "double linacc_z;"
                                "double angvel_x;"
                                "double angvel_y;"
                                "double angvel_z;",
                              topic_name);
    } else if (strcmp(type_name, "speed") == 0) {
        format_len = snprintf(msg.format, sizeof(msg.format),
                              "%s:"
                                "uint64_t timestamp;"
                                "float veh_speed;",
                              topic_name);
    }
    else {
        std::cout << "[ulogger]:Error Variable Type:" << type_name << std::endl;
        _ulogger_mutex.unlock();
        return false;
    }
    _topic_name_id_pair[topic_name] = _topic_name_id++;
    size_t msg_size = sizeof(msg) - sizeof(msg.format) + format_len;
    msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
    write_message(&msg, msg_size, true);
    _ulogger_mutex.unlock();
//    write_time_series_data_format(topic_name);
    std::cout << "[ulogger]:" << type_name << " type " << topic_name << "/" << variable_name
              << " registered to Ulogger!" << std::endl;
    return true;
}

bool Ulogger::write_all_registered_time_series_data_format() {
    for (auto it:_topic_name_id_pair) {
        write_time_series_data_format(it.first);
    }
    _registered_topic_written = true;
    return true;
}

bool Ulogger::write_message(void *ptr, size_t size, bool record_as_headr) {
    _mmap_writer.write_data((uint8_t*)ptr, size);
    // 如果这则消息是ulg头, 需要将其写入到_ulg_header中
    if (record_as_headr) {
        _mmap_writer.write_ulg_headr((uint8_t*)ptr, size);
    }
    return true;
}

void Ulogger::write_time_series_data_format(const char *topic_name) {
    _ulogger_mutex.lock();
    ulog_message_add_logged_s msg = {};
    auto message_name_len = strlen(topic_name);
    size_t msg_size = sizeof(msg) - sizeof(msg.message_name) + message_name_len;
    memcpy(msg.message_name, topic_name, message_name_len);
    msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
    msg.msg_id = _topic_name_id_pair.find(topic_name)->second;
    write_message(&msg, msg_size, true);
    std::cout << "[ulogger]:" << msg.message_name << " written define." << std::endl;
    _ulogger_mutex.unlock();
}

bool Ulogger::write_data(const char *topic_name, uint64_t time_stamp_us, double variable_value) {
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
    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data(const char *topic_name, uint64_t time_stamp_us, int8_t variable_value) {
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
    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data(const char *topic_name, uint64_t time_stamp_us, uint8_t variable_value) {
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
    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data(const char *topic_name, uint64_t time_stamp_us, int16_t variable_value) {
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
    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data(const char *topic_name, uint64_t time_stamp_us, uint16_t variable_value) {
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
    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data(const char *topic_name, uint64_t time_stamp_us, int32_t variable_value) {
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
    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data(const char *topic_name, uint64_t time_stamp_us, uint32_t variable_value) {
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
    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data(const char *topic_name, uint64_t time_stamp_us, int64_t variable_value) {
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
    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data(const char *topic_name, uint64_t time_stamp_us, uint64_t variable_value) {
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
    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

/*
 * Write integer while value changed
 * For saving flash disk space
 * */
bool Ulogger::write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, int8_t variable_value) {
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

    if (!_int8_name_value_pair.empty()) {
        auto it_int = _int8_name_value_pair.find(topic_name);
        if (it_int == _int8_name_value_pair.end()) {
            std::cout << "[ulogger]:ERROR! NO topic " << topic_name << std::endl;
            _ulogger_mutex.unlock();
            assert(0);
        }
        if (!it_int->second.empty()) {
            if (it_int->second[0] == variable_value) {
                _ulogger_mutex.unlock();
                return false;
            }
        } else {
            it_int->second.push_back(variable_value);
        }
        it_int->second[0] = variable_value;
    }

    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, uint8_t variable_value) {
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

    if (!_uint8_name_value_pair.empty()) {
        auto it_int = _uint8_name_value_pair.find(topic_name);
        if (it_int == _uint8_name_value_pair.end()) {
            std::cout << "[ulogger]:ERROR! NO topic " << topic_name << std::endl;
            _ulogger_mutex.unlock();
            assert(0);
        }
        if (!it_int->second.empty()) {
            if (it_int->second[0] == variable_value) {
                _ulogger_mutex.unlock();
                return false;
            }
        } else {
            it_int->second.push_back(variable_value);
        }
        it_int->second[0] = variable_value;
    }

    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, int16_t variable_value) {
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

    if (!_int16_name_value_pair.empty()) {
        auto it_int = _int16_name_value_pair.find(topic_name);
        if (it_int == _int16_name_value_pair.end()) {
            std::cout << "[ulogger]:ERROR! NO topic " << topic_name << std::endl;
            _ulogger_mutex.unlock();
            assert(0);
        }
        if (!it_int->second.empty()) {
            if (it_int->second[0] == variable_value) {
                _ulogger_mutex.unlock();
                return false;
            }
        } else {
            it_int->second.push_back(variable_value);
        }
        it_int->second[0] = variable_value;
    }

    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, uint16_t variable_value) {
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

    if (!_uint16_name_value_pair.empty()) {
        auto it_int = _uint16_name_value_pair.find(topic_name);
        if (it_int == _uint16_name_value_pair.end()) {
            std::cout << "[ulogger]:ERROR! NO topic " << topic_name << std::endl;
            _ulogger_mutex.unlock();
            assert(0);
        }
        if (!it_int->second.empty()) {
            if (it_int->second[0] == variable_value) {
                _ulogger_mutex.unlock();
                return false;
            }
        } else {
            it_int->second.push_back(variable_value);
        }
        it_int->second[0] = variable_value;
    }

    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, int32_t variable_value) {
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

    if (!_int32_name_value_pair.empty()) {
        auto it_int = _int32_name_value_pair.find(topic_name);
        if (it_int == _int32_name_value_pair.end()) {
            std::cout << "[ulogger]:ERROR! NO topic " << topic_name << std::endl;
            _ulogger_mutex.unlock();
            assert(0);
        }
        if (!it_int->second.empty()) {
            if (it_int->second[0] == variable_value) {
                _ulogger_mutex.unlock();
                return false;
            }
        } else {
            it_int->second.push_back(variable_value);
        }
        it_int->second[0] = variable_value;
    }

    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, uint32_t variable_value) {
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

    if (!_uint32_name_value_pair.empty()) {
        auto it_int = _uint32_name_value_pair.find(topic_name);
        if (it_int == _uint32_name_value_pair.end()) {
            std::cout << "[ulogger]:ERROR! NO topic " << topic_name << std::endl;
            _ulogger_mutex.unlock();
            assert(0);
        }
        if (!it_int->second.empty()) {
            if (it_int->second[0] == variable_value) {
                _ulogger_mutex.unlock();
                return false;
            }
        } else {
            it_int->second.push_back(variable_value);
        }
        it_int->second[0] = variable_value;
    }

    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, int64_t variable_value) {
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

    if (!_int64_name_value_pair.empty()) {
        auto it_int = _int64_name_value_pair.find(topic_name);
        if (it_int == _int64_name_value_pair.end()) {
            std::cout << "[ulogger]:ERROR! NO topic " << topic_name << std::endl;
            _ulogger_mutex.unlock();
            assert(0);
        }
        if (!it_int->second.empty()) {
            if (it_int->second[0] == variable_value) {
                _ulogger_mutex.unlock();
                return false;
            }
        } else {
            it_int->second.push_back(variable_value);
        }
        it_int->second[0] = variable_value;
    }

    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, uint64_t variable_value) {
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

    if (!_uint64_name_value_pair.empty()) {
        auto it_int = _uint64_name_value_pair.find(topic_name);
        if (it_int == _uint64_name_value_pair.end()) {
            std::cout << "[ulogger]:ERROR! NO topic " << topic_name << std::endl;
            _ulogger_mutex.unlock();
            assert(0);
        }
        if (!it_int->second.empty()) {
            if (it_int->second[0] == variable_value) {
                _ulogger_mutex.unlock();
                return false;
            }
        } else {
            it_int->second.push_back(variable_value);
        }
        it_int->second[0] = variable_value;
    }

    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

bool Ulogger::write_data_if_value_changed(const char *topic_name, uint64_t time_stamp_us, bool variable_value) {
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

    if (!_bool_name_value_pair.empty()) {
        auto it_int = _bool_name_value_pair.find(topic_name);
        if (it_int == _bool_name_value_pair.end()) {
            std::cout << "[ulogger]:ERROR! NO topic " << topic_name << std::endl;
            _ulogger_mutex.unlock();
            assert(0);
        }
        if (!it_int->second.empty()) {
            if (it_int->second[0] == variable_value) {
                _ulogger_mutex.unlock();
                return false;
            }
        } else {
            it_int->second.push_back(variable_value);
        }
        it_int->second[0] = variable_value;
    }

    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value)];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + sizeof(variable_value);
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), &variable_value, sizeof(variable_value));
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}
/*
 * Lanes:
 * left    left: a b c d
 *         left: a b c d
 *        right: a b c d
 * right  right: a b c d
 * x_start
 * x_end
 * */

bool Ulogger::write_data(const char *topic_name, uint64_t time_stamp_us, std::vector<std::vector<double>> lanes) {
    if (!_ulg_conf.is_log_on) {
        return false;
    }
    if (_topic_name_id_pair.empty()) {
        return false;
    }
    if (!_registered_topic_written) {
        _registered_topic_written = write_all_registered_time_series_data_format();
    }
    if (lanes.empty()) {
        return false;
    }
    _ulogger_mutex.lock();
    auto variable_count = lanes.size() * lanes.back().size() * sizeof(double);
//    std::cout << "variable_count:" << variable_count << std::endl;
    auto it = _topic_name_id_pair.find(topic_name);
    if (it != _topic_name_id_pair.end()) {
        ulog_message_data_header_s msg = {};
        auto *buffer = new uint8_t[sizeof(msg) + sizeof(time_stamp_us) + variable_count];
        size_t msg_size = sizeof(msg) + sizeof(time_stamp_us) + variable_count;
        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);
        msg.msg_id = it->second;
        memcpy(buffer, &msg, sizeof(msg));
        memcpy(buffer + sizeof(msg), &time_stamp_us, sizeof(time_stamp_us));
        double lanes_buf[variable_count];
        uint8_t l_count = 0;
        for (auto ls:lanes) {
            for (auto l:ls) {
                lanes_buf[l_count++] = l;
            }
        }
        memcpy(buffer + sizeof(msg) + sizeof(time_stamp_us), lanes_buf, variable_count);
        write_message(buffer, msg_size);
        delete[]buffer;
        _ulogger_mutex.unlock();
        return true;
    } else {
        _ulogger_mutex.unlock();
        return false;
    }
}

void Ulogger::write_info(const char *name, const char *value) {
    if (!_ulg_conf.is_log_on) {
        return;
    }
    _ulogger_mutex.lock();
    ulog_message_info_header_s msg = {};
    uint8_t *buffer = reinterpret_cast<uint8_t *>(&msg);
    msg.msg_type = static_cast<uint8_t>(ULogMessageType::INFO);

    /* construct format key (type and name) */
    size_t vlen = strlen(value);
    msg.key_len = (uint8_t) snprintf(msg.key, sizeof(msg.key), "char[%zu] %s", vlen, name);
    size_t msg_size = sizeof(msg) - sizeof(msg.key) + msg.key_len;
//    memcpy(&buffer, &msg, 3);
    /* copy string value directly to buffer */
    if (vlen < (sizeof(msg) - msg_size)) {
        memcpy(&buffer[msg_size], value, vlen);
        msg_size += vlen;

        msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);

        write_message(buffer, msg_size, true);
    }
    _ulogger_mutex.unlock();
}

/*
 * 0"EMERGENCY"
 * 1"ALERT"
 * 2"CRITICAL"
 * 3"ERROR"
 * 4"WARNING"
 * 5"NOTICE"
 * 6"INFO"
 * 7"DEBUG"
 * */
void Ulogger::log_printf(uint64_t time_stamp_us, const char *format, ...) {
    if (!_ulg_conf.is_log_on) {
        return;
    }
    if (!_registered_topic_written) {
        return;
    }
    _ulogger_mutex.lock();
    ulog_message_logging_s msg = {};
    va_list args;
    va_start(args, format);
    auto message_len = vsprintf(msg.message, format, args);
    va_end(args);
    uint16_t write_msg_size = sizeof(ulog_message_logging_s) - (sizeof(ulog_message_logging_s::message) - message_len);
    msg.msg_size = (uint16_t) (write_msg_size - ULOG_MSG_HEADER_LEN);
    msg.log_level = '4';
    msg.timestamp = time_stamp_us * 1000;
    write_message(&msg, write_msg_size);
    _ulogger_mutex.unlock();
}

void Ulogger::write_info(const char *name, int32_t value) {
    write_info_template<int32_t>(name, value, "int32_t");
}

template<typename T>
void Ulogger::write_info_template(const char *name, T value, const char *type_str) {
    _ulogger_mutex.lock();
    ulog_message_info_header_s msg = {};
    auto *buffer = reinterpret_cast<uint8_t *>(&msg);
    msg.msg_type = static_cast<uint8_t>(ULogMessageType::INFO);

    /* construct format key (type and name) */
    msg.key_len = (uint8_t) snprintf(msg.key, sizeof(msg.key), "%s %s", type_str, name);
    size_t msg_size = sizeof(msg) - sizeof(msg.key) + msg.key_len;

    /* copy string value directly to buffer */
    memcpy(&buffer[msg_size], &value, sizeof(T));
    msg_size += sizeof(T);

    msg.msg_size = (uint16_t) (msg_size - ULOG_MSG_HEADER_LEN);

    write_message(buffer, msg_size);
    _ulogger_mutex.unlock();
}

void Ulogger::write_header(void) {
    if (!_ulg_conf.is_log_on) {
        return;
    }
    ulog_file_header_s header = {};
    header.magic[0] = 'U';
    header.magic[1] = 'L';
    header.magic[2] = 'o';
    header.magic[3] = 'g';
    header.magic[4] = 0x01;
    header.magic[5] = 0x12;
    header.magic[6] = 0x35;
    header.magic[7] = 0x00; //file version 1
    header.timestamp = 0x00;//hrt_absolute_time();
    _ulogger_mutex.lock();
    write_message(&header, sizeof(header), true);
    // write the Flags message: this MUST be written right after the ulog header
    ulog_message_flag_bits_s flag_bits{};

    flag_bits.msg_size = sizeof(flag_bits) - ULOG_MSG_HEADER_LEN;
    flag_bits.msg_type = static_cast<uint8_t>(ULogMessageType::FLAG_BITS);

    write_message(&flag_bits, sizeof(flag_bits), true);
    _ulogger_mutex.unlock();
}

void Ulogger::write_version(void) {
    if (!_ulg_conf.is_log_on) {
        return;
    }
    write_info("LocalizationAnalysis", "V1.0.0");
    struct utsname u;
    uname(&u);
    write_info("BuildSystemArch", u.machine);
    write_info("BuildSystem", u.sysname);
    write_info("BuildSystemUser", u.nodename);
    write_info("BuildSystemVersion", u.version);
//    write_info("BuildSystemDomain", u.domainname);
}

//void Ulogger::write_parameters(void)
//{
//    _writer.lock();
//    ulog_message_parameter_header_s msg = {};
//    uint8_t *buffer = reinterpret_cast<uint8_t *>(&msg);
//
//    msg.msg_type = static_cast<uint8_t>(ULogMessageType::PARAMETER);
//    int param_idx = 0;
//    param_t param = 0;
//
//    do {
//        // skip over all parameters which are not invalid and not used
//        do {
//            param = param_for_index(param_idx);
//            ++param_idx;
//        } while (param != PARAM_INVALID && !param_used(param));
//
//        // save parameters which are valid AND used
//        if (param != PARAM_INVALID) {
//            // get parameter type and size
//            const char *type_str;
//            param_type_t ptype = param_type(param);
//            size_t value_size = 0;
//
//            switch (ptype) {
//                case PARAM_TYPE_INT32:
//                    type_str = "int32_t";
//                    value_size = sizeof(int32_t);
//                    break;
//
//                case PARAM_TYPE_FLOAT:
//                    type_str = "float";
//                    value_size = sizeof(float);
//                    break;
//
//                default:
//                    continue;
//            }
//
//            // format parameter key (type and name)
//            msg.key_len = snprintf(msg.key, sizeof(msg.key), "%s %s", type_str, param_name(param));
//            size_t msg_size = sizeof(msg) - sizeof(msg.key) + msg.key_len;
//
//            // copy parameter value directly to buffer
//            switch (ptype) {
//                case PARAM_TYPE_INT32:
//                    param_get(param, (int32_t *)&buffer[msg_size]);
//                    break;
//
//                case PARAM_TYPE_FLOAT:
//                    param_get(param, (float *)&buffer[msg_size]);
//                    break;
//
//                default:
//                    continue;
//            }
//
//            msg_size += value_size;
//
//            msg.msg_size = msg_size - ULOG_MSG_HEADER_LEN;
//
//            write_message(buffer, msg_size);
//        }
//    } while ((param != PARAM_INVALID) && (param_idx < (int) param_count()));
//
//    _writer.unlock();
//    _writer.notify();
//}
