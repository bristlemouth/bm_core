#include "fff.h"
#include "util.h"
#include <stddef.h>
#include <stdint.h>

typedef void *BmQueue;
typedef void *BmSemaphore;
typedef void *BmTimer;
typedef void (*BmTimerCb)(void *);
typedef void (*BmTaskCb)(void *);

void bm_malloc_fail_on_attempt(uint8_t attempts);
void bm_malloc_fail_reset(void);
DECLARE_FAKE_VALUE_FUNC(void *, bm_malloc, size_t);
DECLARE_FAKE_VOID_FUNC(bm_free, void *);
DECLARE_FAKE_VALUE_FUNC(BmSemaphore, bm_semaphore_create);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_semaphore_give, BmSemaphore);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_semaphore_take, BmSemaphore, uint32_t);
DECLARE_FAKE_VOID_FUNC(bm_delay, uint32_t);
DECLARE_FAKE_VALUE_FUNC(uint32_t, bm_get_tick_count);
DECLARE_FAKE_VALUE_FUNC(uint32_t, bm_ms_to_ticks, uint32_t);
DECLARE_FAKE_VALUE_FUNC(uint32_t, bm_ticks_to_ms, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmTimer, bm_timer_create, const char *, uint32_t, bool,
                        void *, BmTimerCb);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_timer_start, BmTimer, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_timer_stop, BmTimer, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_timer_change_period, BmTimer, uint32_t,
                        uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmQueue, bm_queue_create, uint32_t, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_queue_receive, BmQueue, void *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_queue_send, BmQueue, const void *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_task_create, BmTaskCb, const char *, uint32_t,
                        void *, uint32_t, void *);
