/*
 * ble_pen.c — BLE 5.0 GATT + HID Pen service for Inkwell
 *
 * Inkwell advertises two services:
 *
 *  1. Custom Inkwell Stroke Service (UUID 1B7E0001-...) with characteristics
 *     for Stroke Data (notify), Control (write), Status (read/notify), and
 *     Journal Replay (write/notify). This is the primary app-channel.
 *
 *  2. HID over GATT (HOGP) with a Digitizer-Pen report map so that
 *     platforms with native pen support (iPadOS, Windows, Android) receive
 *     strokes as system-level stylus input with pressure, without any
 *     custom app.
 *
 * The nRF52833 SoftDevice would normally provide the GATT primitives; this
 * file is structured to compile against a minimal shim so the tree builds
 * standalone, while the real implementation hooks sd_* calls.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "ble_pen.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ---- UUIDs (128-bit, abbreviated for readability) ---- */
static const uint8_t UUID_INKWELL_SERVICE[16] = {
    0x01,0x00,0x7E,0x1B, 0x00,0x00, 0x00,0x00,
    0x00,0x00,0x00,0x00, 0x00,0x00, 0x00,0x00
};
static const uint8_t UUID_STROKE_DATA[16]    = { 0x02,0x00,0x7E,0x1B,0 };
static const uint8_t UUID_CONTROL[16]        = { 0x03,0x00,0x7E,0x1B,0 };
static const uint8_t UUID_STATUS[16]         = { 0x04,0x00,0x7E,0x1B,0 };
static const uint8_t UUID_JOURNAL_REPLAY[16] = { 0x05,0x00,0x7E,0x1B,0 };

/* HID service UUIDs (standard SIG) */
#define UUID_HID_SERVICE      0x1812
#define UUID_HID_REPORT       0x2A4D
#define UUID_HID_REPORT_MAP   0x2A4B
#define UUID_HID_INFORMATION  0x2A4A
#define UUID_HID_CTRL_POINT   0x2A4E

static bool g_connected = false;
static uint16_t g_conn_handle = 0xFFFF;
static bool g_session_active = false;
static uint16_t g_segment_rate_ms = BLE_SEGMENT_PERIOD_MS;

/* ---- Minimal SoftDevice shim (real build links libsoftdevice.a) ---- */
static void sd_ble_gap_adv_start(void) { /* stub */ }
static void sd_ble_gatts_notification_send(uint16_t handle,
                                           const uint8_t *data, uint16_t len)
{ (void)handle; (void)data; (void)len; }

/* HID Pen report descriptor (Digitizer page, Pen usage).
 * Sends: tip pressure (16-bit), in-range (1 bit), tip switch (1 bit),
 * x/y as relative deltas (int16). */
static const uint8_t HID_REPORT_MAP[] = {
    0x05, 0x0D,        /* Usage Page (Digitizers) */
    0x09, 0x01,        /* Usage (Digitizer) */
    0xA1, 0x01,        /* Collection (Application) */
    0x85, 0x01,        /*   Report ID (1) */
    0x09, 0x20,        /*   Usage (Stylus) */
    0xA1, 0x00,        /*   Collection (Physical) */
    0x09, 0x42,        /*     Usage (Tip Switch) */
    0x15, 0x00,        /*     Logical Min (0) */
    0x25, 0x01,        /*     Logical Max (1) */
    0x75, 0x01,        /*     Report Size (1) */
    0x95, 0x01,        /*     Report Count (1) */
    0x81, 0x02,        /*     Input (Data,Var,Abs) */
    0x09, 0x32,        /*     Usage (In Range) */
    0x81, 0x02,        /*     Input (Data,Var,Abs) */
    0x75, 0x06,        /*     Report Size (6) -- padding */
    0x95, 0x01,        /*     Report Count (1) */
    0x81, 0x03,        /*     Input (Const,Var,Abs) */
    0x05, 0x01,        /*     Usage Page (Generic Desktop) */
    0x09, 0x30,        /*     Usage (X) */
    0x09, 0x31,        /*     Usage (Y) */
    0x15, 0x81,        /*     Logical Min (-127) */
    0x25, 0x7F,        /*     Logical Max (127) */
    0x75, 0x08,        /*     Report Size (8) */
    0x95, 0x02,        /*     Report Count (2) */
    0x81, 0x06,        /*     Input (Data,Var,Rel) */
    0x05, 0x0D,        /*     Usage Page (Digitizers) */
    0x09, 0x30,        /*     Usage (Tip Pressure) */
    0x15, 0x00,        /*     Logical Min (0) */
    0x26, 0xFF, 0x7F,  /*     Logical Max (32767) */
    0x75, 0x10,        /*     Report Size (16) */
    0x95, 0x01,        /*     Report Count (1) */
    0x81, 0x02,        /*     Input (Data,Var,Abs) */
    0xC0,              /*   End Collection */
    0xC0               /* End Collection */
};

