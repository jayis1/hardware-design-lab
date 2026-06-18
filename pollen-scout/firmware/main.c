/*
 * main.c - Pollen Scout firmware entry point and FreeRTOS task bootstrap
 *
 * Initializes all peripherals, creates the task set, and starts the
 * scheduler. Each subsystem driver exposes an _init() and a periodic
 * _step() / _sample() function called from its task.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* FreeRTOS (minimal stubs provided by cmsis_os2 layer in production) */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "board.h"
#include "registers.h"

#include "drivers/ov5640.h"
#include "drivers/bme280.h"
#include "drivers/si1145.h"
#include "drivers/pms5003.h"
#include "drivers/sdp810.h"
#include "drivers/sx1262.h"
#include "drivers/wind_rs485.h"
#include "drivers/bq25895.h"
#include "drivers/sd_fatfs.h"
#include "drivers/misc.h"

#include "ml/segment.h"
#include "ml/classifier.h"
#include "ml/forecaster.h"
#include "net/lorawan.h"

/* ---- Shared state ---- */
typedef struct {
    /* weather */
    float temperature_c;
    float humidity_pct;
    float pressure_pa;
    float uv_index;
    float wind_speed_mps;
    float wind_dir_deg;
    uint16_t pm1_0, pm2_5, pm10;
    /* power */
    uint16_t battery_mv;
    uint16_t solar_mv;
    uint8_t  charge_pct;
    /* flow */
    float flow_lpm;
    /* counts */
    uint32_t capture_count;
    uint32_t roi_count_total;
    /* per-taxon concentrations (grains/m3) for TAXA_CLASSES */
    float concentration[TAXA_CLASSES];
    /* 72-hour forecast for top-6 taxa */
    float forecast[6][72];
    uint32_t last_forecast_tick;
    SemaphoreHandle_t lock;
} scout_state_t;

static scout_state_t g_state;

/* ROI queue between capture and classify tasks */
typedef struct {
    uint8_t  pixels[ROI_MAX_DIM * ROI_MAX_DIM];  /* grayscale */
    uint8_t  w, h;
    uint16_t cx, cy;     /* centroid in source frame */
} roi_t;

static QueueHandle_t roi_queue;
static SemaphoreHandle_t capture_done_sem;

/* ---- Forward declarations ---- */
static void system_clock_init(void);
static void gpio_init_all(void);
static void led_set(int green, int red);

/* ---- Task prototypes ---- */
static void capture_task(void *arg);
static void segment_task(void *arg);
static void classify_task(void *arg);
static void forecast_task(void *arg);
static void flow_task(void *arg);
static void weather_task(void *arg);
static void telemetry_task(void *arg);
static void storage_task(void *arg);
static void power_task(void *arg);
static void ble_task(void *arg);

/* ====================================================================== */
/*  Startup                                                                */
/* ====================================================================== */
int main(void)
{
    memset(&g_state, 0, sizeof(g_state));
    g_state.lock = xSemaphoreCreateMutex();
    roi_queue      = xQueueCreate(ROI_MAX, sizeof(roi_t));
    capture_done_sem = xSemaphoreCreateBinary();

    system_clock_init();
    gpio_init_all();

    /* Driver init */
    ov5640_init();
    bme280_init();
    si1145_init();
    pms5003_init();
    sdp810_init();
    sx1262_init();
    wind_init();
    bq25895_init();
    sd_fatfs_init();

    /* ML init */
    segment_init();
    classifier_init();
    forecaster_init();

    /* Network init */
    lorawan_init(LORAWAN_REGION_DEFAULT);

    /* Task creation */
    xTaskCreate(capture_task,    "capture",   2048, NULL, 5, NULL);
    xTaskCreate(segment_task,    "segment",   4096, NULL, 4, NULL);
    xTaskCreate(classify_task,   "classify",  4096, NULL, 4, NULL);
    xTaskCreate(forecast_task,   "forecast",  2048, NULL, 2, NULL);
    xTaskCreate(flow_task,       "flow",      1024, NULL, 3, NULL);
    xTaskCreate(weather_task,    "weather",   1024, NULL, 3, NULL);
    xTaskCreate(telemetry_task,  "telemetry", 2048, NULL, 2, NULL);
    xTaskCreate(storage_task,    "storage",   2048, NULL, 1, NULL);
    xTaskCreate(power_task,      "power",     1024, NULL, 6, NULL);
    xTaskCreate(ble_task,        "ble",       1024, NULL, 1, NULL);

    led_set(1, 0);
    vTaskStartScheduler();

    /* Should never reach here */
    while (1) { }
    return 0;
}

/* ====================================================================== */
/*  Tasks                                                                  */
/* ====================================================================== */

