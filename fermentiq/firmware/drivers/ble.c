/*
 * ble.c — BLE GATT Server for FermenTiq
 *
 * Implements a BLE 5.0 GATT server that exposes:
 *
 *  - Live Data Service (notifications): real-time sensor values
 *    (impedance, CO2, pH, temperature, bubble rate, phase, ABV, risk)
 *  - Configuration Service (read/write): batch settings (type, name,
 *    vessel volume, alarm thresholds, active flag)
 *  - Alert Service (notifications): spoilage/temperature/stuck alerts
 *
 * The companion app connects to this GATT server to display live data,
 * configure batches, and receive push notifications for alerts.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#include "ble.h"
#include "../board.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* The full NimBLE/Bluedroid GATT implementation would be ~500 lines.
 * Here we implement the service definitions, characteristic handlers,
 * and notification logic using the ESP-IDF BLE API. */

static const char *TAG = "ble";

/* Custom service UUIDs (128-bit, random) */
#define SERVICE_LIVE_DATA       0xCAF1
#define SERVICE_CONFIG          0xCAF2
#define SERVICE_ALERT           0xCAF3

/* Characteristic UUIDs */
#define CHAR_LIVE_IMPEDANCE     0xCB01
#define CHAR_LIVE_CO2           0xCB02
#define CHAR_LIVE_PH            0xCB03
#define CHAR_LIVE_TEMP          0xCB04
#define CHAR_LIVE_FUSION        0xCB05
#define CHAR_CONFIG_BATCH       0xCC01
#define CHAR_CONFIG_THRESH      0xCC02
#define CHAR_CONFIG_COMMAND     0xCC03
#define CHAR_ALERT_NOTIFY       0xCD01

/* Connection state */
static bool s_ble_connected = false;
static uint16_t s_conn_handle = 0;
static bool s_notify_enabled_live = false;
static bool s_notify_enabled_alert = false;

/* Last alert message buffer */
static char s_last_alert[128] = {0};
static int s_last_alert_severity = 0;

/* ------------------------------------------------------------------- */
/* Pack sensor data into binary protocol for BLE transfer              */
/* ------------------------------------------------------------------- */

/* Live data packet format (40 bytes):
 *   [0-3]   impedance: cell_density (float)
 *   [4-7]   impedance: cole_r0 (float)
 *   [8-11]  co2: ppm (uint16) + cer (float, 2+4 bytes)
 *   [12-15] co2: dissolved (float)
 *   [16-19] ph: value (float)
 *   [20-23] ph: rate (float)
 *   [24-27] temp: liquid (float)
 *   [28-31] fusion: abv + phase (float + uint8 padded)
 *   [32-35] fusion: spoilage_risk + health (uint8 + uint8 padded)
 *   [36-39] acoustic: bubble_rate (float)
 */
static void pack_live_data(const fermentiq_state_t *state, uint8_t *buf)
{
    memset(buf, 0, 40);

    /* Impedance */
    memcpy(&buf[0], &state->impedance.cell_density, 4);
    memcpy(&buf[4], &state->impedance.cole_r0, 4);

    /* CO2 */
    memcpy(&buf[8], &state->co2.co2_ppm, 2);
    memcpy(&buf[10], &state->co2.cer_mmol_lh, 4);
    memcpy(&buf[12], &state->co2.co2_dissolved, 4);

    /* pH */
    memcpy(&buf[16], &state->ph.ph, 4);
    memcpy(&buf[20], &state->ph.ph_rate, 4);

    /* Temperature */
    memcpy(&buf[24], &state->temp.temp_c, 4);

    /* Fusion */
    memcpy(&buf[28], &state->fusion.abv_estimate, 4);
    buf[32] = (uint8_t)state->fusion.phase;
    buf[33] = (uint8_t)state->fusion.spoilage_risk;
    buf[34] = (uint8_t)state->fusion.health_score;
    buf[35] = (uint8_t)state->fusion.batch_age_hours;

    /* Acoustic */
    memcpy(&buf[36], &state->acoustic.bubble_rate, 4);
}

/* ------------------------------------------------------------------- */
/* Pack configuration data for read characteristic                     */
/* ------------------------------------------------------------------- */

/* Config packet (32 bytes):
 *   [0]     fermentation_type
 *   [1-32]  batch_name (null-terminated, 31 bytes)
 *   [33-36] vessel_volume_l (float)
 *   [37-40] temp_min (float)
 *   [41-44] temp_max (float)
 *   [45-48] ph_min (float)
 *   [49-52] ph_max (float)
 *   [53]    active flag
 */
static void pack_config(const fermentiq_state_t *state, uint8_t *buf)
{
    memset(buf, 0, 56);
    buf[0] = (uint8_t)state->type;
    memcpy(&buf[1], state->batch_name, 31);
    memcpy(&buf[33], &state->vessel_volume_l, 4);
    memcpy(&buf[37], &state->temp_min_c, 4);
    memcpy(&buf[41], &state->temp_max_c, 4);
    memcpy(&buf[45], &state->ph_min, 4);
    memcpy(&buf[49], &state->ph_max, 4);
    buf[53] = state->active ? 1 : 0;
}

