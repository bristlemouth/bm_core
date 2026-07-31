set(SOURCES
    bm_mavlink.c
    bm_service.c
    bm_service_request.c
    cbor_service_helper.c
    config_cbor_map_service.c
    echo_service.c
    middleware.c
    power_info_service.c
    pubsub.c
    sys_info_service.c
)

# bristlemouth.c hardcodes the ADIN2111 network device; exclude on hosted builds.
if(NOT BM_HOSTED)
    list(APPEND SOURCES bristlemouth.c)
endif()

add_library(middleware ${SOURCES})

target_include_directories(middleware PUBLIC
    .
)

target_link_libraries(middleware PRIVATE bmcommon bcmp bmmessages bmthirdparty)
if(NOT BM_HOSTED)
    target_link_libraries(middleware PRIVATE bmadin2111)
endif()
