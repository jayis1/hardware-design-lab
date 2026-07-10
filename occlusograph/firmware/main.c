/*
 * main.c — Firmware entry point and task scheduler for Occlusograph.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 *
 * The firmware runs a cooperative scheduler with three periodic tasks:
 *
 *   1. Sensor task (1 ms period):  acquire raw frame → fuse → classify.
 *      If the classifier emits an event, forward it to BLE (if connected)
 *      or to the flash logger (if not).
 *
 *   2. BLE task (30 ms period):    flush coalesced notifications, check
 *      connection state, handle calibration writes.
 *
 *   3. Power task (1 s period):    update SoC, check charging, enter sleep
 *      if no activity for >5 minutes and BLE disconnected.
 *
 * The scheduler is a simple fixed-priority cooperative loop — no preemption.
 * This is safe because the sensor task has a hard 1 ms deadline and all other
 * tasks are much shorter than 1 ms. A hardware watchdog catches any overrun.
 *
 * On the nRF5340 target, this maps to Zephyr threads: the sensor task is a
 * high-priority thread with a 1 ms k_timer, and BLE/power are lower-priority
 * threads. The platform shims (hal_*) are implemented in a thin adaptation
 * layer that is linked in for the real target build.
 */

#include "board.h"
#include "registers.h"
#include "sensors.h"
#include "fusion.h"
#include "classify.h"
#include "ble.h"
#include "flash_log.h"
#include "power.h"
#include <string.h>

/* ---- Platform shim ---- */
extern void hal_watchdog_enable(uint32_t timeout_ms);
extern void hal_watchdog_feed(void);
extern uint32_t hal_millis(void);
extern void hal_gpio_write(uint8_t port, uint8_t pin, uint8_t val);
extern void hal_panic(const char *msg);

/* ---- Task timing ---- */
#define SENSOR_TASK_PERIOD_MS   1u
#define BLE_TASK_PERIOD_MS      30u
#define POWER_TASK_PERIOD_MS    1000u

/* ---- State ---- */
static sensor_frame_t g_raw_frame;
static force_frame_t g_fused_frame;
static event_record_t g_event;
static bool g_ble_connected = false;
static uint32_t g_last_activity_ms = 0;
static uint32_t g_next_sensor_run = 0;
static uint32_t g_next_ble_run = 0;
static uint32_t g_next_power_run = 0;

/* ---- Calibration callback ---- */
static void on_calib_write(uint8_t element, int16_t offset_mN)
{
    /* Apply zero-offset to the piezo offset table for the given element. */
    int16_t piezo_off[PIEZO_NUM_ELEMENTS];
    int32_t cap_off[CAP_NUM_ELEMENTS];
    memset(piezo_off, 0, sizeof(piezo_off));
    memset(cap_off, 0, sizeof(cap_off));
    if (element < PIEZO_NUM_ELEMENTS) {
        piezo_off[element] = offset_mN;
    }
    sensors_apply_calibration(piezo_off, cap_off);
}

/* ---- BLE connection callback ---- */
static void on_ble_conn(bool connected)
{
    g_ble_connected = connected;
    if (connected) {
        /* When a phone connects, begin streaming. Also sync any buffered
         * events from flash. */
        uint32_t unsynced = flash_log_unsynced_idx();
        if (unsynced < flash_log_count()) {
            event_record_t buf[8];
            uint32_t n = flash_log_read(unsynced, buf, 8);
            for (uint32_t i = 0; i < n; i++) {
                ble_notify_event(&buf[i]);
            }
        }
    }
    g_last_activity_ms = hal_millis();
}

/* =====================================================================
 * Tasks
 * =====================================================================
 */

/* Sensor task: acquire → fuse → classify → emit. 1 ms deadline. */
static void task_sensor(uint32_t now)
{
    int rc = sensors_acquire(&g_raw_frame);
    if (rc) return;  /* skip this frame on error */

    fusion_update(&g_raw_frame, &g_fused_frame);

    int emitted = classify_push(&g_fused_frame, &g_event);
    if (emitted) {
        g_last_activity_ms = now;
        if (g_ble_connected) {
            ble_notify_event(&g_event);
            /* Also send a force snapshot for live display. */
            ble_notify_force(g_fused_frame.force_mN, g_fused_frame.timestamp);
        } else {
            /* Buffer to flash for later sync. */
            flash_log_append(&g_event);
        }
    }
}

