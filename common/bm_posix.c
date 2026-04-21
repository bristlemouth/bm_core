/// @file bm_posix.c
/// @brief POSIX implementation of bm_os.h APIs for bm_sbc.

// Request POSIX.1-2008 interfaces (clock_gettime, etc.) on Linux/glibc.
#define _POSIX_C_SOURCE 200809L

#include "bm_os.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Compute an absolute timespec deadline from a relative timeout in ms.
/// Uses CLOCK_REALTIME because macOS lacks pthread_condattr_setclock.
static void deadline_from_ms(uint32_t timeout_ms, struct timespec *ts) {
  clock_gettime(CLOCK_REALTIME, ts);
  uint64_t ns = (uint64_t)timeout_ms * 1000000ULL;
  ts->tv_sec += (time_t)(ns / 1000000000ULL);
  ts->tv_nsec += (long)(ns % 1000000000ULL);
  if (ts->tv_nsec >= 1000000000L) {
    ts->tv_sec++;
    ts->tv_nsec -= 1000000000L;
  }
}

// ---------------------------------------------------------------------------
// Memory
// ---------------------------------------------------------------------------
void *bm_malloc(size_t size) { return malloc(size); }
void bm_free(void *ptr) { free(ptr); }

// ---------------------------------------------------------------------------
// Queues
// ---------------------------------------------------------------------------

typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t not_empty;
  pthread_cond_t not_full;
  uint8_t *storage;
  uint32_t item_size;
  uint32_t capacity;
  uint32_t count;
  uint32_t head; // dequeue index
  uint32_t tail; // enqueue index
} PosixQueue;

BmQueue bm_queue_create(uint32_t queue_length, uint32_t item_size) {
  PosixQueue *q = (PosixQueue *)calloc(1, sizeof(PosixQueue));
  if (!q) {
    return NULL;
  }
  q->storage = (uint8_t *)calloc(queue_length, item_size);
  if (!q->storage) {
    free(q);
    return NULL;
  }
  pthread_mutex_init(&q->lock, NULL);
  pthread_cond_init(&q->not_empty, NULL);
  pthread_cond_init(&q->not_full, NULL);
  q->item_size = item_size;
  q->capacity = queue_length;
  return (BmQueue)q;
}

void bm_queue_delete(BmQueue queue) {
  PosixQueue *q = (PosixQueue *)queue;
  if (q) {
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    free(q->storage);
    free(q);
  }
}

BmErr bm_queue_receive(BmQueue queue, void *item, uint32_t timeout_ms) {
  PosixQueue *q = (PosixQueue *)queue;
  if (!q || !item) {
    return BmEINVAL;
  }

  pthread_mutex_lock(&q->lock);
  while (q->count == 0) {
    if (timeout_ms == 0) {
      pthread_mutex_unlock(&q->lock);
      return BmETIMEDOUT;
    }
    if (timeout_ms == UINT32_MAX) {
      pthread_cond_wait(&q->not_empty, &q->lock);
    } else {
      struct timespec ts;
      deadline_from_ms(timeout_ms, &ts);
      if (pthread_cond_timedwait(&q->not_empty, &q->lock, &ts) == ETIMEDOUT) {
        pthread_mutex_unlock(&q->lock);
        return BmETIMEDOUT;
      }
    }
  }

  memcpy(item, q->storage + (q->head * q->item_size), q->item_size);
  q->head = (q->head + 1) % q->capacity;
  q->count--;
  pthread_cond_signal(&q->not_full);
  pthread_mutex_unlock(&q->lock);
  return BmOK;
}

BmErr bm_queue_send(BmQueue queue, const void *item, uint32_t timeout_ms) {
  PosixQueue *q = (PosixQueue *)queue;
  if (!q || !item) {
    return BmEINVAL;
  }

  pthread_mutex_lock(&q->lock);
  while (q->count == q->capacity) {
    if (timeout_ms == 0) {
      pthread_mutex_unlock(&q->lock);
      return BmENOMEM;
    }
    if (timeout_ms == UINT32_MAX) {
      pthread_cond_wait(&q->not_full, &q->lock);
    } else {
      struct timespec ts;
      deadline_from_ms(timeout_ms, &ts);
      if (pthread_cond_timedwait(&q->not_full, &q->lock, &ts) == ETIMEDOUT) {
        pthread_mutex_unlock(&q->lock);
        return BmETIMEDOUT;
      }
    }
  }

  memcpy(q->storage + (q->tail * q->item_size), item, q->item_size);
  q->tail = (q->tail + 1) % q->capacity;
  q->count++;
  pthread_cond_signal(&q->not_empty);
  pthread_mutex_unlock(&q->lock);
  return BmOK;
}

