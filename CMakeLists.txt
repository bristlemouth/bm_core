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
    file(GLOB files "${path}/*.c")
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

# Update all submodules recursively
execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

add_compile_definitions(
    -DCBOR_CUSTOM_ALLOC_INCLUDE="tinycbor_alloc.h"
    -DCBOR_PARSER_MAX_RECURSIONS=10
)

set(BM_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/bcmp
    ${CMAKE_CURRENT_LIST_DIR}/common
    ${CMAKE_CURRENT_LIST_DIR}/drivers/adin2111
    ${CMAKE_CURRENT_LIST_DIR}/middleware
    ${CMAKE_CURRENT_LIST_DIR}/network
    ${CMAKE_CURRENT_LIST_DIR}/third_party
    ${CMAKE_CURRENT_LIST_DIR}/third_party/crc
    ${CMAKE_CURRENT_LIST_DIR}/third_party/tinycbor/src
    ${CMAKE_CURRENT_LIST_DIR}/bm_common_messages
)
include_directories(${BM_INCLUDES})

if(ENABLE_TESTING)
    set(CMAKE_CXX_FLAGS "-g -Wall")
    set(CMAKE_C_FLAGS "-g -Wall -W")
    set(CMAKE_EXE_LINKER_FLAGS "-fprofile-arcs -ftest-coverage")
    add_compile_options(-DENABLE_TESTING)

    enable_testing()
    add_subdirectory("test")
else()
    set(BM_SOURCES "")

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
    append_platform_file(BM_SOURCES ${BM_IP_STACK} ${CMAKE_CURRENT_LIST_DIR}/network)
    append_platform_file(BM_SOURCES ${BM_OS} ${CMAKE_CURRENT_LIST_DIR}/common)
    include_directories(
        "${BM_IP_INCLUDES}"
        "${BM_OS_INCLUDES}"
    )

    #Add core library
    add_library(bmcore ${BM_SOURCES})

    # Make includes available to above CMakeLists directories
    target_include_directories(bmcore PUBLIC ${BM_INCLUDES})

    # Add static libraries
    add_subdirectory(bcmp)
    add_subdirectory(middleware)
    add_subdirectory(common)
    add_subdirectory(bm_common_messages)
    add_subdirectory(network)
    add_subdirectory(third_party)
    add_subdirectory(drivers/adin2111)

    target_link_libraries(bmcore PUBLIC
        bmcommon
        bcmp
        middleware
        bmmessages
        bmnetwork
        bmthirdparty
    )
endif()
