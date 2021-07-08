cmake_minimum_required(VERSION 2.8)

project(proto_data)

find_package(Protobuf REQUIRED)
#find_package(Glog REQUIRED)
#find_package(Gflags REQUIRED)
include_directories(
    ${PROTOBUF_INCLUDE_DIR}
    ${GLOG_INCLUDE_DIR}
    )

ADDLIB(${PROJECT_NAME} data
    ${GLOG_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    )