BmErr bm_queue_send_to_front_from_isr(BmQueue queue, const void *item) {
  PosixQueue *q = (PosixQueue *)queue;
  if (!q || !item) {
    return BmEINVAL;
  }

  pthread_mutex_lock(&q->lock);
  if (q->count == q->capacity) {
    pthread_mutex_unlock(&q->lock);
    return BmENOMEM;
  }

  // Insert at front: move head backwards
  q->head = (q->head == 0) ? q->capacity - 1 : q->head - 1;
  memcpy(q->storage + (q->head * q->item_size), item, q->item_size);
  q->count++;
  pthread_cond_signal(&q->not_empty);
  pthread_mutex_unlock(&q->lock);
  return BmOK;
}

// ---------------------------------------------------------------------------
// Stream buffers
// ---------------------------------------------------------------------------

typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t not_empty;
  pthread_cond_t not_full;
  uint8_t *storage;
  uint32_t capacity;
  uint32_t count;
  uint32_t head;
  uint32_t tail;
} PosixStreamBuffer;

BmBuffer bm_stream_buffer_create(uint32_t max_size) {
  PosixStreamBuffer *sb = (PosixStreamBuffer *)calloc(1, sizeof(*sb));
  if (!sb) {
    return NULL;
  }
  sb->storage = (uint8_t *)malloc(max_size);
  if (!sb->storage) {
    free(sb);
    return NULL;
  }
  pthread_mutex_init(&sb->lock, NULL);
  pthread_cond_init(&sb->not_empty, NULL);
  pthread_cond_init(&sb->not_full, NULL);
  sb->capacity = max_size;
  return (BmBuffer)sb;
}

void bm_stream_buffer_delete(BmBuffer buf) {
  PosixStreamBuffer *sb = (PosixStreamBuffer *)buf;
  if (sb) {
    pthread_mutex_destroy(&sb->lock);
    pthread_cond_destroy(&sb->not_empty);
    pthread_cond_destroy(&sb->not_full);
    free(sb->storage);
    free(sb);
  }
}

BmErr bm_stream_buffer_send(BmBuffer buf, uint8_t *data, uint32_t size,
                            uint32_t timeout_ms) {
  PosixStreamBuffer *sb = (PosixStreamBuffer *)buf;
  if (!sb || !data) {
    return BmEINVAL;
  }

  struct timespec ts;
  if (timeout_ms != 0 && timeout_ms != UINT32_MAX) {
    deadline_from_ms(timeout_ms, &ts);
  }

  pthread_mutex_lock(&sb->lock);
  uint32_t written = 0;
  BmErr err = BmEINVAL;
  while (written < size) {
    while (sb->count == sb->capacity) {
      if (timeout_ms == 0) {
        err = BmENOMEM;
        goto done;
      }
      if (timeout_ms == UINT32_MAX) {
        pthread_cond_wait(&sb->not_full, &sb->lock);
      } else {
        if (pthread_cond_timedwait(&sb->not_full, &sb->lock, &ts) ==
            ETIMEDOUT) {
          err = BmETIMEDOUT;
          goto done;
        }
      }
    }
    uint32_t avail = sb->capacity - sb->count;
    uint32_t to_write = size - written;
    if (to_write > avail) {
      to_write = avail;
    }
    for (uint32_t i = 0; i < to_write; i++) {
      sb->storage[sb->tail] = data[written + i];
      sb->tail = (sb->tail + 1) % sb->capacity;
    }
    sb->count += to_write;
    written += to_write;
    pthread_cond_signal(&sb->not_empty);
  }
  err = BmOK;

done:
  pthread_mutex_unlock(&sb->lock);
  return err;
}

