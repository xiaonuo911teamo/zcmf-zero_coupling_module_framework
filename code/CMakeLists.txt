cmake_minimum_required(VERSION 3.5)

project(zcmf-zero_couping_moudle_framework)
set(CMAKE_CXX_STANDARD 11)
add_compile_options(-fPIC -O3)
set(CMAKE_CXX_FLAGS -rdynamic)
if(NOT BUILD_TYPE)
    set(BUILD_TYPE "linux-x86_64" CACHE INTERNAL "BUILD_TYPE")
elseif(NOT CMAKE_TOOLCHAIN_FILE AND ${BUILD_TYPE} STREQUAL "qnx-aarch64")
    MESSAGE(FATAL_ERROR  "CMAKE_TOOLCHAIN_FILE is not define")
endif()

message(STATUS "++++++++++++++ ${BUILD_TYPE}")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../opt/${BUILD_TYPE}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../opt/${BUILD_TYPE}/lib)

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_LIST_DIR}/../inner-depend/3rdParty/${BUILD_TYPE} CACHE INTERNAL "THIRD_PARTY_DIR")

set(CORELIB_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/corelib/include/ CACHE INTERNAL "CORELIB_INCLUDE_DIR")
set(DATA_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/proto_data/ CACHE INTERNAL "DATA_INCLUDE_DIR")
set(UTILITIES_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/utilities/include/ CACHE INTERNAL "UTILITIES_INCLUDE_DIR")

if(${BUILD_TYPE} STREQUAL "linux-x86_64")
    set(DL dl CACHE INTERNAL "DL")
    set(PTHREAD pthread CACHE INTERNAL "PTHREAD")
endif()

find_package(Tools REQUIRED)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src/iv_task)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src/corelib)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src/proto_data)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src/server_log)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src/server_proc)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src/utilities)


if(${BUILD_TYPE} STREQUAL "linux-x86_64")
endif()

