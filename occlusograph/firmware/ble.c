/*
 * ble.c — BLE GATT server for Occlusograph.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 *
 * Implements a custom GATT profile with three services (Device Info, Occlusal
 * Data, Event Log) and a Calibration helper. Notification coalescing batches
 * multiple event records into a single notification to reduce radio wake-ups.
 *
 * The implementation is written against a minimal platform shim (hal_ble_*)
 * so it can be compiled on a host for unit testing. On the nRF5340 target,
 * these shims map to the Zephyr Bluetooth host + controller (net core).
 */

#include "ble.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- Platform shim (implemented by the Zephyr backend on target) ---- */
extern int  hal_ble_init(void);
extern void hal_ble_adv_start(uint32_t interval_ms);
extern void hal_ble_adv_stop(void);
extern bool hal_ble_connected(void);
extern int  hal_ble_notify(uint16_t uuid, const uint8_t *data, uint16_t len);
extern void hal_ble_set_notify_enable(uint16_t uuid, bool enable);
extern void hal_ble_set_write_cb(uint16_t uuid, void (*cb)(const uint8_t*, uint16_t));
extern void hal_ble_set_conn_cb(void (*cb)(bool));

/* ---- Module state ---- */
static bool g_initialized = false;
static ble_conn_cb_t g_conn_cb = NULL;
static ble_calib_cb_t g_calib_cb = NULL;

/* Notification coalescing buffer: up to 4 event records per notification. */
#define EVENTS_PER_NOTIF 4u
static event_record_t g_notif_buf[EVENTS_PER_NOTIF];
static uint8_t g_notif_count = 0;

/* ---- Connection callback trampoline ---- */
static void conn_trampoline(bool connected)
{
    if (g_conn_cb) g_conn_cb(connected);
    /* On disconnect, flush any pending coalesced events. */
    if (!connected && g_notif_count > 0) {
        ble_flush_events();
    }
}

/* ---- Calibration write handler ---- */
static void calib_write_handler(const uint8_t *data, uint16_t len)
{
    if (!data || len < 3) return;
    uint8_t element = data[0];
    int16_t offset = (int16_t)((data[1] << 8) | data[2]);
    if (g_calib_cb) g_calib_cb(element, offset);
}

/* ---- Public API ---- */

int ble_init(void)
{
    int rc = hal_ble_init();
    if (rc) return rc;

    /* Register write callback for the calibration characteristic. */
    hal_ble_set_write_cb(UUID_CHR_CALIBRATION, calib_write_handler);
    hal_ble_set_conn_cb(conn_trampoline);

    g_initialized = true;
    return 0;
}

void ble_advertise_start(void)
{
    if (!g_initialized) return;
    hal_ble_adv_start(1000);  /* 1 s interval in idle */
}

void ble_advertise_stop(void)
{
    if (!g_initialized) return;
    hal_ble_adv_stop();
}

bool ble_is_connected(void)
{
    return hal_ble_connected();
}

void ble_set_conn_callback(ble_conn_cb_t cb)
{
    g_conn_cb = cb;
}

void ble_set_calib_callback(ble_calib_cb_t cb)
{
    g_calib_cb = cb;
}

void ble_notify_event(const event_record_t *evt)
{
    if (!evt || !g_initialized) return;

    /* Coalesce events: buffer up to EVENTS_PER_NOTIF before sending. */
    g_notif_buf[g_notif_count++] = *evt;
    if (g_notif_count >= EVENTS_PER_NOTIF) {
        ble_flush_events();
    }
}

void ble_flush_events(void)
{
    if (g_notif_count == 0) return;
    /* Pack records into a byte array: each record is 14 bytes. */
    uint8_t buf[EVENTS_PER_NOTIF * sizeof(event_record_t)];
    uint16_t len = 0;
    for (int i = 0; i < g_notif_count; i++) {
        memcpy(buf + len, &g_notif_buf[i], sizeof(event_record_t));
        len += sizeof(event_record_t);
    }
    hal_ble_notify(UUID_CHR_EVENTS, buf, len);
    g_notif_count = 0;
}

void ble_notify_force(const int16_t force_mN[64], uint32_t timestamp)
{
    if (!g_initialized || !force_mN) return;
    /* Simple delta compression: send 64 int16 + timestamp.
     * In production we'd apply run-length + Huffman; this is the fallback. */
    uint8_t buf[128 + 4];
    for (int i = 0; i < 64; i++) {
        buf[2*i]     = (uint8_t)(force_mN[i] >> 8);
        buf[2*i + 1] = (uint8_t)(force_mN[i] & 0xFF);
    }
    buf[128] = (uint8_t)(timestamp >> 24);
    buf[129] = (uint8_t)(timestamp >> 16);
    buf[130] = (uint8_t)(timestamp >> 8);
    buf[131] = (uint8_t)(timestamp & 0xFF);
    hal_ble_notify(UUID_CHR_FORCE_STREAM, buf, sizeof(buf));
}