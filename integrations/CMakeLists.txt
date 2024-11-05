set(SOURCES
    spotter.c
)

add_library(bmintegrations ${SOURCES})

target_include_directories(bmintegrations PUBLIC
    .
)

target_link_libraries(bmintegrations PRIVATE bmcommon bcmp middleware)
