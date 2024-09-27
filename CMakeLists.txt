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

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${bm_common_SOURCE_DIR}/include
)


if(ENABLE_TESTING)
    set(CMAKE_CXX_FLAGS "-g -Wall")
    set(CMAKE_C_FLAGS "-g -Wall -W")
    set(CMAKE_EXE_LINKER_FLAGS "-fprofile-arcs -ftest-coverage")

    enable_testing()
    add_subdirectory("test")
else()
    set(SOURCES
        src/device.c
        src/packet.c
        src/service.c
        src/heartbeat.c
        src/bcmp.c
    )
    add_library(bmcore ${SOURCES})

    target_link_libraries(bmcore PUBLIC bmcommon)
endif()
