/// @file bm_posix.c
/// @brief POSIX implementation of bm_os.h APIs for bm_sbc.

#include "bm_os.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Memory
// ---------------------------------------------------------------------------
void *bm_malloc(size_t size) { return malloc(size); }
void bm_free(void *ptr) { free(ptr); }

// ---------------------------------------------------------------------------
// Queues  (TODO: real implementation)
// ---------------------------------------------------------------------------
BmQueue bm_queue_create(uint32_t queue_length, uint32_t item_size) {
  (void)queue_length;
  (void)item_size;
  return NULL;
}
void bm_queue_delete(BmQueue queue) { (void)queue; }
BmErr bm_queue_receive(BmQueue queue, void *item, uint32_t timeout_ms) {
  (void)queue; (void)item; (void)timeout_ms;
  return BmEINVAL;
}
BmErr bm_queue_send(BmQueue queue, const void *item, uint32_t timeout_ms) {
  (void)queue; (void)item; (void)timeout_ms;
  return BmEINVAL;
}
BmErr bm_queue_send_to_front_from_isr(BmQueue queue, const void *item) {
  (void)queue; (void)item;
  return BmEINVAL;
}

// ---------------------------------------------------------------------------
// Stream buffers  (TODO: real implementation)
// ---------------------------------------------------------------------------
BmBuffer bm_stream_buffer_create(uint32_t max_size) { (void)max_size; return NULL; }
void bm_stream_buffer_delete(BmBuffer buf) { (void)buf; }
BmErr bm_stream_buffer_send(BmBuffer buf, uint8_t *data, uint32_t size,
                            uint32_t timeout_ms) {
  (void)buf; (void)data; (void)size; (void)timeout_ms;
  return BmEINVAL;
}
BmErr bm_stream_buffer_receive(BmBuffer buf, uint8_t *data, uint32_t *size,
                               uint32_t timeout_ms) {
  (void)buf; (void)data; (void)size; (void)timeout_ms;
  return BmEINVAL;
}

// ---------------------------------------------------------------------------
// Mutex / Semaphore  (TODO: real implementation)
// ---------------------------------------------------------------------------
BmSemaphore bm_mutex_create(void) { return NULL; }
BmSemaphore bm_semaphore_create(void) { return NULL; }
void bm_semaphore_delete(BmSemaphore semaphore) { (void)semaphore; }
BmErr bm_semaphore_give(BmSemaphore semaphore) { (void)semaphore; return BmEINVAL; }
BmErr bm_semaphore_take(BmSemaphore semaphore, uint32_t timeout_ms) {
  (void)semaphore; (void)timeout_ms;
  return BmEINVAL;
}

// ---------------------------------------------------------------------------
// Tasks  (TODO: real implementation using pthreads)
// ---------------------------------------------------------------------------
BmErr bm_task_create(BmTask task, const char *name, uint32_t stack_size,
                     void *arg, uint32_t priority, BmTaskHandle task_handle) {
  (void)task; (void)name; (void)stack_size;
  (void)arg; (void)priority; (void)task_handle;
  return BmEINVAL;
}
void bm_task_delete(BmTaskHandle task_handle) { (void)task_handle; }
void bm_start_scheduler(void) {}

// ---------------------------------------------------------------------------
// Timers  (TODO: real implementation)
// ---------------------------------------------------------------------------
BmTimer bm_timer_create(const char *name, uint32_t period_ms, bool auto_reload,
                        void *timer_id, BmTimerCallback cb) {
  (void)name; (void)period_ms; (void)auto_reload;
  (void)timer_id; (void)cb;
  return NULL;
}
void bm_timer_delete(BmTimer timer, uint32_t timeout_ms) {
  (void)timer; (void)timeout_ms;
}
BmErr bm_timer_reset(BmTimer timer, uint32_t timeout_ms) {
  (void)timer; (void)timeout_ms; return BmEINVAL;
}
BmErr bm_timer_start(BmTimer timer, uint32_t timeout_ms) {
  (void)timer; (void)timeout_ms; return BmEINVAL;
}
BmErr bm_timer_stop(BmTimer timer, uint32_t timeout_ms) {
  (void)timer; (void)timeout_ms; return BmEINVAL;
}
BmErr bm_timer_change_period(BmTimer timer, uint32_t period_ms,
                             uint32_t timeout_ms) {
  (void)timer; (void)period_ms; (void)timeout_ms;
  return BmEINVAL;
}
uint32_t bm_timer_get_id(BmTimer timer) { (void)timer; return 0; }

// ---------------------------------------------------------------------------
// Tick / delay  (1 tick = 1 ms for POSIX)
// ---------------------------------------------------------------------------
uint32_t bm_get_tick_count(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
uint32_t bm_get_tick_count_from_isr(void) { return bm_get_tick_count(); }
uint32_t bm_ms_to_ticks(uint32_t ms) { return ms; }
uint32_t bm_ticks_to_ms(uint32_t ticks) { return ticks; }
void bm_delay(uint32_t ms) { usleep(ms * 1000); }

