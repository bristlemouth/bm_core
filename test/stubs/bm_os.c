#include "bm_os.h"
#include <stdlib.h>

DEFINE_FFF_GLOBALS;

void *bm_malloc(size_t size) { return malloc(size); }
void bm_free(void *p) { return free(p); }
DEFINE_FAKE_VALUE_FUNC(BmSemaphore, bm_semaphore_create);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_semaphore_give, BmSemaphore);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_semaphore_take, BmSemaphore, uint32_t);
DEFINE_FAKE_VALUE_FUNC(uint32_t, bm_get_tick_count);
DEFINE_FAKE_VALUE_FUNC(uint32_t, bm_ms_to_ticks, uint32_t);
DEFINE_FAKE_VALUE_FUNC(uint32_t, bm_ticks_to_ms, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmTimer, bm_timer_create, BmTimerCb, const char *, uint32_t, void *);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_timer_start, BmTimer, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_timer_stop, BmTimer, uint32_t);
