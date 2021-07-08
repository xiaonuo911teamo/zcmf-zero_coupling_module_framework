MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

MACRO(ADDEXE name dir)
    file(GLOB_RECURSE SRC_LIST ${dir}/*.cpp ${dir}/*.cc)
    if(SRC_LIST)
        add_executable(${name}
            ${SRC_LIST}
        )
        target_link_libraries(${name}
            ${ARGN}
        )
    endif()
ENDMACRO()

MACRO(ADDLIB name dir)
    file(GLOB_RECURSE SRC_LIST ${dir}/*.cpp ${dir}/*.cc)
    if(SRC_LIST)
        add_library(${name} SHARED
            ${SRC_LIST}
        )
        target_link_libraries(${name}
            ${ARGN}
        )
    endif()
ENDMACRO()

MACRO(ADDSLIB name dir)
    file(GLOB_RECURSE SRC_LIST ${dir}/*.cpp ${dir}/*.cc)
    if(SRC_LIST)
        add_library(${name} STATIC
            ${SRC_LIST}
        )
        target_link_libraries(${name}
            ${ARGN}
        )
    endif()
ENDMACRO()
