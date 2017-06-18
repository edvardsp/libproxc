cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(libproxc C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

set(SRC ${PROJECT_SOURCE_DIR}/src)

file(GLOB SRC_FILES ${SRC}/*.c)

add_library(proxc_a  STATIC ${SRC_FILES})
add_library(proxc_so SHARED ${SRC_FILES})

set_target_properties(proxc_a proxc_so
    PROPERTIES OUTPUT_NAME proxc
    PUBLIC_HEADER "${SRC}/proxc.h")

add_definitions(-DCTX_IMPL -D_GNU_SOURCE -DNDEBUG)

find_package(Threads)
target_link_libraries(proxc_a  ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(proxc_so ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS proxc_a proxc_so
        LIBRARY DESTINATION "lib"
        ARCHIVE DESTINATION "lib"
        PUBLIC_HEADER DESTINATION "include"
        COMPONENT library)

