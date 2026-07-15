/*
 * main.c — ThermoGlyph firmware main entry point
 *
 * FreeRTOS task initialization and main system loop.
 * Runs on nRF5340 app core.
 *
 * Tasks:
 *   - thermal_task (200 Hz): PID loop, PWM matrix drive, safety
 *   - glyph_task (50 Hz): glyph engine rendering
 *   - imu_task (200 Hz): IMU sampling + gesture detection
 *   - comm_task (event-driven): BLE + LoRa processing
 *   - power_task (1 Hz): power management update + telemetry
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* FreeRTOS (simplified — real build uses nRF Connect SDK / Zephyr) */
#include "board.h"
#include "registers.h"
#include "glyph_engine.h"
#include "thermal_pid.h"
#include "imu_gesture.h"
#include "ble_service.h"
#include "lora_link.h"
#include "power_mgmt.h"

/* ------------------------------------------------------------------------- */
/* FreeRTOS stubs (simplified for standalone compilation) */
/* ------------------------------------------------------------------------- */

/* In real build, these come from FreeRTOS. Here we define minimal stubs. */
typedef void (*task_func_t)(void *);
typedef struct {
    task_func_t func;
    void *arg;
    const char *name;
    uint32_t stack_size;
    uint32_t priority;
    bool running;
} rtos_task_t;

static rtos_task_t tasks[8];
static uint8_t task_count = 0;
static volatile uint32_t rtos_tick_ms = 0;

static void rtos_init(void) { task_count = 0; }

static bool rtos_create_task(task_func_t func, void *arg, const char *name,
                              uint32_t stack, uint32_t prio)
{
    if (task_count >= 8) return false;
    tasks[task_count].func = func;
    tasks[task_count].arg = arg;
    tasks[task_count].name = name;
    tasks[task_count].stack_size = stack;
    tasks[task_count].priority = prio;
    tasks[task_count].running = true;
    task_count++;
    return true;
}

/* Cooperative scheduler: each task is called in round-robin from main loop.
 * Real FreeRTOS uses preemptive scheduling. */
static void rtos_run(void)
{
    /* In a real FreeRTOS system, vTaskStartScheduler() is called here.
     * For this simulation, tasks are called directly from the main loop. */
}

static void rtos_delay_ms(uint32_t ms)
{
    uint32_t start = rtos_tick_ms;
    while ((rtos_tick_ms - start) < ms) {
        /* busy-wait (in real RTOS, this yields) */
    }
}

/* Semaphore / queue stubs */
typedef volatile uint8_t rtos_semaphore_t;
static void sem_give(rtos_semaphore_t *s) { *s = 1; }
static bool sem_take(rtos_semaphore_t *s, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (*s) { *s = 0; return true; }
    return false;
}

/* ------------------------------------------------------------------------- */
/* Shared state between tasks */
/* ------------------------------------------------------------------------- */
static thermal_frame_t latest_frame;  /* produced by glyph_task, consumed by thermal_task */
static rtos_semaphore_t frame_ready_sem = 0;
static volatile uint8_t last_gesture = GESTURE_NONE;
static volatile bool glyph_cmd_pending = false;
static glyph_cmd_t incoming_glyph_cmd;

/* ------------------------------------------------------------------------- */
/* Task: thermal_task — 200 Hz PID loop */
/* ------------------------------------------------------------------------- */

static void thermal_task(void *arg)
{
    (void)arg;
    thermal_init();

    uint32_t last_tick = 0;
    const uint32_t period_ms = 1000U / PID_RATE_HZ;  /* 5 ms */

    while (1) {
        /* Wait for frame from glyph_task (or use last frame) */
        thermal_frame_t frame;
        if (sem_take(&frame_ready_sem, 0)) {
            frame = latest_frame;
        } else {
            /* Use last known frame */
            frame = latest_frame;
        }

        /* Run PID step */
        bool ok = thermal_pid_step(&frame);
        if (!ok) {
            /* Safety fault! */
            /* In real system, trigger alert via BLE + LoRa */
        }

        /* Timing: wait for next 5 ms period */
        while ((rtos_tick_ms - last_tick) < period_ms) {}
        last_tick = rtos_tick_ms;
    }
}

/* ------------------------------------------------------------------------- */
/* Task: glyph_task — 50 Hz glyph rendering */
/* ------------------------------------------------------------------------- */

