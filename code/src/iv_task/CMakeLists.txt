project(iv_task)

if(${BUILD_TYPE} STREQUAL "linux-x86_64")
    set(DL dl CACHE INTERNAL "DL")
    set(PTHREAD pthread CACHE INTERNAL "PTHREAD")
endif()

include_directories(
    ${CORELIB_INCLUDE_DIR}
    )

add_executable(iv_task
    src/main.cpp
    src/main_loop.cpp
    )

target_link_libraries(iv_task
    corelib
    ${PTHREAD}
    ${DL}
    )