/* capture_task: strobe + DMA a frame every CAPTURE_PERIOD_MS, then signal segment */
static void capture_task(void *arg)
{
    (void)arg;
    static uint8_t frame[IMG_WIDTH * IMG_HEIGHT * IMG_BPP_BYTES]
        __attribute__((aligned(32)));
    TickType_t last = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(CAPTURE_PERIOD_MS));

        /* Adaptive power: skip capture if battery critically low */
        xSemaphoreTake(g_state.lock, portMAX_DELAY);
        uint16_t bat = g_state.battery_mv;
        xSemaphoreGive(g_state.lock);
        if (bat < BAT_CRITICAL_MV) {
            continue;
        }

        /* White-channel capture */
        ov5640_strobe_select(OV5640_STROBE_WHITE);
        if (ov5640_capture(frame, sizeof(frame)) == 0) {
            xSemaphoreGive(capture_done_sem);   /* wake segment_task */
        } else {
            led_set(0, 1);  /* capture error */
        }

        /* UV-channel capture (autofluorescence) */
        ov5640_strobe_select(OV5640_STROBE_UV);
        uint8_t frame_uv[IMG_WIDTH * IMG_HEIGHT * IMG_BPP_BYTES]
            __attribute__((aligned(32)));
        if (ov5640_capture(frame_uv, sizeof(frame_uv)) == 0) {
            /* hand UV frame to classifier as auxiliary channel via queue flag */
            roi_t aux;
            aux.cx = 0xFFFF;   /* sentinel: UV auxiliary frame */
            xQueueSend(roi_queue, &aux, 0);
        }

        xSemaphoreTake(g_state.lock, portMAX_DELAY);
        g_state.capture_count++;
        xSemaphoreGive(g_state.lock);
    }
}

/* segment_task: wait for capture, segment frame into ROIs, enqueue them */
static void segment_task(void *arg)
{
    (void)arg;
    roi_t rois[ROI_MAX];
    int n;

    for (;;) {
        xSemaphoreTake(capture_done_sem, portMAX_DELAY);

        /* Access the last captured frame via the driver's back-buffer */
        const uint8_t *frame = ov5640_last_frame();
        if (!frame) continue;

        n = segment_frame(frame, IMG_WIDTH, IMG_HEIGHT, rois, ROI_MAX);
        for (int i = 0; i < n; i++) {
            if (xQueueSend(roi_queue, &rois[i], 0) != pdPASS) {
                /* queue full -> drop oldest */
                roi_t dropped;
                xQueueReceive(roi_queue, &dropped, 0);
                xQueueSend(roi_queue, &rois[i], 0);
            }
        }
        xSemaphoreTake(g_state.lock, portMAX_DELAY);
        g_state.roi_count_total += n;
        xSemaphoreGive(g_state.lock);
    }
}

/* classify_task: dequeue ROIs, run int8 CNN, accumulate per-taxon concentrations */
static void classify_task(void *arg)
{
    (void)arg;
    roi_t roi;
    float logits[TAXA_CLASSES];
    float volume_m3;     /* air volume sampled this cycle */

    for (;;) {
        if (xQueueReceive(roi_queue, &roi, portMAX_DELAY) == pdPASS) {
            if (roi.cx == 0xFFFF) {
                /* UV auxiliary frame flag — classifier uses it as 2nd input */
                classifier_set_aux_uv(1);
                continue;
            }
            int top = classifier_predict(&roi, logits);
            if (top >= 0 && top < TAXA_CLASSES) {
                /* convert count -> concentration using sampled air volume */
                xSemaphoreTake(g_state.lock, portMAX_DELAY);
                volume_m3 = (g_state.flow_lpm / 60.0f) *
                            (CAPTURE_PERIOD_MS / 1000.0f) / 1000.0f;
                if (volume_m3 > 1e-6f) {
                    g_state.concentration[top] += 1.0f / volume_m3;
                }
                xSemaphoreGive(g_state.lock);
            }
            classifier_set_aux_uv(0);
        }
    }
}

/* forecast_task: every 10 min run TCN forecaster on 24 h history */
static void forecast_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(FORECAST_PERIOD_MS));
        xSemaphoreTake(g_state.lock, portMAX_DELAY);
        forecaster_result_t r;
        forecaster_update(g_state.concentration,
                          g_state.temperature_c,
                          g_state.humidity_pct,
                          g_state.wind_speed_mps,
                          g_state.wind_dir_deg,
                          &r);
        memcpy(g_state.forecast, r.forecast, sizeof(r.forecast));
        g_state.last_forecast_tick = xTaskGetTickCount();
        xSemaphoreGive(g_state.lock);
    }
}

