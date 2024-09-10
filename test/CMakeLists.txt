cmake_minimum_required(VERSION 3.25)

# googletest utilizes C++
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

message(STATUS "Building Unit Tests")

# Setup testing
include(CTest)
include(FetchContent)

# googletest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.0
)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# fff
FetchContent_Declare(
  fff
  GIT_REPOSITORY https://github.com/meekrosoft/fff.git
  GIT_TAG        5111c61
)
FetchContent_MakeAvailable(fff)

set(FETCH_DIR ${CMAKE_BINARY_DIR}/_deps/)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(MOCK_DIR ${CMAKE_CURRENT_LIST_DIR}/mocks)
set(FFF_DIR ${FETCH_DIR}fff-src)

# Unit test general compile option
add_compile_options(
    -Werror
    -g
    -ggdb
    -Og
    -DCI_TEST
    -DGTEST_BREAK_ON_FAILURE=1
)

# Link options for coverage reports
add_link_options(
    -fprofile-arcs
    -ftest-coverage
)

include_directories(
    ${MOCK_DIR}
    ${FFF_DIR}
    .
)

link_libraries(
    gmock
    gtest_main)

file(GLOB TEST_STUBS stubs/*.c)

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
        src/${test_name}_ut.cpp
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

# Add unit tests below here
set (SRCS
    ${SRC_DIR}/util.c
)
create_gtest("packet" "${SRC_DIR}/packet.c" "${SRCS}")
