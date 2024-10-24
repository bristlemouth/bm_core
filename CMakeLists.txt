cmake_minimum_required(VERSION 3.29)

# - Add Platform File To Build

# This function adds any item in a src/platform directory to the build
# based on the parameters. It allows for specific platform implementations
# to be utilized for the integrator's project. This function works by
# passing in a list of sources, a subname (uppercase) of the file in
# which will be pulled into the build, along with a path to the directory of
# the project (it could be another project/module)
#
# sources - list variable to append the found sources to
# platform - uppercase subname of the platform to be compiled into the
#          project
# path - path to the root directory with a following relative path of
#        src/platform (see this repository for example)
function(append_platform_file sources platform path)
    file(GLOB files "${path}/src/platform/*.c")
    foreach(file ${files})
        string(TOUPPER "${file}" file_upper)
        if (${file_upper} MATCHES ${platform})
            list(APPEND ${sources} ${file})
            set(${sources} "${${sources}}" PARENT_SCOPE)
            break()
        endif()
    endforeach()
endfunction()

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

# Update all submodules recursively
execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

set(COMPILE_FLAGS
    -DCBOR_CUSTOM_ALLOC_INCLUDE="tinycbor_alloc.h"
    -DCBOR_PARSER_MAX_RECURSIONS=10
)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/dfu
    ${bm_common_SOURCE_DIR}/include
    ${bm_common_SOURCE_DIR}/third_party/crc
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
        src/config.c
        src/device.c
        src/heartbeat.c
        src/info.c
        src/neighbors.c
        src/l2.c
        src/middleware.c
        src/packet.c
        src/ping.c
        src/pubsub.c
        src/resource_discovery.c
        src/time.c
        src/topology.c
        src/cbor_service_helper.c
        src/dfu/dfu_client.c
        src/dfu/dfu_core.c
        src/dfu/dfu_host.c
        ${CMAKE_CURRENT_LIST_DIR}/third_party/tinycbor/src/cborparser.c
        ${CMAKE_CURRENT_LIST_DIR}/third_party/tinycbor/src/cborencoder_float.c
        ${CMAKE_CURRENT_LIST_DIR}/third_party/tinycbor/src/cborencoder.c
        ${CMAKE_CURRENT_LIST_DIR}/third_party/tinycbor/src/cborerrorstrings.c
        ${CMAKE_CURRENT_LIST_DIR}/third_party/tinycbor/src/cborvalidation.c
    )

    # Ensure integrator called the functions necessary to configure bristlemouth
    set(ERROR_MESSAGE "")
    if("${BM_IP_STACK}" STREQUAL "" OR NOT BM_IP_INCLUDES)
        set(ERROR_MESSAGE "IP Stack")
    endif()
    if("${BM_OS}" STREQUAL "" OR NOT BM_OS_INCLUDES)
        set(ERROR_MESSAGE "Operating System")
    endif()
    if (NOT "${ERROR_MESSAGE}" STREQUAL "")
        message(FATAL_ERROR "The ${ERROR_MESSAGE} must be chosen in order to\
        build the application. To see how this is configured, please visit:\
        https://bristlemouth.github.io/bm_core/usage.html#build-system-integration"
        )
    endif()
    append_platform_file(SOURCES ${BM_IP_STACK} ${CMAKE_CURRENT_LIST_DIR})
    append_platform_file(SOURCES ${BM_OS} ${bm_common_SOURCE_DIR})
    include_directories(
        "${BM_IP_INCLUDES}"
        "${BM_OS_INCLUDES}"
    )
    add_library(bmcore ${SOURCES})

    target_link_libraries(bmcore PUBLIC bmcommon)
endif()
