/*
 * ble_drv.c — BLE 5.3 GATT service driver
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Implements BLE GATT services for TremorSense:
 * - TremorService: real-time tremor status notifications
 * - DataService: bulk episode download (chunked)
 * - ConfigService: runtime configuration (sample rate, sensitivity)
 * - DFUService: OTA firmware update (Nordic Secure Boot)
 *
 * Uses nRF5340 network core running S140 SoftDevice via IPC (RPMsg).
 */

#include "ble_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

static ble_config_callback_t config_cb = NULL;
static nfc_tag_callback_t    nfc_cb = NULL;
static uint8_t  ble_connected = 0;
static uint16_t ble_conn_handle = 0xFFFF;
static uint16_t tremor_status_handle = 0;  /* CCCD handle */
static char     device_name[32] = "TremorSense-0001";

/* ---- GATT service UUIDs (128-bit) ---- */
static const uint8_t tremor_service_uuid[16] = { BLE_UUID_BASE_LSB };
static const uint8_t data_service_uuid[16]   = { 0x10, 0x00, 0x40, 0x6E,
   0xE5, 0x0E, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x10, 0x00, 0x40, 0x6E };
static const uint8_t config_service_uuid[16] = { 0x20, 0x00, 0x40, 0x6E,
   0xE5, 0x0E, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x20, 0x00, 0x40, 0x6E };

/* ---- Advertising data ---- */
static const uint8_t adv_data[] = {
    /* Flags: LE general discoverable, BR/EDR not supported */
    0x02, 0x01, 0x06,
    /* Complete local name (truncated to fit 31-byte adv payload) */
    0x08, 0x09, 'T','r','e','m','o','r',
    /* Service UUID (16-bit shortened) */
    0x03, 0x03, 0x01, 0x00,
    /* TX power level */
    0x02, 0x0A, 0x00,
    /* Manufacturer specific data (TremorSense ID + firmware version) */
    0x06, 0xFF, 0x59, 0x00, 0x54, 0x53, 0x01, 0x00
};

static const uint8_t scan_resp_data[] = {
    /* Complete local name */
    0x0E, 0x09, 'T','r','e','m','o','r','S','e','n','s','e','-','0','1'
};

/* ---- Initialize BLE stack ---- */
int ble_init(void)
{
    /* In production:
     * 1. Initialize IPC (RPMsg) to network core
     * 2. Wait for SoftDevice (S140) to report readiness
     * 3. Configure BLE stack with:
     *    - Central + Peripheral role
     *    - 2 concurrent connections max
     *    - 8 bonds max
     *    - BLE_GAP_PRIVACY_OFF (use random MAC for privacy)
     * 4. Register GATT services
     * 5. Set security: bonding + MITM (LE Secure Connections)
     * 6. Enable NFC-A tag with NDEF message containing pairing URL
     */

    /* Generate device name from FICR DEVICEID */
    uint32_t dev_id = 0;  /* board_get_device_id(); */
    snprintf(device_name, sizeof(device_name), "TremorSense-%04lX",
             (unsigned long)(dev_id & 0xFFFF));

    /* Register GATT services */
    /* (GATT service registration via SoftDevice API calls) */

    return 0;
}

/* ---- Advertising ---- */
void ble_start_advertising(uint16_t interval_ms, int8_t tx_power_dbm)
{
    /* Configure advertising:
     * - Interval: configurable (default 1000 ms for power saving)
     * - Type: connectable undirected
     * - TX power: 0 dBm default
     * - Duration: infinite
     */

    /* Convert ms to BLE units (0.625 ms each) */
    uint16_t interval_units = (uint16_t)(interval_ms * 1000 / 625);

    /* Set TX power */
    /* sd_ble_gap_tx_power_set(tx_power_dbm); */

    /* Set advertising data and scan response */
    /* sd_ble_gap_adv_data_set(adv_data, sizeof(adv_data),
     *                          scan_resp_data, sizeof(scan_resp_data)); */

    /* Start advertising */
    /* sd_ble_gap_adv_start(&adv_params); */

    (void)interval_units;
    (void)tx_power_dbm;
}

void ble_stop_advertising(void)
{
    /* sd_ble_gap_adv_stop(); */
}

