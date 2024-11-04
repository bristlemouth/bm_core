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
set(NETWORK_DIR ${CMAKE_CURRENT_LIST_DIR}/../network)
set(BCMP_DIR ${CMAKE_CURRENT_LIST_DIR}/../bcmp)
set(MIDDLEWARE_DIR ${CMAKE_CURRENT_LIST_DIR}/../middleware)
set(COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/../common)
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_LIST_DIR}/../third_party)
set(MOCK_DIR ${CMAKE_CURRENT_LIST_DIR}/mocks)
set(STUB_DIR ${CMAKE_CURRENT_LIST_DIR}/stubs)
set(ADIN2111_DIR ${CMAKE_CURRENT_LIST_DIR}/../drivers/adin2111)

include_directories(
    ${MOCK_DIR}
    ${fff_SOURCE_DIR}
    .
)

link_libraries(
    gmock
    gtest_main
)

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
    )
    # Generate artifacts for coverage report using lcov
    file(GLOB_RECURSE FILES "../*")
    foreach(FILE ${FILES})
        if (NOT FILE MATCHES "^test/")
            get_filename_component(FILENAME "${FILE}" NAME)
            if (FILENAME MATCHES ".*${test_name}.*")
                set_source_files_properties(${FILE} PROPERTIES
                    COMPILE_FLAGS
                    "-fprofile-arcs -ftest-coverage"
                )
            endif()
        endif()
    endforeach()
    gtest_discover_tests(${GTEST_NAME})
endfunction()

# Linked List (LL) unit tests
set (LL_TEST_SRCS
    # File we are testing
    ${COMMON_DIR}/ll.c

    # Supporting files
    ${STUB_DIR}/bm_os_stub.c
)
create_gtest("ll" "${LL_TEST_SRCS}")

# State Machine unit tests
set (LIB_STATE_MACHINE_TEST_SRCS
    # File we are testing
    ${COMMON_DIR}/lib_state_machine.c
)
create_gtest("lib_state_machine" "${LIB_STATE_MACHINE_TEST_SRCS}")

# Aligned Malloc unit tests
set (ALIGNED_MALLOC_TEST_SRCS
    # File we are testing
    ${COMMON_DIR}/aligned_malloc.c

    # Supporting files
    ${STUB_DIR}/bm_os_stub.c
)
create_gtest("aligned_malloc" "${ALIGNED_MALLOC_TEST_SRCS}")

# PACKET TESTS
set (PACKET_SRCS
    # File we're testing
    ${BCMP_DIR}/packet.c

    # Supporting Files
    ${COMMON_DIR}/util.c
    ${COMMON_DIR}/ll.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
)
create_gtest("packet" "${PACKET_SRCS}")

# BCMP TESTS
set (BCMP_SRCS
    # File we're testing
    ${BCMP_DIR}/bcmp.c

    # Supporting Files
    ${COMMON_DIR}/util.c
    ${COMMON_DIR}/ll.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
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
    ${COMMON_DIR}/util.c
    ${COMMON_DIR}/ll.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
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
    ${COMMON_DIR}/util.c
    ${COMMON_DIR}/ll.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
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
    ${COMMON_DIR}/util.c
    ${COMMON_DIR}/ll.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
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
    ${COMMON_DIR}/util.c
    ${COMMON_DIR}/ll.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
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
    ${COMMON_DIR}/util.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
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
    ${COMMON_DIR}/util.c
    ${THIRD_PARTY_DIR}/crc/crc16.c
    ${THIRD_PARTY_DIR}/crc/crc32.c
    ${COMMON_DIR}/lib_state_machine.c
    ${COMMON_DIR}/ll.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
    ${STUB_DIR}/timer_callback_handler_stub.c
    ${STUB_DIR}/device_stub.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/bcmp_stub.c
    ${STUB_DIR}/dfu_generic_stub.c
)
create_gtest("dfu" "${DFU_SRCS}")


# L2 Tests
set (L2_SRCS
    # File we're testing
    ${NETWORK_DIR}/l2.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
    ${STUB_DIR}/bm_ip_stub.c
)
create_gtest("l2" "${L2_SRCS}")

# TOPOLOGY TESTS
set (TOPOLOGY_SRCS
    # File we're testing
    ${BCMP_DIR}/topology.c

    # Supporting Files
    ${COMMON_DIR}/util.c
    ${COMMON_DIR}/ll.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
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
    ${MIDDLEWARE_DIR}/cbor_service_helper.c

    # Supporting Files
    ${COMMON_DIR}/util.c
    ${THIRD_PARTY_DIR}/crc/crc32.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborparser.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborencoder_float.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborencoder.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborerrorstrings.c
    ${THIRD_PARTY_DIR}/tinycbor/src/cborvalidation.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
)
create_gtest("cbor_service_helper" "${CBOR_SEVICE_HELPER_SRCS}")

# ECHO SERVICE TESTS
set (ECHO_SEVICE_SRCS
    # File we're testing
    ${MIDDLEWARE_DIR}/echo_service.c

    # Supporting Files
    ${COMMON_DIR}/util.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
    ${STUB_DIR}/bm_service_stub.c
    ${STUB_DIR}/device_stub.c
)
create_gtest("echo_service" "${ECHO_SEVICE_SRCS}")

# CONFIG CBOR MAP TESTS
set (CONFIG_CBOR_MAP_SRCS
    # File we're testing
    ${MIDDLEWARE_DIR}/config_cbor_map_service.c

    # Supporting Files
    ${COMMON_DIR}/util.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
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
    ${MIDDLEWARE_DIR}/sys_info_service.c

    # Supporting Files
    ${COMMON_DIR}/util.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
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
    ${MIDDLEWARE_DIR}/bm_service.c

    # Supporting Files
    ${COMMON_DIR}/util.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
    ${STUB_DIR}/bm_service_request_stub.c
    ${STUB_DIR}/pubsub_stub.c
)
create_gtest("bm_service" "${BM_SERVICE_SRCS}")

# BM SERVICE REQUEST TESTS
set (BM_SERVICE_REQUEST_SRCS
    # File we're testing
    ${MIDDLEWARE_DIR}/bm_service_request.c

    # Supporting Files
    ${COMMON_DIR}/util.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
    ${STUB_DIR}/timer_callback_handler_stub.c
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
    ${COMMON_DIR}/aligned_malloc.c)
target_include_directories(bm_adin2111_test PRIVATE
    ${ADIN2111_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../network)
target_link_libraries(bm_adin2111_test GTest::gtest_main)
gtest_discover_tests(bm_adin2111_test)

# Time remaining tests
set (TIME_REMAINING_SRCS
    # File we're testing
    ${COMMON_DIR}/util.c
)
create_gtest("time_remaining" "${TIME_REMAINING_SRCS}")

# UTC From Date Time tests
set (UTC_FROM_DATE_TIME_SRCS
    # File we're testing
    ${COMMON_DIR}/util.c
)
create_gtest("utc_from_date_time" "${UTC_FROM_DATE_TIME_SRCS}")

# Time Messsage tests
set (TIME_MESSAGE_SRCS
    # File we're testing
    ${BCMP_DIR}/time.c

    # Supporting Files
    ${COMMON_DIR}/util.c
    ${COMMON_DIR}/ll.c

    # Stubs
    ${STUB_DIR}/bm_os_stub.c
    ${STUB_DIR}/packet_stub.c
    ${STUB_DIR}/device_stub.c
    ${STUB_DIR}/bcmp_stub.c
    ${STUB_DIR}/bm_rtc_stub.c
)
create_gtest("time" "${TIME_MESSAGE_SRCS}")