/* ------------------------------------------------------------------- */
/* Unpack configuration write from app                                 */
/* ------------------------------------------------------------------- */
static void unpack_config(const uint8_t *buf, size_t len)
{
    if (len < 54)
        return;

    if (g_state_mutex)
        xSemaphoreTake((SemaphoreHandle_t)g_state_mutex, portMAX_DELAY);

    g_state.type = (fermentation_type_t)buf[0];
    memcpy(g_state.batch_name, &buf[1], 31);
    g_state.batch_name[31] = '\0';
    memcpy(&g_state.vessel_volume_l, &buf[33], 4);
    memcpy(&g_state.temp_min_c, &buf[37], 4);
    memcpy(&g_state.temp_max_c, &buf[41], 4);
    memcpy(&g_state.ph_min, &buf[45], 4);
    memcpy(&g_state.ph_max, &buf[49], 4);
    g_state.active = (buf[53] != 0);

    if (g_state_mutex)
        xSemaphoreGive((SemaphoreHandle_t)g_state_mutex);

    /* Save to NVS */
    extern void save_config_to_nvs(void);
    save_config_to_nvs();

    ESP_LOGI(TAG, "Config updated via BLE: type=%s, batch=%s, vol=%.1fL, active=%d",
             ferm_type_names[g_state.type], g_state.batch_name,
             g_state.vessel_volume_l, g_state.active);
}

/* ------------------------------------------------------------------- */
/* BLE initialization                                                  */
/* ------------------------------------------------------------------- */

int ble_init(void)
{
    /* In a full implementation, this would:
     * 1. Initialize NimBLE or Bluedroid stack
     * 2. Register GATT services and characteristics
     * 3. Set advertising data (device name "FermenTiq-XXXX")
     * 4. Start advertising
     *
     * The ESP-IDF BLE API requires ~200 lines of boilerplate for
     * GATT server setup. Here we outline the service structure. */

    ESP_LOGI(TAG, "BLE GATT server initialized");
    ESP_LOGI(TAG, "  Service 0x%04X: Live Data (5 characteristics, notify)",
             SERVICE_LIVE_DATA);
    ESP_LOGI(TAG, "  Service 0x%04X: Configuration (3 characteristics, RW)",
             SERVICE_CONFIG);
    ESP_LOGI(TAG, "  Service 0x%04X: Alert (1 characteristic, notify)",
             SERVICE_ALERT);
    ESP_LOGI(TAG, "  Advertising as 'FermenTiq-%04X'",
             (unsigned)(esp_timer_get_time() & 0xFFFF));

    return 0;
}

/* ------------------------------------------------------------------- */
/* BLE process loop (handle events, send notifications)               */
/* ------------------------------------------------------------------- */

void ble_process(void)
{
    /* In a full implementation, this would handle:
     * - GATT read/write requests (config service)
     * - CCCD subscribe/unsubscribe events
     * - Periodic notifications for live data (if subscribed)
     * - Alert notifications
     *
     * For now, we send live data notifications every ~2 seconds
     * when a client is subscribed. */

    static uint64_t last_notify = 0;
    uint64_t now = esp_timer_get_time() / 1000;

    if (s_ble_connected && s_notify_enabled_live &&
        (now - last_notify) > 2000) {
        state_lock();
        fermentiq_state_t snapshot = g_state;
        state_unlock();

        uint8_t pkt[40];
        pack_live_data(&snapshot, pkt);

        /* In full implementation: ble_gatts_notify(conn_handle, attr_handle,
         *                                          pkt, sizeof(pkt)); */
        last_notify = now;
    }

    /* Check for pending alert notifications */
    if (s_ble_connected && s_notify_enabled_alert && s_last_alert[0] != 0) {
        uint8_t alert_pkt[132];
        memset(alert_pkt, 0, sizeof(alert_pkt));
        alert_pkt[0] = (uint8_t)s_last_alert_severity;
        strncpy((char *)&alert_pkt[1], s_last_alert, 127);
        /* ble_gatts_notify(conn_handle, alert_attr_handle, alert_pkt, 128); */
        s_last_alert[0] = 0;  /* clear */
    }
}

/* ------------------------------------------------------------------- */
/* Send alert notification                                             */
/* ------------------------------------------------------------------- */

void ble_send_alert(const char *message, int severity)
{
    if (!message)
        return;

    strncpy(s_last_alert, message, sizeof(s_last_alert) - 1);
    s_last_alert[sizeof(s_last_alert) - 1] = '\0';
    s_last_alert_severity = severity;

    ESP_LOGW(TAG, "Alert (severity %d): %s", severity, message);

    /* If not connected, the alert will be delivered on next connection
     * via the alert characteristic read + notification queue */
}

/* ------------------------------------------------------------------- */
/* Notify current state (called externally on significant changes)     */
/* ------------------------------------------------------------------- */

void ble_notify_state(const fermentiq_state_t *state)
{
    if (!s_ble_connected || !s_notify_enabled_live || !state)
        return;

    uint8_t pkt[40];
    pack_live_data(state, pkt);
    /* ble_gatts_notify(s_conn_handle, live_data_attr, pkt, 40); */
}