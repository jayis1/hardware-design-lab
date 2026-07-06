/*
 * ble_svc.c — TactiScript BLE GATT service implementation
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Implements a custom BLE 5.0 GATT service for the TactiScript ring.
 * The service exposes 6 characteristics:
 *   1. Text Pipe (Write): stream UTF-8 text for Braille rendering
 *   2. Command (Write): control bytes (mode, speed, intensity, etc.)
 *   3. Status (Notify): battery %, mode, connection state, errors
 *   4. Texture (Write): raw 4×6 frame data for custom haptic patterns
 *   5. Gesture (Notify): detected gesture events for app feedback
 *   6. Nav Vector (Write): navigation direction byte
 *
 * This is a reference implementation using the Nordic SoftDevice API.
 * In a production build, this links against the nRF Connect SDK BLE
 * stack running on the nRF5340 network core via IPC.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ble_svc.h"
#include "../board.h"
#include "../registers.h"

/* ----------------------------------------------------------------
 * Private state
 * ---------------------------------------------------------------- */
static bool s_initialized = false;
static bool s_connected = false;
static uint16_t s_conn_handle = 0xFFFF;

/* GATT handle placeholders (assigned by the stack during service setup) */
typedef struct {
    uint16_t text_handle;
    uint16_t command_handle;
    uint16_t status_handle;
    uint16_t texture_handle;
    uint16_t gesture_handle;
    uint16_t nav_handle;
} gatt_handles_t;

static gatt_handles_t s_handles;

/* TX buffer for notifications */
static uint8_t s_notify_buf[20];

/* ----------------------------------------------------------------
 * Nordic SoftDevice API wrappers (simplified for reference)
 *
 * In a real build, these map to sd_ble_* calls via the IPC driver
 * that communicates with the nRF5340 network core.
 * ---------------------------------------------------------------- */

typedef enum {
    BLE_ERROR_NONE = 0,
    BLE_ERROR_INVALID_PARAM = 1,
    BLE_ERROR_NOT_FOUND = 2,
    BLE_ERROR_NO_MEM = 3,
    BLE_ERROR_INTERNAL = 4,
} ble_err_t;

/* SoftDevice syscall wrappers (the real SDK provides these) */
extern int sd_ble_enable(uint32_t *p_app_ram_base);
extern int sd_ble_gap_addr_set(const uint8_t *p_addr);
extern int sd_ble_gap_adv_start(uint8_t adv_handle);
extern int sd_ble_gatts_service_add(uint16_t type, const void *p_uuid,
                                      uint16_t *p_handle);
extern int sd_ble_gatts_characteristic_add(uint16_t service_handle,
                                            const void *p_char_md,
                                            const void *p_attr,
                                            uint16_t *p_handle);
extern int sd_ble_gatts_hvx(uint16_t conn_handle, void *p_hvx);
extern int sd_ble_gatts_value_set(uint16_t handle, uint16_t offset,
                                   const uint8_t *p_value, uint16_t len);
extern int sd_ble_evt_get(uint8_t *p_dest, uint16_t *p_len);

/* Event types we care about */
#define BLE_GAP_EVT_CONNECTED     0x10
#define BLE_GAP_EVT_DISCONNECTED  0x11
#define BLE_GATTS_EVT_WRITE       0x30

/* ----------------------------------------------------------------
 * Build the advertising payload (device name + service UUID)
 * ---------------------------------------------------------------- */
static uint8_t adv_data[] = {
    /* Flags: LE General Discoverable, BR/EDR Not Supported */
    0x02, 0x01, 0x06,
    /* Complete Local Name: "TactiScript" (11 chars + length byte) */
    0x0C, 0x09, 'T','a','c','t','i','S','c','r','i','p','t',
    /* Service UUID 16-bit: 0x180A (Device Information — placeholder) */
    0x03, 0x03, 0x0A, 0x18,
};

/* ----------------------------------------------------------------
 * Initialize the BLE stack and register our custom GATT service
 * ---------------------------------------------------------------- */
