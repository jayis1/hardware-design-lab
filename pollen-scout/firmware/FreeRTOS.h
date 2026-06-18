/*
 * FreeRTOS minimal stubs for Pollen Scout compile-readiness.
 * In a production build these are provided by FreeRTOS-Kernel + the
 * CMSIS-OS2 wrapper. The signatures here match FreeRTOS V10.4.3.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef FREE_RTOS_H
#define FREE_RTOS_H

#include <stdint.h>
#include <stddef.h>

typedef void *TaskHandle_t;
typedef void *QueueHandle_t;
typedef void *SemaphoreHandle_t;

typedef int32_t  BaseType_t;
typedef uint32_t TickType_t;
typedef uint32_t UBaseType_t;

#define pdPASS        1
#define pdFAIL        0
#define pdTRUE        1
#define pdFALSE       0
#define portMAX_DELAY 0xFFFFFFFFU

#define pdMS_TO_TICKS(ms)  ((TickType_t)((ms) / portTICK_PERIOD_MS))
#define portTICK_PERIOD_MS  1U

/* Task creation */
BaseType_t xTaskCreate(void (*task)(void *), const char *name,
                       uint16_t stack, void *arg, UBaseType_t prio,
                       TaskHandle_t *out);
void       vTaskStartScheduler(void);
void       vTaskDelay(TickType_t ticks);
void       vTaskDelayUntil(TickType_t *prev, TickType_t inc);
TickType_t xTaskGetTickCount(void);
TaskHandle_t xTaskGetHandle(const char *name);
void       vTaskSuspend(TaskHandle_t t);
void       vTaskResume(TaskHandle_t t);
void       ulTaskNotifyTake(BaseType_t clear, TickType_t to);

/* Queues */
QueueHandle_t xQueueCreate(UBaseType_t len, UBaseType_t sz);
BaseType_t    xQueueSend(QueueHandle_t q, const void *item, TickType_t to);
BaseType_t    xQueueReceive(QueueHandle_t q, void *item, TickType_t to);

/* Semaphores */
SemaphoreHandle_t xSemaphoreCreateMutex(void);
SemaphoreHandle_t xSemaphoreCreateBinary(void);
BaseType_t xSemaphoreTake(SemaphoreHandle_t s, TickType_t to);
BaseType_t xSemaphoreGive(SemaphoreHandle_t s);

/* System reset */
void NVIC_SystemReset(void);

#endif /* FREE_RTOS_H */