static void glyph_task(void *arg)
{
    (void)arg;
    glyph_engine_init();

    uint32_t last_tick = 0;
    const uint32_t period_ms = 1000U / GLYPH_FRAME_HZ;  /* 20 ms */

    while (1) {
        /* Check for incoming glyph commands (from BLE or LoRa) */
        if (glyph_cmd_pending) {
            glyph_engine_queue(&incoming_glyph_cmd);
            glyph_cmd_pending = false;
        }

        /* Render next frame */
        thermal_frame_t frame;
        glyph_engine_render(&frame);

        /* Publish frame for thermal_task */
        latest_frame = frame;
        sem_give(&frame_ready_sem);

        /* Timing */
        while ((rtos_tick_ms - last_tick) < period_ms) {}
        last_tick = rtos_tick_ms;
    }
}

/* ------------------------------------------------------------------------- */
/* Task: imu_task — 200 Hz IMU sampling + gesture detection */
/* ------------------------------------------------------------------------- */

static void imu_task(void *arg)
{
    (void)arg;
    if (!imu_init()) {
        /* IMU init failed — continue without gesture support */
        while (1) { rtos_delay_ms(1000); }
    }

    uint32_t last_tick = 0;
    const uint32_t period_ms = 5;  /* 200 Hz */

    while (1) {
        imu_sample_t sample;
        if (imu_read(&sample)) {
            uint8_t gesture = imu_detect_gesture(&sample);
            if (gesture != GESTURE_NONE) {
                last_gesture = gesture;

                /* Notify via BLE */
                ble_notify_gesture(gesture);

                /* Handle gesture actions:
                 * TAP = acknowledge/clear current glyph
                 * DOUBLE_TAP = repeat last glyph
                 * FLIP = snooze (clear and suppress for 10s)
                 * SHAKE = emergency SOS */
                switch (gesture) {
                case GESTURE_TAP:
                    glyph_engine_clear();
                    break;
                case GESTURE_DOUBLE_TAP:
                    {
                        const glyph_cmd_t *curr = glyph_engine_current();
                        if (curr) {
                            glyph_cmd_t repeat = *curr;
                            glyph_engine_queue(&repeat);
                        }
                    }
                    break;
                case GESTURE_FLIP:
                    glyph_engine_clear();
                    break;
                case GESTURE_SHAKE:
                    /* Send SOS via LoRa */
                    lora_send_sos(0x01, 0, 0);
                    break;
                }
            }
        }

        /* Timing */
        while ((rtos_tick_ms - last_tick) < period_ms) {}
        last_tick = rtos_tick_ms;
    }
}

/* ------------------------------------------------------------------------- */
/* Task: comm_task — BLE + LoRA communication */
/* ------------------------------------------------------------------------- */

static void comm_task(void *arg)
{
    (void)arg;
    ble_init();
    lora_init();

    /* Register glyph callback for immediate BLE command handling */
    ble_set_glyph_callback(NULL);  /* we poll instead */

    while (1) {
        /* Process BLE events */
        ble_process();

        /* Check for BLE glyph commands */
        glyph_cmd_t ble_cmd;
        if (ble_get_glyph_cmd(&ble_cmd)) {
            glyph_cmd_pending = true;
            incoming_glyph_cmd = ble_cmd;
        }

        /* Process LoRa events */
        glyph_cmd_t lora_cmd;
        uint8_t sender_id;
        if (lora_process(&lora_cmd, &sender_id)) {
            glyph_cmd_pending = true;
            incoming_glyph_cmd = lora_cmd;
        }

        /* Small delay to avoid busy-looping */
        rtos_delay_ms(10);
    }
}

/* ------------------------------------------------------------------------- */
/* Task: power_task — 1 Hz power management + telemetry */
/* ------------------------------------------------------------------------- */

static void power_task(void *arg)
{
    (void)arg;
    power_init();

    uint32_t last_tick = 0;

    while (1) {
        /* Update power state */
        power_update();

        /* Build and send telemetry (if BLE connected) */
        telemetry_packet_t tel;
        memset(&tel, 0, sizeof(tel));

        uint8_t batt_pct;
        uint16_t sol_mv;
        uint8_t pwr_state;
        power_fill_telemetry(&batt_pct, &sol_mv, &pwr_state);

        tel.battery_pct = batt_pct;
        tel.solar_mv_lo = (uint8_t)(sol_mv & 0xFF);
        tel.solar_mv_hi = (uint8_t)(sol_mv >> 8);
        tel.state = pwr_state;

        /* Get thermal stats */
        int16_t avg_temp, max_temp;
        thermal_get_stats(&avg_temp, &max_temp);
        tel.skin_temp_avg_c = (uint8_t)(avg_temp / 100);
        tel.skin_temp_max_c = (uint8_t)(max_temp / 100);

        /* Get ambient temp from Si7051 (would read here) */
        tel.ambient_temp_c = 25;  /* placeholder */

        /* Current glyph command */
        const glyph_cmd_t *curr = glyph_engine_current();
        tel.current_glyph_cmd = curr ? curr->cmd : 0;

        /* Last gesture */
        tel.gesture_last = last_gesture;

        /* LoRa RSSI */
        tel.lora_rssi = (uint8_t)(lora_get_last_rssi() + 128);

        /* Send telemetry via BLE */
        ble_notify_telemetry(&tel);

        /* Timing: 1 Hz */
        while ((rtos_tick_ms - last_tick) < 1000) {}
        last_tick = rtos_tick_ms;
    }
}

