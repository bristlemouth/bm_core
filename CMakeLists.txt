cmake_minimum_required(VERSION 3.29)

if(NOT ENABLE_TESTING)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/arm-gcc-toolchain.cmake)
endif()

project(Bristlemouth VERSION 0.11 LANGUAGES C)

include(FetchContent)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_C_EXTENSIONS NO)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FetchContent_Declare(
  bm_common
  GIT_REPOSITORY https://github.com/bristlemouth/bm_common.git
  #TODO: This must be set by configuration somehow
  GIT_TAG        main
)
FetchContent_MakeAvailable(bm_common)

#TODO: Configuration will determine RTOS and IP stack here

set(COMPILE_FLAGS
    -DCBOR_CUSTOM_ALLOC_INCLUDE="tinycbor_alloc.h"
    -DCBOR_PARSER_MAX_RECURSIONS=10
)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${bm_common_SOURCE_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/third_party/tinycbor/src
)


if(ENABLE_TESTING)
    set(CMAKE_CXX_FLAGS "-g -Wall")
    set(CMAKE_C_FLAGS "-g -Wall -W")
    set(CMAKE_EXE_LINKER_FLAGS "-fprofile-arcs -ftest-coverage")

    enable_testing()
    add_subdirectory("test")
else()
    set(SOURCES
        src/bcmp.c
        src/device.c
        src/heartbeat.c
        src/info.c
        src/neighbors.c
        src/packet.c
        src/ping.c
        src/resource_discovery.c
        src/topology.c
        ${CMAKE_CURRENT_LIST_DIR}/third_party/tinycbor/src/cborparser.c
        ${CMAKE_CURRENT_LIST_DIR}/third_party/tinycbor/src/cborencoder_float.c
        ${CMAKE_CURRENT_LIST_DIR}/third_party/tinycbor/src/cborencoder.c
        ${CMAKE_CURRENT_LIST_DIR}/third_party/tinycbor/src/cborerrorstrings.c
        ${CMAKE_CURRENT_LIST_DIR}/third_party/tinycbor/src/cborvalidation.c
    )
    add_library(bmcore ${SOURCES})

    target_link_libraries(bmcore PUBLIC bmcommon)
endif()
