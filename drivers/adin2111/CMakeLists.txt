cmake_minimum_required(VERSION 3.29)

if(NOT ENABLE_TESTING)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../../cmake/arm-gcc-toolchain.cmake)
endif()

project(BristlemouthAdin2111Driver VERSION 0.12 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_C_EXTENSIONS NO)

include(FetchContent)
FetchContent_Declare(
  bm_common
  GIT_REPOSITORY https://github.com/bristlemouth/bm_common.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(bm_common)

include_directories(
    .
    ${bm_common_SOURCE_DIR}/include
)

set(SOURCES
    adi_fcs.c
    adi_mac.c
    adi_phy.c
    adi_spi_oa.c
    adin2111.c
    bm_adin2111.c
)

add_library(bmnetif ${SOURCES})

if(ENABLE_TESTING)
    add_compile_definitions(ENABLE_TESTING)
endif()
