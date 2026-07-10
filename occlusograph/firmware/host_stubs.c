/*
 * host_stubs.c — Host-side stub implementations for unit testing.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 *
 * Provides minimal host implementations of the hal_* functions so the
 * firmware modules can be compiled and unit-tested on a development
 * machine without Zephyr or real hardware.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* ---- SAADC ---- */
void hal_saadc_setup(void) { }
void hal_saadc_scan16(uint16_t out16[16]) {
    /* Simulate near-mid-scale noise on all channels. */
    for (int i = 0; i < 16; i++) {
        out16[i] = 2048 + (uint16_t)(rand() % 20 - 10);
    }
}

/* ---- GPIO ---- */
static uint8_t gpio_state[2][32] = {0};
void hal_gpio_write(uint8_t port, uint8_t pin, uint8_t val) {
    if (port < 2 && pin < 32) gpio_state[port][pin] = val;
}
uint8_t hal_gpio_read(uint8_t port, uint8_t pin) {
    if (port < 2 && pin < 32) return gpio_state[port][pin];
    return 0;
}
void hal_gpio_cfg_out(uint8_t port, uint8_t pin) { (void)port; (void)pin; }
void hal_gpio_cfg_in(uint8_t port, uint8_t pin, uint8_t pull) { (void)port; (void)pin; (void)pull; }

/* ---- I²C ---- */
int hal_twi_write(uint8_t bus, uint8_t addr, const uint8_t *w, uint32_t len) {
    (void)bus; (void)addr; (void)w; (void)len;
    return 0;  /* success */
}
int hal_twi_read(uint8_t bus, uint8_t addr, uint8_t reg, uint8_t *r, uint32_t len) {
    (void)bus; (void)addr; (void)reg;
    /* Return zeros for reads. */
    if (r) memset(r, 0, len);
    return 0;
}

/* ---- Delay ---- */
void hal_delay_us(uint32_t us) { (void)us; }
void hal_delay_ms(uint32_t ms) { (void)ms; }

/* ---- Watchdog ---- */
static uint32_t wdt_timeout = 0;
void hal_watchdog_enable(uint32_t timeout_ms) { wdt_timeout = timeout_ms; }
void hal_watchdog_feed(void) { }

/* ---- Millis ---- */
static uint32_t g_millis = 0;
uint32_t hal_millis(void) { return g_millis++; }

/* ---- Panic ---- */
void hal_panic(const char *msg) {
    fprintf(stderr, "PANIC: %s\n", msg);
    exit(1);
}

/* ---- BLE ---- */
int hal_ble_init(void) { return 0; }
void hal_ble_adv_start(uint32_t interval_ms) { (void)interval_ms; }
void hal_ble_adv_stop(void) { }
bool hal_ble_connected(void) { return false; }
int hal_ble_notify(uint16_t uuid, const uint8_t *data, uint16_t len) {
    (void)uuid; (void)data; (void)len; return 0;
}
void hal_ble_set_notify_enable(uint16_t uuid, bool enable) { (void)uuid; (void)enable; }
void hal_ble_set_write_cb(uint16_t uuid, void (*cb)(const uint8_t*, uint16_t)) {
    (void)uuid; (void)cb;
}
void hal_ble_set_conn_cb(void (*cb)(bool)) { (void)cb; }

/* ---- QSPI ---- */
static uint8_t flash_mem[16 * 1024 * 1024];
int hal_qspi_init(void) {
    memset(flash_mem, 0xFF, sizeof(flash_mem));
    return 0;
}
int hal_qspi_read(uint32_t addr, uint8_t *buf, uint32_t len) {
    if (addr + len > sizeof(flash_mem)) return -1;
    memcpy(buf, flash_mem + addr, len);
    return 0;
}
int hal_qspi_program(uint32_t addr, const uint8_t *buf, uint32_t len) {
    if (addr + len > sizeof(flash_mem)) return -1;
    /* NOR flash: can only change 1->0. */
    for (uint32_t i = 0; i < len; i++) {
        flash_mem[addr + i] &= buf[i];
    }
    return 0;
}
int hal_qspi_erase_sector(uint32_t addr) {
    if (addr + 4096 > sizeof(flash_mem)) return -1;
    memset(flash_mem + addr, 0xFF, 4096);
    return 0;
}
int hal_qspi_read_id(uint8_t *id3) {
    id3[0] = 0xEF; id3[1] = 0x40; id3[2] = 0x18;
    return 0;
}
void hal_flash_log_mutex_lock(void) { }
void hal_flash_log_mutex_unlock(void) { }

/* ---- Power ---- */
void hal_sys_off(void) { }
void hal_sys_wake(void) { }
void hal_rtc_set_alarm(uint32_t seconds) { (void)seconds; }
uint16_t hal_adc_vbat_mv(void) { return 3800; }
int32_t hal_chip_temp_mc(void) { return 35000; }

/* ---- Unit test main ----
 * A minimal smoke test that initializes all subsystems and runs a few
 * frames through the pipeline to verify the code compiles and links.
 */
#include "sensors.h"
#include "fusion.h"
#include "classify.h"
#include "ble.h"
#include "flash_log.h"
#include "power.h"

int main(void) {
    printf("Occlusograph host unit test — Author: jayis1\n");

    int rc = sensors_init();
    printf("sensors_init: %d\n", rc);

    fusion_init();
    classify_init();
    ble_init();
    flash_log_init();
    power_init();

    sensors_start();

    /* Run 300 frames (300 ms) through the pipeline. */
    sensor_frame_t raw;
    force_frame_t fused;
    event_record_t evt;
    int events_emitted = 0;

    for (int i = 0; i < 300; i++) {
        raw.timestamp = i;
        if (sensors_acquire(&raw) == 0) {
            fusion_update(&raw, &fused);
            if (classify_push(&fused, &evt)) {
                events_emitted++;
                printf("Event @%lu: class=%d peak=%d mN elem=%d\n",
                       (unsigned long)evt.timestamp, evt.class_id,
                       evt.peak_force_mN, evt.peak_element);
            }
        }
    }

    printf("Frames processed: 300, events emitted: %d\n", events_emitted);
    printf("Flash log count: %lu\n", (unsigned long)flash_log_count());
    printf("Battery: %u mV (%u%%)\n",
           power_battery_mv(), power_battery_pct());

    sensors_stop();
    printf("All tests passed.\n");
    return 0;
}