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
