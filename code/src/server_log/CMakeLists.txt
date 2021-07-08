cmake_minimum_required(VERSION 2.8)

project(server_log)
add_definitions(-DLOAD_PLUGIN=load_${PROJECT_NAME})
add_definitions(-DRUN_PLUGIN=run_${PROJECT_NAME})
add_definitions(-DUNLOAD_PLUGIN=unload_${PROJECT_NAME})

find_package(Protobuf REQUIRED)
include_directories(
    ${GLOG_INCLUDE_DIR}
    ${CORELIB_INCLUDE_DIR}
    ${PROTOBUF_INCLUDE_DIR}
    ${DATA_INCLUDE_DIR}
    ${UTILITIES_INCLUDE_DIR}
    )

ADDLIB(${PROJECT_NAME} src
    )
