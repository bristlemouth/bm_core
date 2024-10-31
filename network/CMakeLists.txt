set(SOURCES
    l2.c
)

add_library(bmnetwork ${SOURCES})

target_include_directories(bmnetwork PUBLIC
    .
)

target_link_libraries(bmnetwork PRIVATE bmcommon)
