#ifndef __MOCK_BM_RTOS_H__
#define __MOCK_BM_RTOS_H__

#include "fff.h"
#include "util.h"
#include <stddef.h>
#include <stdint.h>

typedef void *BmSemaphore;
typedef void *BmTimer;
typedef void (*BmTimerCb)(void *);

DECLARE_FAKE_VALUE_FUNC(void *, bm_malloc, size_t);
DECLARE_FAKE_VOID_FUNC(bm_free, void *);
DECLARE_FAKE_VALUE_FUNC(BmSemaphore *, bm_semaphore_create);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_semaphore_give, BmSemaphore);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_semaphore_take, BmSemaphore, uint32_t);
DECLARE_FAKE_VALUE_FUNC(uint32_t, bm_get_tick_count);
DECLARE_FAKE_VALUE_FUNC(uint32_t, bm_ms_to_ticks, uint32_t);
DECLARE_FAKE_VALUE_FUNC(uint32_t, bm_ticks_to_ms, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmTimer, bm_timer_create, BmTimerCb, const char *, uint32_t, void *);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_timer_start, BmTimer, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_timer_stop, BmTimer, uint32_t);

#endif
