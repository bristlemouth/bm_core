#include "fff.h"
#include "util.h"
#include <stddef.h>
#include <stdint.h>

typedef void *BmQueue;
typedef void *BmSemaphore;
typedef void *BmTimer;
typedef void *BmTaskHandle;
typedef void *BmBuffer;
typedef void (*BmTimerCb)(void *);
typedef void (*BmTaskCb)(void *);

DECLARE_FAKE_VALUE_FUNC(void *, bm_malloc, size_t);
DECLARE_FAKE_VOID_FUNC(bm_free, void *);
DECLARE_FAKE_VALUE_FUNC(BmSemaphore, bm_semaphore_create);
DECLARE_FAKE_VOID_FUNC(bm_semaphore_delete, BmSemaphore);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_semaphore_give, BmSemaphore);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_semaphore_take, BmSemaphore, uint32_t);
DECLARE_FAKE_VOID_FUNC(bm_delay, uint32_t);
DECLARE_FAKE_VALUE_FUNC(uint32_t, bm_get_tick_count);
DECLARE_FAKE_VALUE_FUNC(uint32_t, bm_ms_to_ticks, uint32_t);
DECLARE_FAKE_VALUE_FUNC(uint32_t, bm_ticks_to_ms, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmTimer, bm_timer_create, const char *, uint32_t, bool,
                        void *, BmTimerCb);
DECLARE_FAKE_VOID_FUNC(bm_timer_delete, BmTimer, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_timer_start, BmTimer, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_timer_stop, BmTimer, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_timer_change_period, BmTimer, uint32_t,
                        uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmQueue, bm_queue_create, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC(bm_queue_delete, BmQueue);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_queue_receive, BmQueue, void *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_queue_send, BmQueue, const void *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_queue_send_to_front_from_isr, BmQueue,
                        const void *);
DECLARE_FAKE_VALUE_FUNC(BmBuffer, bm_stream_buffer_create, uint32_t);
DECLARE_FAKE_VOID_FUNC(bm_stream_buffer_delete, BmBuffer);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_stream_buffer_send, BmBuffer, uint8_t *,
                        uint32_t, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_stream_buffer_receive, BmBuffer, uint8_t *,
                        uint32_t *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_task_create, BmTaskCb, const char *, uint32_t,
                        void *, uint32_t, BmTaskHandle);
DECLARE_FAKE_VOID_FUNC(bm_task_delete, BmTaskHandle);
