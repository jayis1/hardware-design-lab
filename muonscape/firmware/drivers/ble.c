/*
 * ble.c — BLE 5.2 status + command interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Uses the STM32H723's integrated radio IP via the ST BlueNRG-MS or
 * the H7's native BLE MAC (depends on package). Exposes a small GATT
 * service with three characteristics:
 *   - status (notify): JSON status, max 200 bytes
 *   - command (write): JSON command from app
 *   - track (notify): last track as JSON, streamed during acquisition
 *
 * The production build links against ST's BlueNRG ACI library; here we
 * expose the application-facing interface and a status cache.
 */
#include "ble.h"
#include "registers.h"
#include "board.h"
#include <string.h>

static char s_status_buf[256];
static uint16_t s_status_len = 0;
static char s_cmd_buf[256];
static volatile uint16_t s_cmd_len = 0;

/* Vendor hooks */
extern int aci_hal_init(void);
extern int aci_gap_set_discoverable(const char *name);
extern int aci_gatt_update_char(uint16_t uuid, const uint8_t *data, uint16_t len);
extern int aci_gatt_get_written_cmd(uint16_t uuid, uint8_t *buf, uint16_t *len);

muon_status_t ble_init(void)
{
    if (aci_hal_init() != 0) return MUON_ERR_WIFI;
    if (aci_gap_set_discoverable("MuonScape") != 0) return MUON_ERR_WIFI;
    s_status_len = 0;
    s_cmd_len = 0;
    return MUON_OK;
}

void ble_set_status(const char *json, uint16_t len)
{
    if (!json || len == 0 || len > sizeof(s_status_buf)) return;
    memcpy(s_status_buf, json, len);
    s_status_len = len;
    /* Push to GATT notify */
    aci_gatt_update_char(BLE_CHAR_STATUS_UUID,
                         (const uint8_t *)s_status_buf, s_status_len);
}

void ble_poll(void)
{
    uint16_t len = sizeof(s_cmd_buf);
    if (aci_gatt_get_written_cmd(BLE_CHAR_CMD_UUID,
                                 (uint8_t *)s_cmd_buf, &len) == 0 && len > 0) {
        s_cmd_len = len;
    }
}

uint16_t ble_get_cmd(char *buf, uint16_t max)
{
    if (!buf || max == 0) return 0;
    if (s_cmd_len == 0) return 0;
    uint16_t n = (s_cmd_len < max) ? s_cmd_len : max;
    memcpy(buf, s_cmd_buf, n);
    s_cmd_len = 0;
    return n;
}

/* ---- Weak vendor stubs ---- */
__attribute__((weak)) int aci_hal_init(void) { return 0; }
__attribute__((weak)) int aci_gap_set_discoverable(const char *n)
{ (void)n; return 0; }
__attribute__((weak)) int aci_gatt_update_char(uint16_t u, const uint8_t *d, uint16_t l)
{ (void)u; (void)d; (void)l; return 0; }
__attribute__((weak)) int aci_gatt_get_written_cmd(uint16_t u, uint8_t *b, uint16_t *l)
{ (void)u; if (b) *b = 0; if (l) *l = 0; return -1; }