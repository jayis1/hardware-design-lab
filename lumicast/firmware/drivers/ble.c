/*
 * ble.c — BLE advertising & GATT server
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Uses the STM32WB55 integrated 2.4 GHz radio with the BLE stack on the
 * Cortex-M0+ core.  This driver provides a simplified interface to the
 * ACI (Application Controller Interface) commands exposed via IPCC
 * (Inter-Processor Communication Channel) mailbox.
 *
 * In a full implementation this would link against ST's STM32WB_BLE_Stack
 * binary library.  This driver provides the ACI command wrappers and a
 * basic GATT server definition for the LumiCast service.
 */

#include "ble.h"
#include "timer_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  IPCC mailbox for M4↔M0+ communication                               */
/* ------------------------------------------------------------------ */

#define IPCC_BASE_ADDR   0x58000C00U
#define IPCC_CPU1_TO_CPU2 ((volatile uint32_t *)(IPCC_BASE_ADDR + 0x08U))
#define IPCC_CPU2_TO_CPU1 ((volatile uint32_t *)(IPCC_BASE_ADDR + 0x0CU))
#define IPCC_C1TO2SR     (*(volatile uint32_t *)(IPCC_BASE_ADDR + 0x10U))
#define IPCC_C1TO2SCR    (*(volatile uint32_t *)(IPCC_BASE_ADDR + 0x14U))
#define IPCC_C2TO1SR     (*(volatile uint32_t *)(IPCC_BASE_ADDR + 0x18U))
#define IPCC_C2TO1SCR    (*(volatile uint32_t *)(IPCC_BASE_ADDR + 0x1CU))

#define SRAM1_BASE       0x20000000U
#define SRAM2A_BASE      0x20030000U

/* Shared memory buffer for ACI commands. */
#define ACI_CMD_BUF      ((volatile uint8_t *)(SRAM2A_BASE))
#define ACI_RSP_BUF      ((volatile uint8_t *)(SRAM2A_BASE + 0x100U))

/* ------------------------------------------------------------------ */
/*  GATT database (simplified — in real impl this is in TLV table)       */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t id;
    uint8_t data[20];
    uint8_t len;
    bool notify_enabled;
} ble_char_t;

static ble_char_t chars[BLE_CHAR_TIMESTAMP + 1];
static bool connected = false;
static ble_cmd_cb_t cmd_callback = NULL;

/* Latest measurements (cached for notifications). */
static circadian_result_t last_circ;
static spectral_sample_t last_spec;
static flicker_result_t   last_flk;

/* ------------------------------------------------------------------ */

static void ipcc_send_cmd(uint8_t channel)
{
    IPCC_C1TO2SCR = (1U << channel);  /* set bit = send */
    /* Wait for M0+ to acknowledge by clearing. */
    uint32_t t = 0;
    while (IPCC_C1TO2SR & (1U << channel)) {
        if (t++ > 100000) break;
    }
}

static int aci_send(uint16_t opcode, const uint8_t *params, uint8_t plen)
{
    ACI_CMD_BUF[0] = plen + 3;
    ACI_CMD_BUF[1] = 0x01;  /* ACI command type */
    ACI_CMD_BUF[2] = opcode & 0xFF;
    ACI_CMD_BUF[3] = (opcode >> 8) & 0xFF;
    for (uint8_t i = 0; i < plen; i++) ACI_CMD_BUF[4 + i] = params[i];
    ipcc_send_cmd(1);
    return 0;
}

static void ble_init_gatt(void)
{
    /* ACI_GATT_ADD_SERVICE — primary service 0xFC01. */
    uint8_t svc_params[] = {
        0x01,  /* primary service */
        0x16,  /* UUID type: 16-bit */
        0x01, 0xFC  /* UUID LSB, MSB */
    };
    aci_send(0x0A01, svc_params, sizeof(svc_params));

    /* Add characteristics for each measurement. */
    for (uint8_t i = BLE_CHAR_MELANOPIC_EDI; i <= BLE_CHAR_TIMESTAMP; i++) {
        uint8_t ch_params[] = {
            0x00,  /* service handle (placeholder) */
            0x00, 0x14,  /* value length = 20 */
            0x0A,  /* properties: READ | NOTIFY */
            0x02,  /* UUID type: 16-bit */
            i, 0xFC  /* UUID */
        };
        aci_send(0x0A04, ch_params, sizeof(ch_params));
        chars[i].id = i;
        chars[i].len = 0;
        chars[i].notify_enabled = false;
    }
}

