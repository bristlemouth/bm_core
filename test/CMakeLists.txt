cmake_minimum_required(VERSION 3.25)

# googletest utilizes C++
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

message(STATUS "Building Unit Tests")

# Setup testing
include(CTest)

# googletest v1.15.0
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.0
)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(MOCK_DIR ${CMAKE_CURRENT_LIST_DIR}/mocks)

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
    .
)

link_libraries(
    gmock
    gtest_main)

# - Create Unit Test
#
# This function creates a unit test with required source files.
# Coverage report information is compiled in for the file to be tested.
#
# test_name - the name to call the test
# srcs sources - necessary to build the application, if no external sources
#                are used, fill this in with the ${DUMMY} variable
# test_src - test file that is to be unit tested

function(create_gtest test_name srcs test_src)
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
        #${TEST_STUBS}
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
    ${SRC_DIR}/ll.c
    ${SRC_DIR}/util.c
)
create_gtest("packet" ${SRCS} ${SRC_DIR}/packet.c)
create_gtest("ll" ${DUMMY} ${SRC_DIR}/ll.c)
