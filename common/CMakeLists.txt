set(SOURCES
    aligned_malloc.c
    cb_queue.c
    device.c
    lib_state_machine.c
    ll.c
    timer_callback_handler.c
    util.c
)

add_library(bmcommon ${SOURCES})

target_include_directories(bmcommon PUBLIC
    .
)