void ble_init(void)
{
    /* Enable IPCC and RF clocks. */
    RCC->AHB3ENR |= RCC_AHB3ENR_HSEMEN;
    RCC->APB3ENR |= RCC_APB3ENR_RFEN;

    /* Wake up M0+ core. */
    PWR->CR4 |= PWR_CR4_VBE;  /* enable backup domain */
    timer_delay_ms(5);

    /* Wait for M0+ to boot and signal readiness. */
    uint32_t t = 0;
    while (!(IPCC_C2TO1SR & 0x01)) {
        if (t++ > 200000) break;
    }
    IPCC_C2TO1SCR = 0x01;  /* clear ready signal */

    /* Initialize BLE stack on M0+. */
    aci_send(0xFC00, NULL, 0);  /* ACI_HAL_STACK_INIT */
    timer_delay_ms(50);

    /* Set device name. */
    static const char name[] = "LumiCast";
    uint8_t name_cmd[2 + 16] = { 0x00, 0x08 };
    memcpy(&name_cmd[2], name, sizeof(name) - 1);
    aci_send(0xFC0F, name_cmd, 2 + 8);

    /* Configure TX power (+0 dBm). */
    uint8_t txpwr[] = { 0x19, 0x00 };
    aci_send(0xFC07, txpwr, 2);

    /* Initialize GATT database. */
    ble_init_gatt();
}

static void ble_set_advertising_data(void)
{
    /* Advertising packet: Flags + Service UUID + Name (short). */
    uint8_t adv[20];
    uint8_t pos = 0;

    /* Flags: LE general discoverable, no BR/EDR. */
    adv[pos++] = 0x02;
    adv[pos++] = 0x01;
    adv[pos++] = 0x06;

    /* Service UUID 0xFC01. */
    adv[pos++] = 0x03;
    adv[pos++] = 0x02;
    adv[pos++] = 0x01;
    adv[pos++] = 0xFC;

    /* Local name (shortened). */
    adv[pos++] = 0x08;
    adv[pos++] = 0x08;
    adv[pos++] = 'L';
    adv[pos++] = 'u';
    adv[pos++] = 'm';
    adv[pos++] = 'i';
    adv[pos++] = 'C';
    adv[pos++] = 'a';

    aci_send(0x0C02, adv, pos);
}

static void ble_set_scan_response_data(void)
{
    uint8_t sr[28];
    uint8_t pos = 0;
    /* Full local name. */
    sr[pos++] = 0x09;
    sr[pos++] = 0x09;
    sr[pos++] = 'L'; sr[pos++] = 'u'; sr[pos++] = 'm'; sr[pos++] = 'i';
    sr[pos++] = 'C'; sr[pos++] = 'a'; sr[pos++] = 's'; sr[pos++] = 't';
    /* Manufacturer data (jayis1, 0xFFFF placeholder). */
    sr[pos++] = 0x06;
    sr[pos++] = 0xFF;
    sr[pos++] = 0xFF; sr[pos++] = 0xFF;  /* company ID placeholder */
    sr[pos++] = 'L'; sr[pos++] = 'C';     /* device type marker */
    aci_send(0x0C03, sr, pos);
}

void ble_start_advertising(void)
{
    ble_set_advertising_data();
    ble_set_scan_response_data();

    /* ACI_GAP_SET_DISCOVERABLE — connectable undirected. */
    uint8_t params[] = {
        0x00,  /* connectable undirected */
        0xC8, 0x00,  /* interval min = 200 ms */
        0xC8, 0x00,  /* interval max = 200 ms */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* own addr */
        0x00,  /* filter: no whitelist */
        0x00,  /* filter: no whitelist */
        0x00, 0x00  /* duration = infinite */
    };
    aci_send(0x0C08, params, sizeof(params));
}

void ble_stop_advertising(void)
{
    aci_send(0x0C09, NULL, 0);
}

static void ble_pack_float(uint8_t *buf, float val)
{
    /* Pack float as IEEE 754 little-endian. */
    union { float f; uint32_t u; } v;
    v.f = val;
    buf[0] = v.u & 0xFF;
    buf[1] = (v.u >> 8) & 0xFF;
    buf[2] = (v.u >> 16) & 0xFF;
    buf[3] = (v.u >> 24) & 0xFF;
}

static void ble_pack_u32(uint8_t *buf, uint32_t val)
{
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}