void ble_init(void)
{
    if (s_initialized)
        return;

    /* Enable the SoftDevice (BLE stack on network core) */
    uint32_t ram_base = 0x20000000;
    sd_ble_enable(&ram_base);

    /* Set device address (use the factory-programmed one) */
    uint8_t addr[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    sd_ble_gap_addr_set(addr);

    /* --- Register the custom GATT service --- */

    /* Service UUID: custom 128-bit (base + 0x0000) */
    uint8_t svc_uuid[16] = BLE_SVC_UUID_BASE;
    /* The last two bytes are the service-specific part */
    svc_uuid[14] = 0x00;
    svc_uuid[15] = 0x00;

    uint16_t service_handle;
    sd_ble_gatts_service_add(0, svc_uuid, &service_handle); /* primary */

    /* Characteristic 1: Text Pipe (Write, no response) */
    {
        /* Characteristic metadata: write-only, no response */
        uint8_t char_uuid[16] = BLE_SVC_UUID_BASE;
        char_uuid[15] = (BLE_SVC_TEXT >> 8) & 0xFF;
        char_uuid[14] = BLE_SVC_TEXT & 0xFF;
        sd_ble_gatts_characteristic_add(service_handle, NULL, NULL,
                                         &s_handles.text_handle);
    }

    /* Characteristic 2: Command (Write) */
    {
        uint8_t char_uuid[16] = BLE_SVC_UUID_BASE;
        char_uuid[15] = (BLE_SVC_CMD >> 8) & 0xFF;
        char_uuid[14] = BLE_SVC_CMD & 0xFF;
        sd_ble_gatts_characteristic_add(service_handle, NULL, NULL,
                                         &s_handles.command_handle);
    }

    /* Characteristic 3: Status (Notify) */
    {
        uint8_t char_uuid[16] = BLE_SVC_UUID_BASE;
        char_uuid[15] = (BLE_SVC_STATUS >> 8) & 0xFF;
        char_uuid[14] = BLE_SVC_STATUS & 0xFF;
        sd_ble_gatts_characteristic_add(service_handle, NULL, NULL,
                                         &s_handles.status_handle);
    }

    /* Characteristic 4: Texture (Write) */
    {
        uint8_t char_uuid[16] = BLE_SVC_UUID_BASE;
        char_uuid[15] = (BLE_SVC_TEXTURE >> 8) & 0xFF;
        char_uuid[14] = BLE_SVC_TEXTURE & 0xFF;
        sd_ble_gatts_characteristic_add(service_handle, NULL, NULL,
                                         &s_handles.texture_handle);
    }

    /* Characteristic 5: Gesture (Notify) */
    {
        uint8_t char_uuid[16] = BLE_SVC_UUID_BASE;
        char_uuid[15] = (BLE_SVC_GESTURE >> 8) & 0xFF;
        char_uuid[14] = BLE_SVC_GESTURE & 0xFF;
        sd_ble_gatts_characteristic_add(service_handle, NULL, NULL,
                                         &s_handles.gesture_handle);
    }

    /* Characteristic 6: Nav Vector (Write) */
    {
        uint8_t char_uuid[16] = BLE_SVC_UUID_BASE;
        char_uuid[15] = (BLE_SVC_NAV >> 8) & 0xFF;
        char_uuid[14] = BLE_SVC_NAV & 0xFF;
        sd_ble_gatts_characteristic_add(service_handle, NULL, NULL,
                                         &s_handles.nav_handle);
    }

    /* Start advertising */
    sd_ble_gap_adv_start(0);

    s_initialized = true;
}

/* ----------------------------------------------------------------
 * Poll BLE events
 * ---------------------------------------------------------------- */
void ble_poll(void)
{
    uint8_t evt_buf[64];
    uint16_t evt_len = sizeof(evt_buf);

    while (sd_ble_evt_get(evt_buf, &evt_len) == 0) {
        /* Parse event (simplified — real SDK provides a parser) */
        uint8_t evt_id = evt_buf[0];
        uint16_t evt_conn_handle = (evt_buf[1] << 8) | evt_buf[2];

        switch (evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            s_connected = true;
            s_conn_handle = evt_conn_handle;
            ble_on_connected();
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            s_connected = false;
            s_conn_handle = 0xFFFF;
            ble_on_disconnected();
            /* Restart advertising */
            sd_ble_gap_adv_start(0);
            break;

        case BLE_GATTS_EVT_WRITE: {
            /* Extract the written handle and data */
            uint16_t write_handle = (evt_buf[3] << 8) | evt_buf[4];
            uint16_t data_len = (evt_buf[5] << 8) | evt_buf[6];
            uint8_t *data = &evt_buf[7];

            if (write_handle == s_handles.text_handle) {
                ble_on_text_received(data, data_len);
            } else if (write_handle == s_handles.command_handle) {
                if (data_len >= 1)
                    ble_on_command_received(data[0]);
            } else if (write_handle == s_handles.texture_handle) {
                ble_on_texture_received(data, data_len);
            } else if (write_handle == s_handles.nav_handle) {
                if (data_len >= 1)
                    ble_on_nav_received(data[0]);
            }
            break;
        }

        default:
            break;
        }

        evt_len = sizeof(evt_buf);
    }
}

/* ----------------------------------------------------------------
 * Send a notification for the gesture characteristic
 * ---------------------------------------------------------------- */
void ble_notify_gesture(uint8_t gesture)
{
    if (!s_connected)
        return;

    s_notify_buf[0] = gesture;
    /* HVX = Handle-Value Notification */
    sd_ble_gatts_hvx(s_conn_handle, NULL);
    /* In a real implementation, we set up the HVX params with the
     * gesture handle, the notify flag, and the data pointer. */
    sd_ble_gatts_value_set(s_handles.gesture_handle, 0,
                            s_notify_buf, 1);
}

/* ----------------------------------------------------------------
 * Send a status notification
 * ---------------------------------------------------------------- */
void ble_notify_status(uint8_t battery_pct, bool charging,
                        uint8_t mode, bool connected)
{
    if (!s_connected)
        return;

    s_notify_buf[0] = battery_pct;
    s_notify_buf[1] = charging ? 1 : 0;
    s_notify_buf[2] = mode;
    s_notify_buf[3] = connected ? 1 : 0;

    sd_ble_gatts_value_set(s_handles.status_handle, 0,
                            s_notify_buf, 4);
}

/* ----------------------------------------------------------------
 * Connection state
 * ---------------------------------------------------------------- */
bool ble_is_connected(void)
{
    return s_connected;
}

void ble_disconnect(void)
{
    /* In real SDK: sd_ble_gap_disconnect(s_conn_handle, ...); */
    s_connected = false;
    s_conn_handle = 0xFFFF;
}

/* ----------------------------------------------------------------
 * End of ble_svc.c
 * Author: jayis1
 * ---------------------------------------------------------------- */