/* BLE task: flush notifications, handle connection state. 30 ms. */
static void task_ble(uint32_t now)
{
    if (g_ble_connected) {
        ble_flush_events();
    }
    /* Check if we should start/stop advertising. */
    if (!g_ble_connected && !ble_is_connected()) {
        /* In idle, advertise slowly. */
        ble_advertise_start();
    }
    (void)now;
}

/* Power task: SoC, charging, sleep management. 1 s. */
static void task_power(uint32_t now)
{
    (void)power_battery_pct();

    /* If no BLE and no activity for 5 minutes, go to sleep. */
    if (!g_ble_connected && (now - g_last_activity_ms) > 300000u) {
        /* Schedule a wake in 60 s for a quick bruxism scan, then sleep. */
        power_set_wake_timer(60u);
        power_sleep();
        /* On wake: */
        power_wake();
        g_last_activity_ms = now;
    }

    /* Thermal protection: if chip > 70°C, stop charging. */
    int32_t t = power_chip_temp_mc();
    if (t > 70000 && power_is_charging()) {
        uint8_t stop = 0x00;
        /* Disable charging via PMIC. */
        extern int hal_twi_write(uint8_t, uint8_t, const uint8_t*, uint32_t);
        hal_twi_write(0, LTC4124_ADDR, &stop, 1);
    }
}

/* =====================================================================
 * System initialization
 * =====================================================================
 */

static int system_init(void)
{
    int rc;

    hal_watchdog_enable(WDT_TIMEOUT_MS);

    rc = sensors_init();
    if (rc) { hal_panic("sensors_init failed"); return rc; }

    fusion_init();
    classify_init();

    rc = ble_init();
    if (rc) { hal_panic("ble_init failed"); return rc; }

    rc = flash_log_init();
    if (rc) { hal_panic("flash_log_init failed"); return rc; }

    rc = power_init();
    if (rc) { hal_panic("power_init failed"); return rc; }

    /* Register callbacks. */
    ble_set_conn_callback(on_ble_conn);
    ble_set_calib_callback(on_calib_write);

    /* Start acquiring + advertising. */
    sensors_start();
    ble_advertise_start();

    g_last_activity_ms = hal_millis();
    g_next_sensor_run = g_last_activity_ms;
    g_next_ble_run = g_last_activity_ms;
    g_next_power_run = g_last_activity_ms;

    return 0;
}

/* =====================================================================
 * Main loop
 * =====================================================================
 */

#ifndef HOST_BUILD
int main(void)
{
    int rc = system_init();
    if (rc) {
        /* If init fails, blink the LED and loop forever so the watchdog
         * resets us — a simple fault indicator for the patient. */
        while (1) {
            hal_gpio_write(LED_STATUS_PORT, LED_STATUS_PIN, 1);
            for (volatile int i = 0; i < 200000; i++);
            hal_gpio_write(LED_STATUS_PORT, LED_STATUS_PIN, 0);
            for (volatile int i = 0; i < 200000; i++);
            hal_watchdog_feed();
        }
    }

    /* Cooperative scheduler. */
    while (1) {
        uint32_t now = hal_millis();

        if ((int32_t)(now - g_next_sensor_run) >= 0) {
            task_sensor(now);
            g_next_sensor_run += SENSOR_TASK_PERIOD_MS;
            hal_watchdog_feed();
        }

        if ((int32_t)(now - g_next_ble_run) >= 0) {
            task_ble(now);
            g_next_ble_run += BLE_TASK_PERIOD_MS;
        }

        if ((int32_t)(now - g_next_power_run) >= 0) {
            task_power(now);
            g_next_power_run += POWER_TASK_PERIOD_MS;
        }

        /* Idle: if nothing is due, yield to save power. On the real target
         * this is a WFE (wait-for-event) instruction that wakes on the next
         * interrupt (SAADC done, BLE event, or RTC). */
#ifdef __arm__
        __asm volatile ("wfe");
#else
        /* On host, just loop tight; the watchdog is not active. */
        for (volatile int i = 0; i < 100; i++);
#endif
    }

    return 0;  /* unreachable */
}
#endif /* !HOST_BUILD */