BmErr bm_stream_buffer_receive(BmBuffer buf, uint8_t *data, uint32_t *size,
                               uint32_t timeout_ms) {
  PosixStreamBuffer *sb = (PosixStreamBuffer *)buf;
  if (!sb || !data || !size) {
    return BmEINVAL;
  }

  uint32_t max_read = *size;
  *size = 0;

  struct timespec ts;
  if (timeout_ms != 0 && timeout_ms != UINT32_MAX) {
    deadline_from_ms(timeout_ms, &ts);
  }

  BmErr err = BmETIMEDOUT;
  pthread_mutex_lock(&sb->lock);
  // Block until at least 1 byte is available
  while (sb->count == 0) {
    if (timeout_ms == 0) {
      goto done;
    }
    if (timeout_ms == UINT32_MAX) {
      pthread_cond_wait(&sb->not_empty, &sb->lock);
    } else {
      if (pthread_cond_timedwait(&sb->not_empty, &sb->lock, &ts) == ETIMEDOUT) {
        goto done;
      }
    }
  }
  uint32_t to_read = sb->count;
  if (to_read > max_read) {
    to_read = max_read;
  }
  for (uint32_t i = 0; i < to_read; i++) {
    data[i] = sb->storage[sb->head];
    sb->head = (sb->head + 1) % sb->capacity;
  }
  sb->count -= to_read;
  *size = to_read;
  pthread_cond_signal(&sb->not_full);
  err = BmOK;

done:
  pthread_mutex_unlock(&sb->lock);
  return err;
}

// ---------------------------------------------------------------------------
// Mutex / Semaphore
// ---------------------------------------------------------------------------

typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t cond;
  uint32_t count;
} PosixSemaphore;

BmSemaphore bm_mutex_create(void) {
  PosixSemaphore *s = (PosixSemaphore *)calloc(1, sizeof(*s));
  if (!s) {
    return NULL;
  }
  pthread_mutex_init(&s->lock, NULL);
  pthread_cond_init(&s->cond, NULL);
  s->count = 1; // Mutex starts available
  return (BmSemaphore)s;
}

BmSemaphore bm_semaphore_create(void) {
  PosixSemaphore *s = (PosixSemaphore *)calloc(1, sizeof(*s));
  if (!s) {
    return NULL;
  }
  pthread_mutex_init(&s->lock, NULL);
  pthread_cond_init(&s->cond, NULL);
  s->count = 0; // Binary semaphore starts unavailable
  return (BmSemaphore)s;
}

void bm_semaphore_delete(BmSemaphore semaphore) {
  PosixSemaphore *s = (PosixSemaphore *)semaphore;
  if (s) {
    pthread_mutex_destroy(&s->lock);
    pthread_cond_destroy(&s->cond);
    free(s);
  }
}

BmErr bm_semaphore_give(BmSemaphore semaphore) {
  PosixSemaphore *s = (PosixSemaphore *)semaphore;
  if (!s) {
    return BmEINVAL;
  }
  pthread_mutex_lock(&s->lock);
  if (s->count == 0) {
    s->count = 1;
    pthread_cond_signal(&s->cond);
  }
  pthread_mutex_unlock(&s->lock);
  return BmOK;
}

BmErr bm_semaphore_take(BmSemaphore semaphore, uint32_t timeout_ms) {
  PosixSemaphore *s = (PosixSemaphore *)semaphore;
  if (!s) {
    return BmEINVAL;
  }
  pthread_mutex_lock(&s->lock);
  while (s->count == 0) {
    if (timeout_ms == 0) {
      pthread_mutex_unlock(&s->lock);
      return BmETIMEDOUT;
    }
    if (timeout_ms == UINT32_MAX) {
      pthread_cond_wait(&s->cond, &s->lock);
    } else {
      struct timespec ts;
      deadline_from_ms(timeout_ms, &ts);
      if (pthread_cond_timedwait(&s->cond, &s->lock, &ts) == ETIMEDOUT) {
        pthread_mutex_unlock(&s->lock);
        return BmETIMEDOUT;
      }
    }
  }
  s->count = 0;
  pthread_mutex_unlock(&s->lock);
  return BmOK;
}

