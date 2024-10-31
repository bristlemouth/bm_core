#include "util.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// Something like this for the asserts? Although maybe this shouldn't live here
// #ifdef DEV_MODE
// #define ASSERT(x) configASSERT(x)
// #else
// #define ASSERT(x) ((void)0)
// #endif

#define BM_MAX_DELAY_UINT64 0xFFFFFFFFFFFFFFFF
#define BM_MAX_DELAY_UINT32 0xFFFFFFFF
#define BM_MAX_DELAY_UINT16 0xFFFF
#define BM_MAX_DELAY_UINT8 0xFF

typedef void *BmQueue;
typedef void *BmSemaphore;
typedef void *BmTimer;
typedef void *BmTaskHandle;

// Memory functions - these may not necessarily be tied to the OS but I'm including them here for now
void *bm_malloc(size_t size);
void bm_free(void *ptr);

// Queue functions
BmQueue bm_queue_create(uint32_t queue_length, uint32_t item_size);
void bm_queue_delete(BmQueue queue);
BmErr bm_queue_receive(BmQueue queue, void *item, uint32_t timeout_ms);
BmErr bm_queue_send(BmQueue queue, const void *item, uint32_t timeout_ms);

// Semaphore functions
BmSemaphore bm_semaphore_create(void);
void bm_semaphore_delete(BmSemaphore semaphore);
BmErr bm_semaphore_give(BmSemaphore semaphore);
BmErr bm_semaphore_take(BmSemaphore semaphore, uint32_t timeout_ms);

// Task functions
BmErr bm_task_create(void (*task)(void *), const char *name,
                     uint32_t stack_size, void *arg, uint32_t priority,
                     BmTaskHandle task_handle);
void bm_task_delete(BmTaskHandle task_handle);
void bm_start_scheduler(void);

// Timer functions
BmTimer bm_timer_create(const char *name, uint32_t period_ms, bool auto_reload,
                        void *time_id, void (*callback)(void *));
void bm_timer_delete(BmTimer timer, uint32_t timeout_ms);
BmErr bm_timer_start(BmTimer timer, uint32_t timeout_ms);
BmErr bm_timer_stop(BmTimer timer, uint32_t timeout_ms);
BmErr bm_timer_change_period(BmTimer timer, uint32_t period_ms,
                             uint32_t timeout_ms);
uint32_t bm_get_tick_count(void);
uint32_t bm_get_tick_count_from_isr(void);
uint32_t bm_ms_to_ticks(uint32_t ms);
uint32_t bm_ticks_to_ms(uint32_t ticks);
void bm_delay(uint32_t ms);

// These use time_remaining from util.c
#define time_remaining_ticks(startTicks, timeoutTicks)                         \
  time_remaining(startTicks, bm_get_tick_count(), timeoutTicks)
#define time_remaining_ticks_from_ISR(startTicks, timeoutTicks)                \
  time_remaining(startTicks, bm_get_tick_count_from_isr(), timeoutTicks)
#define time_remaining_ms(startTimeMs, timeoutMs)                              \
  bm_ticks_to_ms(time_remaining(bm_ms_to_ticks(startTimeMs),                   \
                                bm_get_tick_count(),                           \
                                bm_ms_to_ticks(timeoutMs)))
#define time_remaining_ms_from_ISR(startTimeMs, timeoutMs)                     \
  bm_ticks_to_ms(time_remaining(bm_ms_to_ticks(startTimeMs),                   \
                                bm_get_tick_count_from_isr(),                  \
                                bm_ms_to_ticks(timeoutMs)))

#ifdef __cplusplus
}
#endif