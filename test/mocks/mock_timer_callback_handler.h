#include "fff.h"
#include <stdint.h>
#include <stdbool.h>
#include "util.h"

typedef void (*timer_handler_cb)(void * arg);

DECLARE_FAKE_VALUE_FUNC(bool, timer_callback_handler_send_cb, timer_handler_cb, void*, uint32_t);