// ---------------------------------------------------------------------------
// Tasks
// ---------------------------------------------------------------------------

typedef struct {
  BmTask func;
  void *arg;
} TaskTrampoline;

static void *posix_task_trampoline(void *param) {
  TaskTrampoline *t = (TaskTrampoline *)param;
  BmTask func = t->func;
  void *arg = t->arg;
  free(t);
  func(arg);
  return NULL;
}

BmErr bm_task_create(BmTask task, const char *name, uint32_t stack_size,
                     void *arg, uint32_t priority, BmTaskHandle task_handle) {
  (void)name;
  (void)stack_size;
  (void)priority;

  TaskTrampoline *t = (TaskTrampoline *)malloc(sizeof(*t));
  if (!t) {
    return BmENOMEM;
  }
  t->func = task;
  t->arg = arg;

  pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t));
  if (!thread) {
    free(t);
    return BmENOMEM;
  }

  int rc = pthread_create(thread, NULL, posix_task_trampoline, t);
  if (rc != 0) {
    free(t);
    free(thread);
    return BmEINVAL;
  }
  pthread_detach(*thread);

  // Store the handle if caller wants it (task_handle is actually a void**)
  if (task_handle) {
    *(void **)task_handle = (void *)thread;
  } else {
    free(thread);
  }
  return BmOK;
}

void bm_task_delete(BmTaskHandle task_handle) {
  // In POSIX, we can't cleanly kill a thread from outside without cooperation.
  // For now, just free the handle. The thread will exit when its function returns.
  if (task_handle) {
    free(task_handle);
  }
}

void bm_start_scheduler(void) {
  // No-op on POSIX: threads run immediately after creation.
}

// ---------------------------------------------------------------------------
// Timers
// ---------------------------------------------------------------------------

typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t cond;
  pthread_t thread;
  bool thread_created;
  bool running;
  bool auto_reload;
  bool delete_requested;
  bool
      self_delete; // true when bm_timer_delete was called from this timer's own thread
  bool needs_reset;
  uint32_t period_ms;
  void *timer_id;
  BmTimerCallback cb;
} PosixTimer;

static void *posix_timer_thread(void *param) {
  PosixTimer *t = (PosixTimer *)param;

  pthread_mutex_lock(&t->lock);
  for (;;) {
    // Phase 1: wait to be started (or deleted)
    while (!t->running && !t->delete_requested) {
      pthread_cond_wait(&t->cond, &t->lock);
    }
    if (t->delete_requested) {
      bool do_free = t->self_delete;
      pthread_mutex_unlock(&t->lock);
      if (do_free) {
        pthread_mutex_destroy(&t->lock);
        pthread_cond_destroy(&t->cond);
        free(t);
      }
      return NULL;
    }

    // Phase 2: running — sleep for the period
    struct timespec ts;
    deadline_from_ms(t->period_ms, &ts);
    t->needs_reset = false;

    while (t->running && !t->delete_requested) {
      int rc = pthread_cond_timedwait(&t->cond, &t->lock, &ts);
      if (t->delete_requested) {
        bool do_free = t->self_delete;
        pthread_mutex_unlock(&t->lock);
        if (do_free) {
          pthread_mutex_destroy(&t->lock);
          pthread_cond_destroy(&t->cond);
          free(t);
        }
        return NULL;
      }
      if (!t->running) {
        break; // stopped
      }
      if (t->needs_reset) {
        t->needs_reset = false;
        deadline_from_ms(t->period_ms, &ts);
        continue;
      }
      if (rc == ETIMEDOUT) {
        // Fire the callback (unlock during callback to avoid deadlock)
        BmTimerCallback cb = t->cb;
        pthread_mutex_unlock(&t->lock);
        cb((BmTimer)t);
        pthread_mutex_lock(&t->lock);
        if (t->auto_reload && t->running && !t->delete_requested) {
          deadline_from_ms(t->period_ms, &ts);
          continue;
        } else {
          t->running = false;
          break;
        }
      }
      // Spurious wakeup — just re-wait with same deadline
    }
  }
}

