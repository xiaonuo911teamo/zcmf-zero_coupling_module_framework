cmake_minimum_required(VERSION 2.8)

project(corelib)

add_compile_options(-std=c++11)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

ADDLIB(${PROJECT_NAME} src)
