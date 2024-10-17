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
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_LIST_DIR}/../third_party)
set(MOCK_DIR ${CMAKE_CURRENT_LIST_DIR}/mocks)
set(STUB_DIR ${CMAKE_CURRENT_LIST_DIR}/stubs)
set(BM_COMMON_MOCK_DIR ${bm_common_SOURCE_DIR}/test/mocks)
set(BM_COMMON_TEST_DIR ${bm_common_SOURCE_DIR}/test)
set(BM_COMMON_SOURCE_DIR ${bm_common_SOURCE_DIR}/src)
set(BM_COMMON_STUBS_DIR ${bm_common_SOURCE_DIR}/test/stubs)
set(BM_COMMON_THIRD_PARTY_DIR ${bm_common_SOURCE_DIR}/third_party)

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
# srcs - list of sources necessary to build the test including the file to be tested

function(create_gtest test_name  srcs)
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

# PACKET TESTS
set (PACKET_SRCS
    # File we're testing
    ${SRC_DIR}/packet.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
)
create_gtest("packet" "${PACKET_SRCS}")

# BCMP TESTS
set (BCMP_SRCS
    # File we're testing
    ${SRC_DIR}/bcmp.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/time_stub.c
    ${STUB_DIR}/dfu_stub.c
    ${STUB_DIR}/config_stub.c
    ${STUB_DIR}/heartbeat_stub.c
    ${STUB_DIR}/info_stub.c
    ${STUB_DIR}/resource_discovery_stub.c
    ${STUB_DIR}/ping_stub.c
    ${STUB_DIR}/neighbors_stub.c
    ${STUB_DIR}/topology_stub.c
    ${STUB_DIR}/bm_ip_stub.c
)
create_gtest("bcmp" "${BCMP_SRCS}")

# HEARTBEAT TESTS
set (HEARTBEAT_SRCS
    # File we're testing
    ${SRC_DIR}/heartbeat.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/neighbors_stub.c
    ${STUB_DIR}/info_stub.c
    ${STUB_DIR}/bcmp_stub.c
)
create_gtest("heartbeat" "${HEARTBEAT_SRCS}")

# INFO TESTS
set (INFO_SRCS
    # File we're testing
    ${SRC_DIR}/info.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/device_stub.c
    ${STUB_DIR}/neighbors_stub.c
    ${STUB_DIR}/bcmp_stub.c
)
create_gtest("info" "${INFO_SRCS}")

# RESOURCE DISCOVERY TESTS
set (RESOURCE_SRCS
    # File we're testing
    ${SRC_DIR}/resource_discovery.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/device_stub.c
    ${STUB_DIR}/bcmp_stub.c
)
create_gtest("resource_discovery" "${RESOURCE_SRCS}")

# PING TESTS
set (PING_SRCS
    # File we're testing
    ${SRC_DIR}/ping.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/device_stub.c
    ${STUB_DIR}/bcmp_stub.c
)
create_gtest("ping" "${PING_SRCS}")

# NEIGHBORS TESTS
set (NEIGHBORS_SRCS
    # File we're testing
    ${SRC_DIR}/neighbors.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/info_stub.c
)
create_gtest("neighbors" "${NEIGHBORS_SRCS}")


# DFU TESTS

set (DFU_SRCS
    # Files we're testing
    ${SRC_DIR}/dfu/dfu_core.c
    ${SRC_DIR}/dfu/dfu_client.c
    ${SRC_DIR}/dfu/dfu_host.c

    # Support files
    ${BM_COMMON_THIRD_PARTY_DIR}/crc/crc16.c
    ${BM_COMMON_THIRD_PARTY_DIR}/crc/crc32.c
    ${BM_COMMON_SOURCE_DIR}/lib_state_machine.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborparser.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborencoder_float.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborencoder.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborerrorstrings.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborvalidation.c

    # Stubs
    # TODO - probably need to mock the generic/wrapper functions!
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/device_stub.c
    # ${TEST_DIR}/stubs/mock_timer_callback_handler.cpp # TODO - mock this
    ${BM_COMMON_STUBS_DIR}/bm_os.c
)

create_gtest("dfu" "${DFU_SRCS}")