/* flow_task: PI loop on blower PWM to hold 2 L/min using SDP810 feedback */
static void flow_task(void *arg)
{
    (void)arg;
    const float target = 2.0f;     /* L/min */
    float integ = 0.0f;
    TickType_t last = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(FLOW_PERIOD_MS));

        float measured = sdp810_read_flow_lpm();
        float err = target - measured;
        float kp = 12.0f, ki = 0.4f;
        integ += err * (FLOW_PERIOD_MS / 1000.0f);
        if (integ > 100.0f) integ = 100.0f;
        if (integ < 0.0f)   integ = 0.0f;
        float pwm_pct = kp * err + ki * integ;
        if (pwm_pct < 0.0f)   pwm_pct = 0.0f;
        if (pwm_pct > 100.0f) pwm_pct = 100.0f;

        /* suspend blower if battery low */
        xSemaphoreTake(g_state.lock, portMAX_DELAY);
        if (g_state.battery_mv < BAT_LOW_MV) pwm_pct = 0.0f;
        g_state.flow_lpm = measured;
        xSemaphoreGive(g_state.lock);

        blower_set_pwm((uint8_t)pwm_pct);
    }
}

/* weather_task: sample all environmental sensors every 2 s */
static void weather_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(WEATHER_PERIOD_MS));

        bme280_data_t b;  bme280_read(&b);
        float uv = si1145_read_uv();
        wind_data_t w;    wind_read(&w);
        pms_data_t p;     pms5003_read(&p);

        xSemaphoreTake(g_state.lock, portMAX_DELAY);
        g_state.temperature_c = b.temp_c;
        g_state.humidity_pct  = b.rh_pct;
        g_state.pressure_pa   = b.pres_pa;
        g_state.uv_index      = uv;
        g_state.wind_speed_mps = w.speed_mps;
        g_state.wind_dir_deg   = w.dir_deg;
        g_state.pm1_0 = p.pm1_0;
        g_state.pm2_5 = p.pm2_5;
        g_state.pm10  = p.pm10;
        xSemaphoreGive(g_state.lock);
    }
}

/* telemetry_task: uplink compressed state over LoRaWAN every 10 min */
static void telemetry_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    uint8_t buf[64];

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));

        xSemaphoreTake(g_state.lock, portMAX_DELAY);
        /* Pack: timestamp(4) bat(2) solar(2) temp(1) rh(1) wind(2)
         *       pm2.5(2) pm10(2) top-6 taxa conc(6*2) */
        uint32_t tick = xTaskGetTickCount();
        memcpy(buf + 0, &tick, 4);
        memcpy(buf + 4, &g_state.battery_mv, 2);
        memcpy(buf + 6, &g_state.solar_mv, 2);
        int8_t t = (int8_t)g_state.temperature_c;
        buf[8] = (uint8_t)t;
        buf[9] = (uint8_t)g_state.humidity_pct;
        uint16_t ws = (uint16_t)(g_state.wind_speed_mps * 10.0f);
        memcpy(buf + 10, &ws, 2);
        memcpy(buf + 12, &g_state.pm2_5, 2);
        memcpy(buf + 14, &g_state.pm10, 2);
        for (int i = 0; i < 6; i++) {
            uint16_t c = (uint16_t)g_state.concentration[i];
            memcpy(buf + 16 + 2 * i, &c, 2);
        }
        xSemaphoreGive(g_state.lock);

        lorawan_send(buf, 28, /*confirmed=*/1);
    }
}

/* storage_task: append a CSV row + raw image ring to microSD */
static void storage_task(void *arg)
{
    (void)arg;
    char line[256];

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        xSemaphoreTake(g_state.lock, portMAX_DELAY);
        int len = snprintf(line, sizeof(line),
            "%lu,%.1f,%.1f,%.0f,%.2f,%.1f,%.0f,%u,%u,%u,%u,%u,%u\n",
            (unsigned long)xTaskGetTickCount(),
            g_state.temperature_c, g_state.humidity_pct,
            (float)g_state.pressure_pa, g_state.uv_index,
            g_state.wind_speed_mps, g_state.wind_dir_deg,
            g_state.pm1_0, g_state.pm2_5, g_state.pm10,
            g_state.battery_mv, g_state.solar_mv,
            g_state.charge_pct);
        xSemaphoreGive(g_state.lock);

        if (len > 0) sd_fatfs_append_csv(line, len);
    }
}

/* power_task: poll charger/ADC, compute charge %, adapt duty cycle */
static void power_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(POWER_PERIOD_MS));

        bq25895_status_t st;
        bq25895_read(&st);

        xSemaphoreTake(g_state.lock, portMAX_DELAY);
        g_state.battery_mv = st.bat_mv;
        g_state.solar_mv   = st.vin_mv;
        g_state.charge_pct = st.charge_pct;
        uint16_t bat = g_state.battery_mv;
        xSemaphoreGive(g_state.lock);

        /* Adaptive duty cycling */
        if (bat < BAT_LOW_MV) {
            /* Double capture interval by suspending capture task briefly */
            vTaskSuspend(xTaskGetHandle("capture"));
            vTaskDelay(pdMS_TO_TICKS(CAPTURE_PERIOD_MS));
            vTaskResume(xTaskGetHandle("capture"));
        }
        led_set(st.charging ? 1 : 0, 0);
    }
}

