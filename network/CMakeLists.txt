set(SOURCES
    l2.c
    l2_policy.c
)

add_library(bmnetwork ${SOURCES})

target_include_directories(bmnetwork PUBLIC
    .
)

target_link_libraries(bmnetwork PRIVATE bmcommon)
