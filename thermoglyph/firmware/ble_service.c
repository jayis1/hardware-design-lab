/*
 * ble_service.c — BLE 5.4 GATT service implementation
 *
 * Custom GATT service "TG01" with 5 characteristics:
 *   - Glyph command (write)
 *   - Gesture event (notify)
 *   - Telemetry (notify)
 *   - Configuration (write)
 *   - OTA DFU (write)
 *
 * Uses nRF5340 net core's SoftDevice via IPC. This file runs on the app core
 * and communicates with the net core through IPC channels.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#include "ble_service.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ------------------------------------------------------------------------- */
/* IPC to net core (simplified) */
/* ------------------------------------------------------------------------- */
/* The nRF5340 uses IPC (Inter-Process Communication) between app and net
 * cores. We use a simple shared-memory ring buffer for BLE events.
 * In the real implementation, this uses the nRF Connect SDK's
 * ipc_service library. Here we simulate the interface. */

#define IPC_BUF_SIZE    128
#define IPC_EVENT_COUNT 8

typedef struct {
    uint8_t data[IPC_BUF_SIZE];
    uint8_t len;
    uint8_t type;   /* 0=glyph_cmd, 1=gesture_notify, 2=telemetry_notify,
                       3=config_write, 4=dfu_write, 5=connected, 6=disconnected */
} ipc_event_t;

static ipc_event_t ipc_events[IPC_EVENT_COUNT];
static volatile uint8_t ipc_head = 0;
static volatile uint8_t ipc_tail = 0;

static bool ipc_send(const uint8_t *data, uint8_t len, uint8_t type)
{
    uint8_t next = (ipc_head + 1) % IPC_EVENT_COUNT;
    if (next == ipc_tail) return false;  /* full */
    ipc_events[ipc_head].len = len;
    ipc_events[ipc_head].type = type;
    if (len > IPC_BUF_SIZE) len = IPC_BUF_SIZE;
    memcpy(ipc_events[ipc_head].data, data, len);
    ipc_head = next;
    return true;
}

static bool ipc_recv(ipc_event_t *evt)
{
    if (ipc_head == ipc_tail) return false;  /* empty */
    *evt = ipc_events[ipc_tail];
    ipc_tail = (ipc_tail + 1) % IPC_EVENT_COUNT;
    return true;
}

/* ------------------------------------------------------------------------- */
/* State */
/* ------------------------------------------------------------------------- */
static bool ble_connected = false;
static glyph_cmd_t pending_glyph;
static bool glyph_pending = false;
static void (*glyph_callback)(const glyph_cmd_t *cmd) = NULL;

/* ------------------------------------------------------------------------- */
/* GATT service definition (conceptual — net core handles actual att_db) */
/* ------------------------------------------------------------------------- */

/* The GATT service layout:
 *
 * Service: 0x180T (custom "TG01" → 0x54 0x47 0x01 0x00)
 *
 *   Characteristic 1: Glyph Command (Write)
 *     UUID: 0x54 0x47 0x01 0x01
 *     Value: binary glyph_cmd_t packet
 *
 *   Characteristic 2: Gesture Event (Notify)
 *     UUID: 0x54 0x47 0x02 0x01
 *     Value: 1-byte gesture code
 *
 *   Characteristic 3: Telemetry (Notify)
 *     UUID: 0x54 0x47 0x03 0x01
 *     Value: telemetry_packet_t (10 bytes)
 *
 *   Characteristic 4: Configuration (Write)
 *     UUID: 0x54 0x47 0x04 0x01
 *     Value: config packet (PID gains, safety, brightness)
 *
 *   Characteristic 5: OTA DFU (Write)
 *     UUID: 0x54 0x47 0x05 0x01
 *     Value: DFU command + firmware chunk
 */

/* ------------------------------------------------------------------------- */
/* Packet parsing */
/* ------------------------------------------------------------------------- */

/* BLE glyph command packet format (binary):
 * [cmd] [polarity] [intensity] [direction/shape] [duration_lo] [duration_hi]
 * [text_len] [text[0..15]] [scroll]
 * Total: 7 + 16 + 1 = 24 bytes (text padded to 16) */