BmTimer bm_timer_create(const char *name, uint32_t period_ms, bool auto_reload,
                        void *timer_id, BmTimerCallback cb) {
  (void)name;
  PosixTimer *t = (PosixTimer *)calloc(1, sizeof(*t));
  if (!t) {
    return NULL;
  }
  pthread_mutex_init(&t->lock, NULL);
  pthread_cond_init(&t->cond, NULL);
  t->period_ms = period_ms;
  t->auto_reload = auto_reload;
  t->timer_id = timer_id;
  t->cb = cb;

  int rc = pthread_create(&t->thread, NULL, posix_timer_thread, t);
  if (rc != 0) {
    pthread_mutex_destroy(&t->lock);
    pthread_cond_destroy(&t->cond);
    free(t);
    return NULL;
  }
  t->thread_created = true;
  pthread_detach(t->thread);
  return (BmTimer)t;
}

void bm_timer_delete(BmTimer timer, uint32_t timeout_ms) {
  (void)timeout_ms;
  PosixTimer *t = (PosixTimer *)timer;
  if (!t) {
    return;
  }
  // If called from within this timer's own callback thread, destroying the
  // mutex here would crash posix_timer_thread when it tries to relock after
  // the callback returns.  Mark for deferred self-cleanup instead.
  if (t->thread_created && pthread_equal(pthread_self(), t->thread)) {
    pthread_mutex_lock(&t->lock);
    t->delete_requested = true;
    t->running = false;
    t->self_delete = true;
    pthread_mutex_unlock(&t->lock);
    return; // posix_timer_thread will free memory after cb() returns
  }
  pthread_mutex_lock(&t->lock);
  t->delete_requested = true;
  pthread_cond_signal(&t->cond);
  pthread_mutex_unlock(&t->lock);
  // Thread is detached; give it a moment to notice the delete flag and exit.
  struct timespec delay = {0, 1000000L}; // 1 ms
  nanosleep(&delay, NULL);
  pthread_mutex_destroy(&t->lock);
  pthread_cond_destroy(&t->cond);
  free(t);
}

BmErr bm_timer_reset(BmTimer timer, uint32_t timeout_ms) {
  (void)timeout_ms;
  PosixTimer *t = (PosixTimer *)timer;
  if (!t) {
    return BmEINVAL;
  }
  pthread_mutex_lock(&t->lock);
  t->running = true;
  t->needs_reset = true;
  pthread_cond_signal(&t->cond);
  pthread_mutex_unlock(&t->lock);
  return BmOK;
}

BmErr bm_timer_start(BmTimer timer, uint32_t timeout_ms) {
  (void)timeout_ms;
  PosixTimer *t = (PosixTimer *)timer;
  if (!t) {
    return BmEINVAL;
  }
  pthread_mutex_lock(&t->lock);
  t->running = true;
  t->needs_reset = true;
  pthread_cond_signal(&t->cond);
  pthread_mutex_unlock(&t->lock);
  return BmOK;
}

BmErr bm_timer_stop(BmTimer timer, uint32_t timeout_ms) {
  (void)timeout_ms;
  PosixTimer *t = (PosixTimer *)timer;
  if (!t) {
    return BmEINVAL;
  }
  pthread_mutex_lock(&t->lock);
  t->running = false;
  pthread_cond_signal(&t->cond);
  pthread_mutex_unlock(&t->lock);
  return BmOK;
}

BmErr bm_timer_change_period(BmTimer timer, uint32_t period_ms,
                             uint32_t timeout_ms) {
  (void)timeout_ms;
  PosixTimer *t = (PosixTimer *)timer;
  if (!t) {
    return BmEINVAL;
  }
  pthread_mutex_lock(&t->lock);
  t->period_ms = period_ms;
  t->needs_reset = true;
  pthread_cond_signal(&t->cond);
  pthread_mutex_unlock(&t->lock);
  return BmOK;
}

uint32_t bm_timer_get_id(BmTimer timer) {
  PosixTimer *t = (PosixTimer *)timer;
  if (!t) {
    return 0;
  }
  return (uint32_t)(uintptr_t)t->timer_id;
}

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
void bm_delay(uint32_t ms) {
  struct timespec delay = {ms / 1000, (ms % 1000) * 1000000L};
  nanosleep(&delay, NULL);
}
