include_directories(
    .
    ${CMAKE_CURRENT_LIST_DIR}/../../network
    ${bm_common_SOURCE_DIR}/include
)

set(SOURCES
    adi_fcs.c
    adi_mac.c
    adi_phy.c
    adi_spi_oa.c
    adin2111.c
    bm_adin2111.c
)

add_library(bmadin2111 ${SOURCES})

target_link_libraries(bmadin2111 bmcommon)

if(ENABLE_TESTING)
    add_compile_definitions(ENABLE_TESTING)
endif()