static void parse_glyph_packet(const uint8_t *data, uint8_t len)
{
    if (len < 7) return;

    glyph_cmd_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.cmd = data[0];
    cmd.polarity = data[1];
    cmd.intensity = data[2];
    cmd.direction = data[3];
    cmd.shape = data[3];  /* same byte, depending on cmd type */
    cmd.duration_ms_lo = data[4];
    cmd.duration_ms_hi = data[5];
    cmd.text_len = data[6];

    if (len > 7 && cmd.text_len > 0) {
        uint8_t copy_len = len - 7;
        if (copy_len > 15) copy_len = 15;
        if (copy_len > cmd.text_len) copy_len = cmd.text_len;
        memcpy(cmd.text, &data[7], copy_len);
        cmd.text[copy_len] = '\0';
    }

    cmd.scroll = (len > 23) ? data[23] : 0;

    /* Store as pending for comm_task to pick up */
    pending_glyph = cmd;
    glyph_pending = true;

    /* Also call callback if registered */
    if (glyph_callback) {
        glyph_callback(&cmd);
    }
}

static void parse_config_packet(const uint8_t *data, uint8_t len)
{
    /* Config packet:
     * [0] = config type
     *   0x01: intensity scale  (data[1] = scale 0–255)
     *   0x02: PID gains        (data[1..3] = Kp, Ki, Kd)
     *   0x03: safety limits    (data[1..2] = max temp lo/hi, data[3..4] = min)
     *   0x04: power mode       (data[1] = mode: 0=sleep, 1=idle, 2=active)
     */
    if (len < 2) return;

    switch (data[0]) {
    case 0x01:
        if (len >= 2) {
            glyph_engine_set_intensity_scale(data[1]);
        }
        break;
    case 0x02:
        /* PID gains — would update thermal_pid module's gains.
         * Stored in flash for persistence. */
        break;
    case 0x03:
        /* Safety limits — would update board.h limits at runtime.
         * Restricted to narrower range than compiled defaults. */
        break;
    case 0x04:
        /* Power mode — handled by power_mgmt module */
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------------- */
/* Public API */
/* ------------------------------------------------------------------------- */

bool ble_init(void)
{
    /* In real implementation: initialize SoftDevice on net core,
     * register GATT service, start advertising.
     * Here we just reset state. */
    ble_connected = false;
    glyph_pending = false;
    ipc_head = 0;
    ipc_tail = 0;

    /* Simulate: send "start advertising" command to net core via IPC */
    uint8_t adv_cmd[2] = {0xA0, 0x01};  /* start adv */
    ipc_send(adv_cmd, 2, 0xF0);

    return true;
}

void ble_process(void)
{
    ipc_event_t evt;
    while (ipc_recv(&evt)) {
        switch (evt.type) {
        case 5:  /* connected */
            ble_connected = true;
            break;
        case 6:  /* disconnected */
            ble_connected = false;
            /* Restart advertising */
            {
                uint8_t adv_cmd[2] = {0xA0, 0x01};
                ipc_send(adv_cmd, 2, 0xF0);
            }
            break;
        case 0:  /* glyph command received */
            parse_glyph_packet(evt.data, evt.len);
            break;
        case 3:  /* config write */
            parse_config_packet(evt.data, evt.len);
            break;
        case 4:  /* DFU write */
            /* Forward to DFU module — not implemented in this file */
            break;
        default:
            break;
        }
    }
}

bool ble_notify_gesture(uint8_t gesture_code)
{
    if (!ble_connected) return false;
    uint8_t buf[1] = {gesture_code};
    return ipc_send(buf, 1, 1);  /* type 1 = gesture notify */
}

bool ble_notify_telemetry(const telemetry_packet_t *tel)
{
    if (!ble_connected) return false;
    return ipc_send((const uint8_t *)tel, sizeof(*tel), 2);
}

bool ble_is_connected(void)
{
    return ble_connected;
}

bool ble_get_glyph_cmd(glyph_cmd_t *cmd)
{
    if (!glyph_pending) return false;
    *cmd = pending_glyph;
    glyph_pending = false;
    return true;
}

void ble_set_glyph_callback(void (*cb)(const glyph_cmd_t *cmd))
{
    glyph_callback = cb;
}