void ble_update_measurements(const circadian_result_t *circ,
                              const spectral_sample_t *spec,
                              const flicker_result_t *flk)
{
    memcpy(&last_circ, circ, sizeof(last_circ));
    memcpy(&last_spec, spec, sizeof(last_spec));
    memcpy(&last_flk, flk, sizeof(last_flk));

    /* Pack melanopic EDI. */
    ble_pack_float(chars[BLE_CHAR_MELANOPIC_EDI].data, circ->melanopic_edi);
    chars[BLE_CHAR_MELANOPIC_EDI].len = 4;

    /* Pack illuminance + CCT. */
    ble_pack_float(chars[BLE_CHAR_ILLUMINANCE].data, circ->illuminance_lux);
    ble_pack_float(chars[BLE_CHAR_ILLUMINANCE].data + 4, circ->cct_k);
    ble_pack_float(chars[BLE_CHAR_ILLUMINANCE].data + 8, circ->duv);
    chars[BLE_CHAR_ILLUMINANCE].len = 12;

    /* Pack CCT + CS. */
    ble_pack_float(chars[BLE_CHAR_CCT].data, circ->cct_k);
    ble_pack_float(chars[BLE_CHAR_CCT].data + 4, circ->x);
    ble_pack_float(chars[BLE_CHAR_CCT].data + 8, circ->y);
    chars[BLE_CHAR_CCT].len = 12;

    /* Pack circadian stimulus. */
    ble_pack_float(chars[BLE_CHAR_CIRCADIAN_CS].data, circ->circadian_stimulus);
    ble_pack_float(chars[BLE_CHAR_CIRCADIAN_CS].data + 4, circ->circadian_lightCLA);
    ble_pack_float(chars[BLE_CHAR_CIRCADIAN_CS].data + 8, circ->melanopic_er);
    chars[BLE_CHAR_CIRCADIAN_CS].len = 12;

    /* Pack spectrum (first 9 channels + clear = 10 × 2 = 20 bytes). */
    for (int i = 0; i < 10; i++) {
        chars[BLE_CHAR_SPECTRUM].data[i*2]     = spec->raw[i] & 0xFF;
        chars[BLE_CHAR_SPECTRUM].data[i*2 + 1] = (spec->raw[i] >> 8) & 0xFF;
    }
    chars[BLE_CHAR_SPECTRUM].len = 20;

    /* Pack flicker result. */
    ble_pack_float(chars[BLE_CHAR_FLICKER].data, flk->flicker_percent);
    ble_pack_float(chars[BLE_CHAR_FLICKER].data + 4, flk->fundamental_hz);
    ble_pack_float(chars[BLE_CHAR_FLICKER].data + 8, flk->safety_rating);
    chars[BLE_CHAR_FLICKER].len = 12;

    /* Status: connected, battery, etc. */
    chars[BLE_CHAR_STATUS].data[0] = connected ? 1 : 0;
    chars[BLE_CHAR_STATUS].data[1] = spec->gain;
    chars[BLE_CHAR_STATUS].data[2] = spec->atime & 0xFF;
    chars[BLE_CHAR_STATUS].data[3] = (spec->atime >> 8) & 0xFF;
    ble_pack_u32(chars[BLE_CHAR_STATUS].data + 4, spec->timestamp_ms);
    chars[BLE_CHAR_STATUS].len = 8;

    /* Timestamp. */
    ble_pack_u32(chars[BLE_CHAR_TIMESTAMP].data, spec->timestamp_ms);
    chars[BLE_CHAR_TIMESTAMP].len = 4;
}

void ble_notify_subscribers(void)
{
    if (!connected) return;
    for (uint8_t i = BLE_CHAR_MELANOPIC_EDI; i <= BLE_CHAR_TIMESTAMP; i++) {
        if (chars[i].notify_enabled && chars[i].len > 0) {
            /* ACI_GATT_UPDATE_CHAR_VALUE */
            uint8_t params[32];
            params[0] = i;  /* char handle (simplified) */
            params[1] = chars[i].len;
            params[2] = 0;   /* value offset */
            for (uint8_t j = 0; j < chars[i].len && j < 20; j++)
                params[3 + j] = chars[i].data[j];
            aci_send(0x0A0E, params, 3 + chars[i].len);
        }
    }
}

void ble_set_command_callback(ble_cmd_cb_t cb) { cmd_callback = cb; }
bool ble_is_connected(void) { return connected; }

void ble_tick(void)
{
    /* Poll for M0+ events. */
    if (IPCC_C2TO1SR & 0x02) {
        /* Event received — parse ACI event from ACI_RSP_BUF. */
        uint8_t evt_code = ACI_RSP_BUF[2];
        uint8_t evt_len = ACI_RSP_BUF[0] - 3;

        switch (evt_code) {
            case 0x06: {  /* LE_CONNECTION_COMPLETE */
                connected = true;
                break;
            }
            case 0x04: {  /* DISCONNECT_COMPLETE */
                connected = false;
                for (uint8_t i = 0; i <= BLE_CHAR_TIMESTAMP; i++)
                    chars[i].notify_enabled = false;
                ble_start_advertising();
                break;
            }
            case 0x0A: {  /* GATT_ATTRIBUTE_MODIFIED */
                /* Check if notification enabled flag changed. */
                uint8_t handle = ACI_RSP_BUF[4];
                if (handle <= BLE_CHAR_TIMESTAMP && ACI_RSP_BUF[8] & 0x01) {
                    chars[handle].notify_enabled = true;
                }
                break;
            }
            case 0x0D: {  /* VENDOR_DATA_WRITE */
                if (cmd_callback) {
                    uint8_t cmd = ACI_RSP_BUF[4];
                    cmd_callback(cmd, (uint8_t *)&ACI_RSP_BUF[5], evt_len - 1);
                }
                break;
            }
        }
        IPCC_C2TO1SCR = 0x02;  /* clear event */
    }
}