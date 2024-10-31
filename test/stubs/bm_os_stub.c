#include "mock_bm_os.h"
#include <stdlib.h>

static int16_t FAIL_ATTEMPTS = -1;
static int16_t FAIL_COUNT = 0;

void bm_malloc_fail_on_attempt(uint8_t attempts) { FAIL_ATTEMPTS = attempts; }
void bm_malloc_fail_reset(void) {
  FAIL_COUNT = 0;
  FAIL_ATTEMPTS = -1;
}
void *bm_malloc(size_t size) {
  void *ret = NULL;

  FAIL_COUNT = FAIL_ATTEMPTS != -1 ? FAIL_COUNT + 1 : 0;
  if (FAIL_ATTEMPTS == -1 || FAIL_COUNT < FAIL_ATTEMPTS) {
    ret = malloc(size);
  } else {
    FAIL_COUNT = 0;
    FAIL_ATTEMPTS = -1;
  }

  return ret;
}
void bm_free(void *p) { return free(p); }
DEFINE_FAKE_VALUE_FUNC(BmSemaphore, bm_semaphore_create);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_semaphore_give, BmSemaphore);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_semaphore_take, BmSemaphore, uint32_t);
DEFINE_FAKE_VOID_FUNC(bm_delay, uint32_t);
DEFINE_FAKE_VALUE_FUNC(uint32_t, bm_get_tick_count);
DEFINE_FAKE_VALUE_FUNC(uint32_t, bm_ms_to_ticks, uint32_t);
DEFINE_FAKE_VALUE_FUNC(uint32_t, bm_ticks_to_ms, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmTimer, bm_timer_create, const char *, uint32_t, bool,
                       void *, BmTimerCb);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_timer_start, BmTimer, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_timer_stop, BmTimer, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_timer_change_period, BmTimer, uint32_t,
                       uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmQueue, bm_queue_create, uint32_t, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_queue_receive, BmQueue, void *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_queue_send, BmQueue, const void *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_task_create, BmTaskCb, const char *, uint32_t,
                       void *, uint32_t, void *);