/* ble_task: bridge commands from ESP32-C3 (provisioning, OTA trigger) */
static void ble_task(void *arg)
{
    (void)arg;
    uint8_t cmd[32];
    for (;;) {
        int n = esp32_bridge_recv(cmd, sizeof(cmd), portMAX_DELAY);
        if (n <= 0) continue;
        /* Minimal command parser: 'O' -> OTA, 'P' -> ping, 'R' -> reset */
        switch (cmd[0]) {
        case 'O':
            esp32_ota_start();
            break;
        case 'R':
            NVIC_SystemReset();
            break;
        case 'P':
            esp32_bridge_send((const uint8_t *)"OK", 2);
            break;
        default:
            break;
        }
    }
}

/* ====================================================================== */
/*  Low-level helpers                                                      */
/* ====================================================================== */

/* 480 MHz from 16 MHz HSE: PLL1M=4 -> 4 MHz VCO in, PLL1N=120 -> 480 MHz,
 * PLL1P=1 -> 480 MHz SYS; HPRE=2 -> 240 MHz HCLK? H7 uses fixed /1 for sys.
 * Simplified for readability; production uses LL/Cube HAL. */
static void system_clock_init(void)
{
    /* Enable HSE */
    RCC_CR |= BIT(16);                       /* HSEON */
    while (!(RCC_CR & BIT(17))) { }          /* HSERDY */

    /* Configure PLL1: M=4, N=120, P=1, Q=20, R=20 */
    RCC_PLLCFGR = (4U << 4) | (120U << 8) | (1U << 16) | (20U << 24) | BIT(0);
    RCC_CR |= BIT(24);                       /* PLL1ON */
    while (!(RCC_CR & BIT(25))) { }          /* PLL1RDY */

    /* Switch SYSCLK to PLL1 */
    RCC_CFGR = (3U << 0);                    /* SW = PLL1 */
    while (((RCC_CFGR >> 4) & 3) != 3) { }   /* SWS = PLL1 */

    /* Enable peripheral clocks */
    RCC_AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_ADC12EN | RCC_AHB1ENR_DCMIEN;
    RCC_AHB2ENR |= RCC_AHB2ENR_SDMMC1EN;
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                   RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                   RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOFEN;
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN | RCC_APB1ENR_USART3EN |
                   RCC_APB1ENR_UART4EN | RCC_APB1ENR_TIM3EN |
                   RCC_APB1ENR_TIM4EN;
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_USART6EN |
                   RCC_APB2ENR_TIM1EN;
    RCC_APB4ENR |= RCC_APB4ENR_I2C3EN | RCC_APB4ENR_I2C4EN;
}

static void gpio_init_all(void)
{
    /* Status LEDs: output push-pull */
    GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET) &= ~(3U << (LED_STATUS_PIN * 2));
    GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET) |=  (1U << (LED_STATUS_PIN * 2));
    GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET) &= ~(3U << (LED_ERROR_PIN * 2));
    GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET) |=  (1U << (LED_ERROR_PIN * 2));

    /* Strobe pins as alternate function (TIM4) */
    GPIO_REG(GPIOD_BASE, GPIO_MODER_OFFSET) &= ~(3U << (STROBE_WHITE_PIN * 2));
    GPIO_REG(GPIOD_BASE, GPIO_MODER_OFFSET) |=  (2U << (STROBE_WHITE_PIN * 2));
    GPIO_REG(GPIOD_BASE, GPIO_MODER_OFFSET) &= ~(3U << (STROBE_UV_PIN * 2));
    GPIO_REG(GPIOD_BASE, GPIO_MODER_OFFSET) |=  (2U << (STROBE_UV_PIN * 2));
}

static void led_set(int green, int red)
{
    GPIO_REG(LED_STATUS_PORT, GPIO_BSRR_OFFSET) =
        green ? (1U << LED_STATUS_PIN) : (1U << (LED_STATUS_PIN + 16));
    GPIO_REG(LED_ERROR_PORT, GPIO_BSRR_OFFSET) =
        red   ? (1U << LED_ERROR_PIN)   : (1U << (LED_ERROR_PIN + 16));
}

/* ---- FreeRTOS hooks ---- */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask; (void)pcTaskName;
    led_set(0, 1);
    while (1) { }
}

void vApplicationMallocFailedHook(void)
{
    led_set(0, 1);
    while (1) { }
}

void HardFault_Handler(void)
{
    led_set(0, 1);
    while (1) { }
}