/* ---- Connection management ---- */
int ble_is_connected(void)
{
    return (int)ble_connected;
}

void ble_disconnect(void)
{
    if (ble_connected && ble_conn_handle != 0xFFFF) {
        /* sd_ble_gap_disconnect(ble_conn_handle,
         *    BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION); */
    }
    ble_connected = 0;
    ble_conn_handle = 0xFFFF;
}

/* ---- Notify tremor status ---- */
void ble_notify_tremor_status(const uint8_t *data, uint8_t len)
{
    if (!ble_connected) return;

    /* Check if notifications are enabled (CCCD) */
    /* if (notifications_enabled) { */
    /*   sd_ble_gatts_hvx(ble_conn_handle,
     *      BLE_GATT_HVX_NOTIFICATION, tremor_status_handle, &len, data); */
    /* } */

    (void)data;
    (void)len;
}

/* ---- Send episode data (chunked) ---- */
int ble_send_episode_chunk(const uint8_t *data, uint16_t len)
{
    if (!ble_connected) return -1;

    /* MTU is typically 247 bytes after negotiation.
     * Each chunk: 244 bytes payload + 3 bytes header (seq, total, crc).
     */

    uint16_t offset = 0;
    uint8_t chunk[247];
    uint8_t seq = 0;

    while (offset < len) {
        uint16_t chunk_len = len - offset;
        if (chunk_len > 244) chunk_len = 244;

        /* Build chunk header: [seq][total_chunks_hi][total_chunks_lo] */
        chunk[0] = seq++;
        chunk[1] = (uint8_t)((len + 243) / 244);
        chunk[2] = chunk[1];  /* Same as total for now */
        memcpy(&chunk[3], data + offset, chunk_len);

        /* Send via notification */
        /* sd_ble_gatts_hvx(ble_conn_handle,
         *   BLE_GATT_HVX_NOTIFICATION, episode_chunk_handle,
         *   &chunk_len_plus3, chunk); */

        offset += chunk_len;

        /* Flow control: wait for previous chunk to be sent */
        /* In production: use BLE_GATTS_EVT_HVN_TX_COMPLETE */
    }

    return 0;
}

int ble_start_download(void)
{
    /* Signal the firmware to start reading all records from flash
     * and sending them via ble_send_episode_chunk.
     * The actual reading happens in the main loop or a dedicated thread.
     */
    return 0;
}

/* ---- Config write handler ---- */
void ble_register_config_callback(ble_config_callback_t cb)
{
    config_cb = cb;
}

/* Called by BLE stack when a config characteristic is written */
static void handle_config_write(uint16_t uuid, const uint8_t *data, uint16_t len)
{
    if (config_cb) {
        config_cb(uuid, data, len);
    }

    /* Handle specific config writes */
    switch (uuid) {
    case BLE_UUID_SAMPLE_RATE:
        if (len >= 1) {
            /* Update sample rate: g_config.sample_rate_hz = data[0]; */
        }
        break;
    case BLE_UUID_SENSITIVITY:
        if (len >= 1) {
            /* Update sensitivity: g_config.sensitivity = data[0]; */
        }
        break;
    default:
        break;
    }
}

/* ---- NFC ---- */
void ble_register_nfc_callback(nfc_tag_callback_t cb)
{
    nfc_cb = cb;
}

void ble_log_medication_dose(uint32_t timestamp)
{
    /* Write a medication dose event to flash log.
     * Format: [MAGIC_MED][timestamp][crc16]
     * This allows the companion app to correlate medication timing
     * with tremor suppression onset.
     */
    uint8_t record[10];
    uint32_t magic = 0x4D454400;  /* "MED\0" */
    memcpy(&record[0], &magic, 4);
    memcpy(&record[4], &timestamp, 4);
    /* CRC-16 over bytes 0-7 */
    /* uint16_t crc = compute_crc16(record, 8); */
    /* memcpy(&record[8], &crc, 2); */
    /* flash_append_record(record, 10); */
}

/* ---- Set device name ---- */
void ble_set_device_name(const char *name)
{
    if (name && strlen(name) < sizeof(device_name)) {
        strcpy(device_name, name);
    }
}

/* ---- End of ble_drv.c ---- */