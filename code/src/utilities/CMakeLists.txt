cmake_minimum_required(VERSION 2.8)

project(utilities)
set(CMAKE_CXX_STANDARD 11)

find_package(Protobuf REQUIRED)
#find_package(Glog REQUIRED)
find_package(Zmq REQUIRED)
find_package(Tools REQUIRED)
message(${PROTOBUF_INCLUDE_DIR})
include_directories(
    ${ZMQ_INCLUDE_DIR}
    ${GLOG_INCLUDE_DIR}
    ${CORELIB_INCLUDE_DIR}
    ${PROTOBUF_INCLUDE_DIR}
    ${DATA_INCLUDE_DIR}
    ./include/
    )

if(${BUILD_TYPE} STREQUAL "linaro-aarch64")
    add_library(proc SHARED
        src/proc.cpp
    )
else()
    add_library(proc SHARED
        src/proc.cpp
    )
endif()

target_link_libraries(proc
    ${PROTOBUF_LIBRARIES}
    proto_data
)

add_executable(sysproc
    src/proc_main.cpp
)
target_link_libraries(sysproc
    ${PROTOBUF_LIBRARIES}
    proc
    proto_data
)
