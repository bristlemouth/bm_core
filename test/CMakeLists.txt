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
set(STUB_DIR ${CMAKE_CURRENT_LIST_DIR}/stubs)
set(BM_COMMON_MOCK_DIR ${bm_common_SOURCE_DIR}/test/mocks)
set(BM_COMMON_TEST_DIR ${bm_common_SOURCE_DIR}/test)
set(BM_COMMON_SOURCE_DIR ${bm_common_SOURCE_DIR}/src)
set(BM_COMMON_STUBS_DIR ${bm_common_SOURCE_DIR}/test/stubs)

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
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
)
create_gtest("packet" "${SRC_DIR}/packet.c" "${PACKET_SRCS}")

# BCMP TESTS
set (BCMP_SRCS
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/time_stub.c
    ${STUB_DIR}/config_stub.c
    ${STUB_DIR}/heartbeat_stub.c
    ${STUB_DIR}/info_stub.c
    ${STUB_DIR}/resource_discovery_stub.c
    ${STUB_DIR}/ping_stub.c
    ${STUB_DIR}/neighbors_stub.c
    ${STUB_DIR}/topology_stub.c
    ${STUB_DIR}/bm_ip_stub.c
)
create_gtest("bcmp" "${SRC_DIR}/bcmp.c" "${BCMP_SRCS}")

# HEARTBEAT TESTS
set (HEARTBEAT_SRCS
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/neighbors_stub.c
    ${STUB_DIR}/info_stub.c
    ${STUB_DIR}/bcmp_stub.c
)
create_gtest("heartbeat" "${SRC_DIR}/heartbeat.c" "${HEARTBEAT_SRCS}")

# INFO TESTS
set (INFO_SRCS
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/device_stub.c
    ${STUB_DIR}/neighbors_stub.c
    ${STUB_DIR}/bcmp_stub.c
)
create_gtest("info" "${SRC_DIR}/info.c" "${INFO_SRCS}")

# RESOURCE DISCOVERY TESTS
set (RESOURCE_SRCS
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/device_stub.c
    ${STUB_DIR}/bcmp_stub.c
)
create_gtest("resource_discovery" "${SRC_DIR}/resource_discovery.c" "${RESOURCE_SRCS}")

# PING TESTS
set (PING_SRCS
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/device_stub.c
    ${STUB_DIR}/bcmp_stub.c
)
create_gtest("ping" "${SRC_DIR}/ping.c" "${PING_SRCS}")

# NEIGHBORS TESTS
set (NEIGHBORS_SRCS
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/info_stub.c
)
create_gtest("neighbors" "${SRC_DIR}/neighbors.c" "${NEIGHBORS_SRCS}")

# create_gtest("dfu" "${SRC_DIR}/dfu.c" "${SRCS}")

#
# BCMP DFU
#

# add_executable(bcmp_dfu_tests)
# target_include_directories(bcmp_dfu_tests
#     PRIVATE
#     ${SRC_DIR}/lib/common
#     ${TEST_DIR}/header_overrides
#     ${TEST_DIR}/mocks
#     ${SRC_DIR}/third_party/FreeRTOS/Source/include
#     ${SRC_DIR}/lib/sys
#     ${SRC_DIR}/third_party/crc
#     ${SRC_DIR}/third_party/tinycbor/src
#     ${TEST_DIR}/third_party/fff
#     ${SRC_DIR}/lib/sys
#     ${SRC_DIR}/lib/bcmp/dfu
#     ${SRC_DIR}/lib/bcmp
#     ${SRC_DIR}/third_party/mcuboot/boot/bootutil/include
#     ${SRC_DIR}/lib/mcuboot/include
#     ${SRC_DIR}/lib/drivers
#     ${SRC_DIR}/lib/drivers/abstract
#     ${SRC_DIR}/apps/bringup
#     ${SRC_DIR}/lib/mcuboot/include/flash_map_backend
# )

# target_sources(bcmp_dfu_tests
#     PRIVATE
#     # File we're testing
#     ${SRC_DIR}/lib/bcmp/dfu/bm_dfu_core.cpp # TODO - get this from where ever it is in bm_core
#     ${SRC_DIR}/lib/bcmp/dfu/bm_dfu_client.cpp # TODO - get this from where ever it is in bm_core
#     ${SRC_DIR}/lib/bcmp/dfu/bm_dfu_host.cpp # TODO - get this from where ever it is in bm_core

#     # Support files
#     ${SRC_DIR}/third_party/crc/crc16.c # TODO - get this from where ever it is in bm_core
#     ${SRC_DIR}/third_party/crc/crc32.c # TODO - get this from where ever it is in bm_core
#     ${SRC_DIR}/lib/common/lib_state_machine.cpp # TODO - get this from where ever it is in bm_common
#     # ${SRC_DIR}/lib/common/nvmPartition.cpp
#     # ${SRC_DIR}/lib/sys/configuration.cpp
#     ${SRC_DIR}/third_party/tinycbor/src/cborparser.c # TODO - get this from where ever it is in bm_core
#     ${SRC_DIR}/third_party/tinycbor/src/cborencoder_float.c # TODO - get this from where ever it is in bm_core
#     ${SRC_DIR}/third_party/tinycbor/src/cborencoder.c # TODO - get this from where ever it is in bm_core
#     ${SRC_DIR}/third_party/tinycbor/src/cborerrorstrings.c # TODO - get this from where ever it is in bm_core
#     ${SRC_DIR}/third_party/tinycbor/src/cborvalidation.c # TODO - get this from where ever it is in bm_core

#     # Mocks
#     # ${TEST_DIR}/stubs/mock_mcu_boot.cpp
#     # ${TEST_DIR}/stubs/mock_FreeRTOS.c
#     # ${TEST_DIR}/stubs/mock_reset_reason.cpp
#     # ${TEST_DIR}/stubs/mock_device_info.cpp
#     # ${TEST_DIR}/stubs/mock_timer_callback_handler.cpp
#     # ${TEST_DIR}/stubs/mock_lpm.cpp
#     # Stubs
#     ${BM_COMMON_STUBS_DIR}/bm_os.c

#     # Unit test wrapper for test
#     bcmp_dfu_ut.cpp
# )