void ble_pen_init(void)
{
    /* Real build: sd_ble_enable(), add service, add characteristics, set
     * advertising data (name "Inkwell" + UUID list + service solicitation),
     * configure connection parameters. Here we just start advertising. */
    sd_ble_gap_adv_start();
}

/* Serialize a stroke segment into the 20-byte notification payload. */
static void serialize_segment(const stroke_segment_t *seg, uint8_t *out)
{
    out[0]  = seg->flags;
    out[1]  = (uint8_t)(seg->seq & 0xFF);
    out[2]  = (uint8_t)((seg->seq >> 8) & 0xFF);
    out[3]  = (uint8_t)((seg->seq >> 16) & 0xFF);
    out[4]  = (uint8_t)((seg->seq >> 24) & 0xFF);
    out[5]  = (uint8_t)(seg->ts_ms & 0xFF);
    out[6]  = (uint8_t)((seg->ts_ms >> 8) & 0xFF);
    out[7]  = (uint8_t)((seg->ts_ms >> 16) & 0xFF);
    out[8]  = (uint8_t)((seg->ts_ms >> 24) & 0xFF);
    out[9]  = (uint8_t)(seg->dx_um & 0xFF);
    out[10] = (uint8_t)((seg->dx_um >> 8) & 0xFF);
    out[11] = (uint8_t)((seg->dx_um >> 16) & 0xFF);
    out[12] = (uint8_t)((seg->dx_um >> 24) & 0xFF);
    out[13] = (uint8_t)(seg->dy_um & 0xFF);
    out[14] = (uint8_t)((seg->dy_um >> 8) & 0xFF);
    out[15] = (uint8_t)((seg->dy_um >> 16) & 0xFF);
    out[16] = (uint8_t)((seg->dy_um >> 24) & 0xFF);
    out[17] = (uint8_t)(seg->p_mN & 0xFF);
    out[18] = (uint8_t)((seg->p_mN >> 8) & 0xFF);
    out[19] = seg->crc8;
}

void ble_pen_notify_segment(const stroke_segment_t *seg)
{
    if (!g_connected) return;
    uint8_t payload[20];
    serialize_segment(seg, payload);
    sd_ble_gatts_notification_send(g_conn_handle, payload, sizeof(payload));

    /* Also emit an HID Pen report for OS-native pen input. */
    uint8_t hid_report[6];
    hid_report[0] = 0x01;  /* report ID */
    hid_report[1] = (seg->flags & 0x01) ? 0x03 : 0x02;  /* tip+in-range or in-range */
    /* Clamp deltas to int8 range for the HID report. */
    int8_t hdx = (int8_t)CLAMP(seg->dx_um / 100, -127, 127);
    int8_t hy  = (int8_t)CLAMP(seg->dy_um / 100, -127, 127);
    hid_report[2] = (uint8_t)hdx;
    hid_report[3] = (uint8_t)hy;
    hid_report[4] = (uint8_t)(seg->p_mN & 0xFF);
    hid_report[5] = (uint8_t)((seg->p_mN >> 8) & 0x7F);
    sd_ble_gatts_notification_send(g_conn_handle, hid_report, sizeof(hid_report));
}

void ble_pen_notify_status(uint8_t battery_pct, uint8_t power_state, uint8_t flash_pct)
{
    if (!g_connected) return;
    uint8_t status[3] = { battery_pct, power_state, flash_pct };
    sd_ble_gatts_notification_send(g_conn_handle, status, sizeof(status));
}

bool ble_pen_is_connected(void) { return g_connected; }

void ble_pen_start_session(void) { g_session_active = true; }
void ble_pen_stop_session(void)  { g_session_active = false; }
void ble_pen_set_segment_rate(uint16_t ms) { g_segment_rate_ms = ms; }