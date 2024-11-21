set(SOURCES
    bm_service.c
    bm_service_request.c
    bristlemouth.c
    cbor_service_helper.c
    config_cbor_map_service.c
    echo_service.c
    middleware.c
    pubsub.c
    sys_info_service.c
)

add_library(middleware ${SOURCES})

target_include_directories(middleware PUBLIC
    .
)

target_link_libraries(middleware PRIVATE bmadin2111 bmcommon bcmp bmmessages bmthirdparty)
