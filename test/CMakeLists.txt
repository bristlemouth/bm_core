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
set(BCMP_DIR ${CMAKE_CURRENT_LIST_DIR}/../bcmp)
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_LIST_DIR}/../third_party)
set(MOCK_DIR ${CMAKE_CURRENT_LIST_DIR}/mocks)
set(STUB_DIR ${CMAKE_CURRENT_LIST_DIR}/stubs)
set(BM_COMMON_MOCK_DIR ${bm_common_SOURCE_DIR}/test/mocks)
set(BM_COMMON_TEST_DIR ${bm_common_SOURCE_DIR}/test)
set(BM_COMMON_SOURCE_DIR ${bm_common_SOURCE_DIR}/src)
set(BM_COMMON_STUBS_DIR ${bm_common_SOURCE_DIR}/test/stubs)
set(BM_COMMON_THIRD_PARTY_DIR ${bm_common_SOURCE_DIR}/third_party)
set(ADIN2111_DIR ${CMAKE_CURRENT_LIST_DIR}/../drivers/adin2111)

include_directories(
    ${MOCK_DIR}
    ${fff_SOURCE_DIR}
    ${BM_COMMON_MOCK_DIR}
    ${BM_COMMON_TEST_DIR}
    ${BCMP_DIR}
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
    file(GLOB_RECURSE FILES "../src/*")
    foreach(FILE ${FILES})
        get_filename_component(FILENAME "${FILE}" NAME)
        message(WARNING "${FILENAME}")
        if (FILENAME MATCHES ".*${test_name}.*")
            set_source_files_properties(${FILE} PROPERTIES
                COMPILE_FLAGS
                "-fprofile-arcs -ftest-coverage"
            )
        endif()
    endforeach()
endfunction()

# PACKET TESTS
set (PACKET_SRCS
    # File we're testing
    ${BCMP_DIR}/packet.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
)
create_gtest("packet" "${PACKET_SRCS}")

# BCMP TESTS
set (BCMP_SRCS
    # File we're testing
    ${BCMP_DIR}/bcmp.c

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
    ${BCMP_DIR}/heartbeat.c

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
    ${BCMP_DIR}/info.c

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
    ${BCMP_DIR}/resource_discovery.c

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
    ${BCMP_DIR}/ping.c

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
    ${BCMP_DIR}/neighbors.c

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
    ${BCMP_DIR}/dfu_core.c
    ${BCMP_DIR}/dfu_client.c
    ${BCMP_DIR}/dfu_host.c

    # Support files
    ${BM_COMMON_SOURCE_DIR}/util.c
    ${BM_COMMON_THIRD_PARTY_DIR}/crc/crc16.c
    ${BM_COMMON_THIRD_PARTY_DIR}/crc/crc32.c
    ${BM_COMMON_SOURCE_DIR}/lib_state_machine.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${BM_COMMON_STUBS_DIR}/timer_callback_handler_stub.c
    ${STUB_DIR}/device_stub.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/bcmp_stub.c
    ${STUB_DIR}/dfu_generic_stub.c
)
create_gtest("dfu" "${DFU_SRCS}")


# L2 Tests
set (L2_SRCS
    # File we're testing
    ${SRC_DIR}/l2.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/bm_ip_stub.c
)
create_gtest("l2" "${L2_SRCS}")

# TOPOLOGY TESTS
set (TOPOLOGY_SRCS
    # File we're testing
    ${BCMP_DIR}/topology.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/bcmp_stub.c
    ${STUB_DIR}/device_stub.c
    ${STUB_DIR}/l2_stub.c
    ${STUB_DIR}/neighbors_stub.c
)
create_gtest("topology" "${TOPOLOGY_SRCS}")

# CBOR SERVICE HELPER TESTS
set (CBOR_SEVICE_HELPER_SRCS
    # File we're testing
    ${SRC_DIR}/cbor_service_helper.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c
    ${BM_COMMON_THIRD_PARTY_DIR}/crc/crc32.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborparser.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborencoder_float.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborencoder.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborerrorstrings.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborvalidation.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
)
create_gtest("cbor_service_helper" "${CBOR_SEVICE_HELPER_SRCS}")

# ECHO SERVICE TESTS
set (ECHO_SEVICE_SRCS
    # File we're testing
    ${SRC_DIR}/echo_service.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/bm_service_stub.c
    ${STUB_DIR}/device_stub.c
)
create_gtest("echo_service" "${ECHO_SEVICE_SRCS}")

# CONFIG CBOR MAP TESTS
set (CONFIG_CBOR_MAP_SRCS
    # File we're testing
    ${SRC_DIR}/config_cbor_map_service.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/bm_service_stub.c
    ${STUB_DIR}/bm_service_request_stub.c
    ${STUB_DIR}/config_cbor_map_srv_reply_msg_stub.c
    ${STUB_DIR}/config_cbor_map_srv_request_msg_stub.c
    ${STUB_DIR}/cbor_service_helper_stub.c
    ${STUB_DIR}/device_stub.c
)
create_gtest("config_cbor_map_service" "${CONFIG_CBOR_MAP_SRCS}")

# SYS INFO SERVICE TESTS
set (SYS_INFO_SERVICE_SRCS
    # File we're testing
    ${SRC_DIR}/sys_info_service.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/bm_service_stub.c
    ${STUB_DIR}/bm_service_request_stub.c
    ${STUB_DIR}/sys_info_svc_reply_msg_stub.c
    ${STUB_DIR}/cbor_service_helper_stub.c
    ${STUB_DIR}/device_stub.c
)
create_gtest("sys_info_service" "${SYS_INFO_SERVICE_SRCS}")

# BM SERVICE TESTS
set (BM_SERVICE_SRCS
    # File we're testing
    ${SRC_DIR}/bm_service.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${STUB_DIR}/bm_service_request_stub.c
    ${STUB_DIR}/pubsub_stub.c
)
create_gtest("bm_service" "${BM_SERVICE_SRCS}")

# BM SERVICE REQUEST TESTS
set (BM_SERVICE_REQUEST_SRCS
    # File we're testing
    ${SRC_DIR}/bm_service_request.c

    # Supporting Files
    ${BM_COMMON_SOURCE_DIR}/util.c

    # Stubs
    ${BM_COMMON_STUBS_DIR}/bm_os.c
    ${BM_COMMON_STUBS_DIR}/timer_callback_handler_stub.c
    ${STUB_DIR}/pubsub_stub.c
    ${STUB_DIR}/device_stub.c
)
create_gtest("bm_service_request" "${BM_SERVICE_REQUEST_SRCS}")

# ADIN2111 DRIVER TESTS
add_executable(bm_adin2111_test
    src/bm_adin2111_test.cpp
    ${ADIN2111_DIR}/bm_adin2111.c
    ${ADIN2111_DIR}/adin2111.c
    ${ADIN2111_DIR}/adi_fcs.c
    ${ADIN2111_DIR}/adi_mac.c
    ${ADIN2111_DIR}/adi_phy.c
    ${ADIN2111_DIR}/adi_spi_oa.c
    ${bm_common_SOURCE_DIR}/src/aligned_malloc.c)
target_include_directories(bm_adin2111_test PRIVATE
    ${ADIN2111_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../network)
target_link_libraries(bm_adin2111_test GTest::gtest_main)
gtest_discover_tests(bm_adin2111_test)
