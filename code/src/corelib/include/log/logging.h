#pragma once
//ceres依赖glog,所以不能去除glog
#ifdef USE_GLOG
#include <glog/logging.h>
#define DEBUG()               LOG(DEBUG)
#define INFO()                LOG(INFO)
#define WARNING()             LOG(WARNING)
#define ERROR()               LOG(ERROR)
#define FATAL()               LOG(FATAL)
#define DIRECT()              VLOG(0)

#define DATAINFO(name, value) INFO() << #name": " << value
#define SET_LOG_LEVEL(level) FLAGS_minloglevel=google::GLOG_##level
#else
#include "iv_log.h"
#define GLOG_NO_ABBREVIATED_SEVERITIES
#define DEBUG()               iv_log::Debug(__FILE__, __LINE__)
#define INFO()                iv_log::Info(__FILE__, __LINE__)
#define WARNING()             iv_log::Warning(__FILE__, __LINE__)
#define ERROR()               iv_log::Error(__FILE__, __LINE__)
#define FATAL()               iv_log::Fatal(__FILE__, __LINE__)
#define DIRECT()               iv_log::Direct(__FILE__, __LINE__)

#define DATAINFO(name, value) INFO() << #name": " << value
#define DATAINFO1(v) DATAINFO(v, v)
#define SET_LOG_LEVEL(level) LogInterface::set_log_level(level)
#endif

#define DEBUG_IF(condition)   if(condition) DEBUG() << #condition" is true: "
#define INFO_IF(condition)    if(condition) INFO() << #condition" is true: "
#define WARNING_IF(condition) if(condition) WARNING() << #condition" is true: "
#define ERROR_IF(condition)   if(condition) ERROR() << #condition" is true: "
#define FATAL_IF(condition)   if(condition) FATAL() << #condition" is true: "
#define DIRECT_IF(condition)   if(condition) DIRECT() << #condition" is true: "

#define DEBUG_IF_NOT(condition)   if(!condition) DEBUG() << #condition" is false: "
#define INFO_IF_NOT(condition)    if(!condition) INFO() << #condition" is false: "
#define WARNING_IF_NOT(condition) if(!condition) WARNING() << #condition" is false: "
#define ERROR_IF_NOT(condition)   if(!condition) ERROR() << #condition" is false: "
#define FATAL_IF_NOT(condition)   if(!condition) FATAL() << #condition" is false: "
#define DIRECT_IF_NOT(condition)   if(!condition) DIRECT() << #condition" is false: "
