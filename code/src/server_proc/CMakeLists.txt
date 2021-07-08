cmake_minimum_required(VERSION 2.8)

project(server_proc)
set(CMAKE_CXX_STANDARD 11)
add_definitions(-DLOAD_PLUGIN=load_${PROJECT_NAME})
add_definitions(-DRUN_PLUGIN=run_${PROJECT_NAME})
add_definitions(-DUNLOAD_PLUGIN=unload_${PROJECT_NAME})

find_package(Protobuf REQUIRED)
find_package(Zmq REQUIRED)
include_directories(
    ${CORELIB_INCLUDE_DIR}
    ${PROTOBUF_INCLUDE_DIR}
    ${ZMQ_INCLUDE_DIR}
    ${DATA_INCLUDE_DIR}
    ${UTILITIES_INCLUDE_DIR}
    )

ADDLIB(${PROJECT_NAME} src
    ${ZMQ_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    proto_data
    proc
    )
