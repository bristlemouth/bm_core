#include "bm_os.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "stream_buffer.h"
#include "task.h"
#include "timers.h"

void *bm_malloc(size_t size) { return pvPortMalloc(size); }

void bm_free(void *ptr) { vPortFree(ptr); }

BmQueue bm_queue_create(uint32_t queue_length, uint32_t item_size) {
  return xQueueCreate(queue_length, item_size);
}

void bm_queue_delete(BmQueue queue) { vQueueDelete((QueueHandle_t)queue); }

BmErr bm_queue_receive(BmQueue queue, void *item, uint32_t timeout_ms) {
  if (xQueueReceive(queue, item, pdMS_TO_TICKS(timeout_ms)) == pdPASS) {
    return BmOK;
  } else {
    return BmETIMEDOUT;
  }
}

BmErr bm_queue_send(BmQueue queue, const void *item, uint32_t timeout_ms) {
  if (xQueueSend(queue, item, pdMS_TO_TICKS(timeout_ms)) == pdPASS) {
    return BmOK;
  } else {
    return BmENOMEM;
  }
}

BmErr bm_queue_send_to_front_from_isr(BmQueue queue, const void *item) {
  BaseType_t higher_priority_task_woken = pdFALSE;
  if (xQueueSendToFrontFromISR(queue, item, &higher_priority_task_woken) ==
      pdPASS) {
    // The portYIELD_FROM_ISR() is safe to do on ARM Cortex-M architectures because
    // it sets a pending switch bit in the NVIC such that once all interrupts are complete
    // it knows to tell the scheduler to do a context switch. On some other architectures,
    // the yield may immediately call the scheduler to perform the context switch so this
    // function should be the last thing called from the ISR if possible.
    portYIELD_FROM_ISR(higher_priority_task_woken);
    return BmOK;
  } else {
    return BmENOMEM;
  }
}

BmBuffer bm_stream_buffer_create(uint32_t max_size) {
  return (BmBuffer)xStreamBufferCreate(max_size, 0);
}

void bm_stream_buffer_delete(BmBuffer buf) {
  if (buf) {
    vStreamBufferDelete((StreamBufferHandle_t)buf);
  }
}

BmErr bm_stream_buffer_send(BmBuffer buf, uint8_t *data, uint32_t size,
                            uint32_t timeout_ms) {
  BmErr err = BmEINVAL;

  if (buf) {
    xStreamBufferSend((StreamBufferHandle_t)buf, (const void *)data, size,
                      pdMS_TO_TICKS(timeout_ms));
    err = BmOK;
  }

  return err;
}

BmErr bm_stream_buffer_receive(BmBuffer buf, uint8_t *data, uint32_t *size,
                               uint32_t timeout_ms) {
  BmErr err = BmEINVAL;

  if (buf) {
    *size = xStreamBufferReceive((StreamBufferHandle_t)buf, (void *)data, *size,
                                 pdMS_TO_TICKS(timeout_ms));
    err = BmOK;
  }

  return err;
}

BmSemaphore bm_semaphore_create(void) { return xSemaphoreCreateMutex(); }

void bm_semaphore_delete(BmSemaphore semaphore) {
  vSemaphoreDelete((SemaphoreHandle_t)semaphore);
}

BmErr bm_semaphore_give(BmSemaphore semaphore) {
  if (xSemaphoreGive(semaphore) == pdPASS) {
    return BmOK;
  } else {
    return BmEPERM;
  }
}

BmErr bm_semaphore_take(BmSemaphore semaphore, uint32_t timeout_ms) {
  if (xSemaphoreTake(semaphore, pdMS_TO_TICKS(timeout_ms)) == pdPASS) {
    return BmOK;
  } else {
    return BmETIMEDOUT;
  }
}

BmErr bm_task_create(void (*task)(void *), const char *name,
                     uint32_t stack_size, void *arg, uint32_t priority,
                     BmTaskHandle task_handle) {
  if (xTaskCreate(task, name, stack_size, arg, priority, task_handle) ==
      pdPASS) {
    return BmOK;
  } else {
    return BmENOMEM;
  }
}

void bm_task_delete(BmTaskHandle task_handle) {
  vTaskDelete((TaskHandle_t)task_handle);
}

void bm_start_scheduler(void) { vTaskStartScheduler(); }

BmTimer bm_timer_create(const char *name, uint32_t period_ms, bool auto_reload,
                        void *timer_id, void (*callback)(void *)) {
  return xTimerCreate(name, pdMS_TO_TICKS(period_ms), (UBaseType_t)auto_reload,
                      timer_id, (TimerCallbackFunction_t)callback);
}

void bm_timer_delete(BmTimer timer, uint32_t timeout_ms) {
  xTimerDelete((TimerHandle_t)timer, timeout_ms);
}

BmErr bm_timer_start(BmTimer timer, uint32_t timeout_ms) {
  if (xTimerStart(timer, pdMS_TO_TICKS(timeout_ms)) == pdPASS) {
    return BmOK;
  } else {
    return BmETIMEDOUT;
  }
}

BmErr bm_timer_stop(BmTimer timer, uint32_t timeout_ms) {
  if (xTimerStop(timer, pdMS_TO_TICKS(timeout_ms)) == pdPASS) {
    return BmOK;
  } else {
    return BmETIMEDOUT;
  }
}

BmErr bm_timer_change_period(BmTimer timer, uint32_t period_ms,
                             uint32_t timeout_ms) {
  if (xTimerChangePeriod(timer, pdMS_TO_TICKS(period_ms),
                         pdMS_TO_TICKS(timeout_ms)) == pdPASS) {
    return BmOK;
  } else {
    return BmETIMEDOUT;
  }
}

uint32_t bm_timer_get_id(BmTimer timer) {
    return (uint32_t) pvTimerGetTimerID(timer);
}

uint32_t bm_get_tick_count(void) { return xTaskGetTickCount(); }

uint32_t bm_get_tick_count_from_isr(void) { return xTaskGetTickCountFromISR(); }

uint32_t bm_ms_to_ticks(uint32_t ms) { return pdMS_TO_TICKS(ms); }

uint32_t bm_ticks_to_ms(uint32_t ticks) { return pdTICKS_TO_MS(ticks); }

void bm_delay(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
