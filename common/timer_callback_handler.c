#include "timer_callback_handler.h"
#include "bm_os.h"

#define TIMER_CB_QUEUE_LEN (10)

typedef struct TimerCallbackHandlerCtx {
  BmQueue cb_queue;
} TimerCallbackHandlerCtx;

typedef struct TimerCallbackHandlerEvent {
  timer_handler_cb cb;
  void *arg;
} TimerCallbackHandlerEvent;

static TimerCallbackHandlerCtx CTX;

static void timer_callback_handler_task(void *arg) {
  (void)arg;
  TimerCallbackHandlerEvent evt;
  while (true) {
    if (bm_queue_receive(CTX.cb_queue, &evt, BM_MAX_DELAY_UINT32) == BmOK) {
      if (evt.cb) {
        evt.cb(evt.arg);
      }
    }
  }
}

BmErr timer_callback_handler_init() {
  CTX.cb_queue = bm_queue_create(TIMER_CB_QUEUE_LEN,
                                  sizeof(TimerCallbackHandlerEvent));
  if (!CTX.cb_queue) {
    return BmENOMEM;
  }
  if (bm_task_create(timer_callback_handler_task, "timer_cb_handler", 1024, NULL,
                 TIMER_HANDLER_TASK_PRIORITY, NULL) != BmOK) {
    return BmENOMEM;
  }
  return BmOK;
}

bool timer_callback_handler_send_cb(timer_handler_cb cb, void *arg,
                                    uint32_t timeoutMs) {
  TimerCallbackHandlerEvent evt = {
      .cb = cb,
      .arg = arg,
  };
  return (bm_queue_send(CTX.cb_queue, &evt, bm_ms_to_ticks(timeoutMs)) ==
          BmOK);
}
