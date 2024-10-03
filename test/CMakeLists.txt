cmake_minimum_required(VERSION 3.29)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

include(CTest)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
FetchContent_Declare(
  fff
  GIT_REPOSITORY https://github.com/meekrosoft/fff.git
  GIT_TAG        5111c61
)
FetchContent_MakeAvailable(fff)
include(GoogleTest)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/../src)
set(MOCK_DIR ${CMAKE_CURRENT_LIST_DIR}/mocks)
set(BM_COMMON_MOCK_DIR ${bm_common_SOURCE_DIR}/test/mocks)
set(BM_COMMON_TEST_DIR ${bm_common_SOURCE_DIR}/test)
set(BM_COMMON_SOURCE_DIR ${bm_common_SOURCE_DIR}/src)

include_directories(
    ${MOCK_DIR}
    ${fff_SOURCE_DIR}
    ${BM_COMMON_MOCK_DIR}
    ${BM_COMMON_TEST_DIR}
    .
)

link_libraries(
    gmock
    gtest_main
)

file(GLOB TEST_STUBS_BM_COMMON ${bm_common_SOURCE_DIR}/test/stubs/*.c)

# - Create Unit Test
#
# This function creates a unit test with required source files.
# Coverage report information is compiled in for the file to be tested.
#
# test_name - the name to call the test
# test_src - test file that is to be unit tested
# srcs sources - necessary to build the application, if no external sources
#                are used, fill this in with the ${DUMMY} variable

function(create_gtest test_name test_src srcs)
    set(GTEST_NAME ${test_name}_test)
    file(GLOB TEST_STUBS stubs/*.c)
    list(FILTER TEST_STUBS EXCLUDE REGEX "${test_name}")
    add_executable(${GTEST_NAME})
    target_include_directories(${GTEST_NAME}
        PRIVATE
        ${includes})
    target_sources(${GTEST_NAME}
        PRIVATE
        ${srcs}
        ${test_src}
        # Stubs
        "${TEST_STUBS}"
        "${TEST_STUBS_BM_COMMON}"
        src/${test_name}_test.cpp
        ${BM_COMMON_SOURCE_DIR}/ll.c
    )
    gtest_discover_tests(${GTEST_NAME})
    # Generate artifacts for coverage report using lcov
    set_source_files_properties(${test_src} PROPERTIES
        COMPILE_FLAGS
        "-fprofile-arcs -ftest-coverage"
    )
endfunction()

# Dummy variable for unit tests that require no source files
set(DUMMY src)

set (SRCS
    ${BM_COMMON_SOURCE_DIR}/util.c
)
create_gtest("packet" "${SRC_DIR}/packet.c" "${SRCS}")
create_gtest("bcmp" "${SRC_DIR}/bcmp.c" "${SRCS}")
create_gtest("heartbeat" "${SRC_DIR}/heartbeat.c" "${SRCS}")
create_gtest("info" "${SRC_DIR}/info.c" "${SRCS}")
create_gtest("resource_discovery" "${SRC_DIR}/resource_discovery.c" "${SRCS}")
