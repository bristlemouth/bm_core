#include "bm_os.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
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

BmSemaphore bm_semaphore_create(void) { return xSemaphoreCreateMutex(); }

void bm_semaphore_delete(BmSemaphore semaphore) { vSemaphoreDelete((SemaphoreHandle_t)semaphore); }

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

BmErr bm_task_create(void (*task)(void *), const char *name, uint32_t stack_size, void *arg,
                       uint32_t priority, BmTaskHandle task_handle) {
  if (xTaskCreate(task, name, stack_size, arg, priority, task_handle) == pdPASS) {
    return BmOK;
  } else {
    return BmENOMEM;
  }
}

void bm_task_delete(BmTaskHandle task_handle) { vTaskDelete((TaskHandle_t)task_handle); }

void bm_start_scheduler(void) {
  vTaskStartScheduler();
}

BmTimer bm_timer_create(const char *name, uint32_t period_ms, bool auto_reload,
                        void *timer_id, void (*callback)(void *)) {
  return xTimerCreate(name, pdMS_TO_TICKS(period_ms), (UBaseType_t)auto_reload, timer_id,
                      (TimerCallbackFunction_t)callback);
}

void bm_timer_delete(BmTimer timer, uint32_t timeout_ms) { xTimerDelete((TimerHandle_t)timer, timeout_ms); }

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

BmErr bm_timer_change_period(BmTimer timer, uint32_t period_ms, uint32_t timeout_ms) {
  if (xTimerChangePeriod(timer, pdMS_TO_TICKS(period_ms), pdMS_TO_TICKS(timeout_ms)) ==
      pdPASS) {
    return BmOK;
  } else {
    return BmETIMEDOUT;
  }
}

uint32_t bm_get_tick_count(void) { return xTaskGetTickCount(); }

uint32_t bm_get_tick_count_from_isr(void) { return xTaskGetTickCountFromISR(); }

uint32_t bm_ms_to_ticks(uint32_t ms) { return pdMS_TO_TICKS(ms); }

uint32_t bm_ticks_to_ms(uint32_t ticks) { return pdTICKS_TO_MS(ticks); }

void bm_delay(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }