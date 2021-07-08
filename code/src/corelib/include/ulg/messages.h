//
// Created by nvidia on 10/17/19.
//

#ifndef PROJECT_MESSAGES_H
#define PROJECT_MESSAGES_H

#include <stdint.h>

enum class ULogMessageType : uint8_t {
    FORMAT = 'F',
    DATA = 'D',
    INFO = 'I',
    INFO_MULTIPLE = 'M',
    PARAMETER = 'P',
    ADD_LOGGED_MSG = 'A',
    REMOVE_LOGGED_MSG = 'R',
    SYNC = 'S',
    DROPOUT = 'O',
    LOGGING = 'L',
    LOGGING_TAGGED = 'C',
    FLAG_BITS = 'B',
};

#pragma pack(push, 1)

/** first bytes of the file */
struct ulog_file_header_s {
    uint8_t magic[8];
    uint64_t timestamp;
};

#define ULOG_MSG_HEADER_LEN 3 //accounts for msg_size and msg_type
struct ulog_message_header_s {
    uint16_t msg_size;
    uint8_t msg_type;
};

struct ulog_message_format_s {
    uint16_t msg_size; //size of message - ULOG_MSG_HEADER_LEN
    uint8_t msg_type = static_cast<uint8_t>(ULogMessageType::FORMAT);

    char format[1500];
};

struct ulog_message_add_logged_s {
    uint16_t msg_size; //size of message - ULOG_MSG_HEADER_LEN
    uint8_t msg_type = static_cast<uint8_t>(ULogMessageType::ADD_LOGGED_MSG);

    uint8_t multi_id;
    uint16_t msg_id;
    char message_name[255];
};

struct ulog_message_remove_logged_s {
    uint16_t msg_size; //size of message - ULOG_MSG_HEADER_LEN
    uint8_t msg_type = static_cast<uint8_t>(ULogMessageType::REMOVE_LOGGED_MSG);

    uint16_t msg_id;
};

struct ulog_message_sync_s {
    uint16_t msg_size; //size of message - ULOG_MSG_HEADER_LEN
    uint8_t msg_type = static_cast<uint8_t>(ULogMessageType::SYNC);

    uint8_t sync_magic[8];
};

struct ulog_message_dropout_s {
    uint16_t msg_size = sizeof(uint16_t); //size of message - ULOG_MSG_HEADER_LEN
    uint8_t msg_type = static_cast<uint8_t>(ULogMessageType::DROPOUT);

    uint16_t duration; //in ms
};

struct ulog_message_data_header_s {
    uint16_t msg_size; //size of message - ULOG_MSG_HEADER_LEN
    uint8_t msg_type = static_cast<uint8_t>(ULogMessageType::DATA);

    uint16_t msg_id;
};

struct ulog_message_info_header_s {
    uint16_t msg_size; //size of message - ULOG_MSG_HEADER_LEN
    uint8_t msg_type = static_cast<uint8_t>(ULogMessageType::INFO);

    uint8_t key_len;
    char key[255];
};

struct ulog_message_info_multiple_header_s {
    uint16_t msg_size; //size of message - ULOG_MSG_HEADER_LEN
    uint8_t msg_type = static_cast<uint8_t>(ULogMessageType::INFO_MULTIPLE);

    uint8_t is_continued; ///< can be used for arrays: set to 1, if this message is part of the previous with the same key
    uint8_t key_len;
    char key[255];
};

struct ulog_message_logging_s {
    uint16_t msg_size; //size of message - ULOG_MSG_HEADER_LEN
    uint8_t msg_type = static_cast<uint8_t>(ULogMessageType::LOGGING);

    uint8_t log_level; //same levels as in the linux kernel
    uint64_t timestamp;
    char message[128]; //defines the maximum length of a logged message string
};

struct ulog_message_logging_tagged_s {
    uint16_t msg_size; //size of message - ULOG_MSG_HEADER_LEN
    uint8_t msg_type = static_cast<uint8_t>(ULogMessageType::LOGGING_TAGGED);

    uint8_t log_level; //same levels as in the linux kernel
    uint16_t tag;
    uint64_t timestamp;
    char message[128]; //defines the maximum length of a logged message string
};

struct ulog_message_parameter_header_s {
    uint16_t msg_size;
    uint8_t msg_type = static_cast<uint8_t>(ULogMessageType::PARAMETER);

    uint8_t key_len;
    char key[255];
};


#define ULOG_INCOMPAT_FLAG0_DATA_APPENDED_MASK (1<<0)

struct ulog_message_flag_bits_s {
    uint16_t msg_size;
    uint8_t msg_type = static_cast<uint8_t>(ULogMessageType::FLAG_BITS);

    uint8_t compat_flags[8];
    uint8_t incompat_flags[8]; ///< @see ULOG_INCOMPAT_FLAG_*
    uint64_t appended_offsets[3]; ///< file offset(s) for appended data if ULOG_INCOMPAT_FLAG0_DATA_APPENDED_MASK is set
};

#pragma pack(pop)

#endif //PROJECT_MESSAGES_H
