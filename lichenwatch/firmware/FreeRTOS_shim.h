/*
 * FreeRTOS_shim.h — Minimal FreeRTOS-compatible API surface so the firmware
 * compiles standalone in CI without the full FreeRTOS port. In a real build
 * this file is replaced by the genuine FreeRTOS headers.
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef FREERTOS_SHIM_H
#define FREERTOS_SHIM_H

#include <stdint.h>
#include <stddef.h>

typedef void *QueueHandle_t;
typedef void *TaskHandle_t;
typedef uint32_t TickType_t;
typedef int BaseType_t;

#define pdPASS        1
#define pdFAIL        0
#define pdMS_TO_TICKS(ms)  ((TickType_t)((ms) / (1000 / 1000)))
#define portMAX_DELAY      0xFFFFFFFFU

/* Task function pointer */
typedef void (*TaskFunction_t)(void *);

static inline QueueHandle_t xQueueCreate(uint32_t len, uint32_t sz)
{ (void)len; (void)sz; return (QueueHandle_t)1; }

static inline BaseType_t xQueueSendToBack(QueueHandle_t q, const void *p, TickType_t t)
{ (void)q; (void)p; (void)t; return pdPASS; }

static inline BaseType_t xQueueOverwrite(QueueHandle_t q, const void *p)
{ (void)q; (void)p; return pdPASS; }

static inline BaseType_t xQueueReceive(QueueHandle_t q, void *p, TickType_t t)
{ (void)q; (void)p; (void)t; return pdPASS; }

static inline TickType_t xTaskGetTickCount(void) { return 0; }

static inline void vTaskDelay(TickType_t t) { (void)t; }
static inline void vTaskDelayUntil(TickType_t *last, TickType_t inc) { (void)last; (void)inc; }

static inline BaseType_t xTaskCreate(TaskFunction_t fn, const char *n, uint16_t st,
                                     void *arg, uint32_t pri, TaskHandle_t *h)
{ (void)fn; (void)n; (void)st; (void)arg; (void)pri; (void)h; return pdPASS; }

static inline void vTaskStartScheduler(void) { /* never returns in real RTOS */ }

#endif /* FREERTOS_SHIM_H */