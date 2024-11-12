set(SOURCES
    bcmp.c
    config.c
    dfu_client.c
    dfu_core.c
    dfu_host.c
    heartbeat.c
    info.c
    neighbors.c
    packet.c
    ping.c
    resource_discovery.c
    time.c
    configuration.c
)

add_library(bcmp ${SOURCES})

target_include_directories(bcmp PUBLIC
    .
)

target_link_libraries(bcmp PRIVATE bmcommon)