/* ------------------------------------------------------------------------- */
/* System tick (simulated SysTick) */
/* ------------------------------------------------------------------------- */

static void systick_init(void)
{
    /* In real nRF5340, configure SysTick for 1 kHz tick.
     * Here we use a simulated counter. */
    rtos_tick_ms = 0;
}

/* ------------------------------------------------------------------------- */
/* Main entry point */
/* ------------------------------------------------------------------------- */

int main(void)
{
    /* Initialize system */
    systick_init();
    rtos_init();

    /* Create FreeRTOS tasks */
    rtos_create_task(thermal_task, NULL, "thermal",
                     THERMAL_STACK_BYTES, THERMAL_TASK_PRIO);
    rtos_create_task(glyph_task, NULL, "glyph",
                     GLYPH_STACK_BYTES, GLYPH_TASK_PRIO);
    rtos_create_task(imu_task, NULL, "imu",
                     IMU_STACK_BYTES, IMU_TASK_PRIO);
    rtos_create_task(comm_task, NULL, "comm",
                     COMM_STACK_BYTES, COMM_TASK_PRIO);
    rtos_create_task(power_task, NULL, "power",
                     1024, 1);

    /* Start scheduler.
     * In real FreeRTOS: vTaskStartScheduler().
     * Here: run cooperative scheduler from main loop. */

    /* Main super-loop (cooperative scheduler) */
    uint32_t last_thermal = 0;
    uint32_t last_glyph = 0;
    uint32_t last_imu = 0;
    uint32_t last_comm = 0;
    uint32_t last_power = 0;

    /* Run all tasks in round-robin. In real RTOS, preemptive scheduling
     * handles this. This loop demonstrates the task interactions. */
    while (1) {
        rtos_tick_ms++;  /* simulated 1 ms tick */

        /* Thermal task: every 5 ms (200 Hz) */
        if ((rtos_tick_ms - last_thermal) >= 5) {
            thermal_frame_t frame;
            if (sem_take(&frame_ready_sem, 0)) {
                frame = latest_frame;
            } else {
                memset(&frame, 0, sizeof(frame));
            }
            thermal_pid_step(&frame);
            last_thermal = rtos_tick_ms;
        }

        /* Glyph task: every 20 ms (50 Hz) */
        if ((rtos_tick_ms - last_glyph) >= 20) {
            if (glyph_cmd_pending) {
                glyph_engine_queue(&incoming_glyph_cmd);
                glyph_cmd_pending = false;
            }
            glyph_engine_render(&latest_frame);
            sem_give(&frame_ready_sem);
            last_glyph = rtos_tick_ms;
        }

        /* IMU task: every 5 ms (200 Hz) */
        if ((rtos_tick_ms - last_imu) >= 5) {
            imu_sample_t sample;
            if (imu_read(&sample)) {
                uint8_t g = imu_detect_gesture(&sample);
                if (g != GESTURE_NONE) {
                    last_gesture = g;
                    ble_notify_gesture(g);
                    switch (g) {
                    case GESTURE_TAP:
                        glyph_engine_clear();
                        break;
                    case GESTURE_SHAKE:
                        lora_send_sos(0x01, 0, 0);
                        break;
                    }
                }
            }
            last_imu = rtos_tick_ms;
        }

        /* Comm task: every 10 ms */
        if ((rtos_tick_ms - last_comm) >= 10) {
            ble_process();

            glyph_cmd_t cmd;
            if (ble_get_glyph_cmd(&cmd)) {
                glyph_cmd_pending = true;
                incoming_glyph_cmd = cmd;
            }

            glyph_cmd_t lora_cmd;
            uint8_t sender;
            if (lora_process(&lora_cmd, &sender)) {
                glyph_cmd_pending = true;
                incoming_glyph_cmd = lora_cmd;
            }

            last_comm = rtos_tick_ms;
        }

        /* Power task: every 1000 ms (1 Hz) */
        if ((rtos_tick_ms - last_power) >= 1000) {
            power_update();
            last_power = rtos_tick_ms;
        }
    }

    return 0;  /* never reached */
}