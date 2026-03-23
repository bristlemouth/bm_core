add_library(mavlink INTERFACE)

target_include_directories(mavlink INTERFACE
    mavlink
)

# Suppress build warnings for mavlink as advised here:
# https://mavlink.io/en/mavgen_c/#build-warnings
target_compile_options(mavlink INTERFACE -Wno-address-of-packed-member)

set(SOURCES
    tinycbor/src/cborparser.c
    tinycbor/src/cborencoder_float.c
    tinycbor/src/cborencoder.c
    tinycbor/src/cborerrorstrings.c
    tinycbor/src/cborvalidation.c
    tinycbor/src/cborpretty.c
    crc/crc16.c
    crc/crc32.c
)

add_library(bmthirdparty ${SOURCES})

target_include_directories(bmthirdparty PUBLIC
    .
)

target_link_libraries(bmthirdparty mavlink)
