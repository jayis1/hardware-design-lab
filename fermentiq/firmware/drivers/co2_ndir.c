/*
 * co2_ndir.c — Senseair S8 (LP8) NDIR CO2 Sensor Driver
 *
 * Communicates with the Senseair S8 sensor over UART2 using the
 * sensor's custom byte-framing protocol. The S8 is a true NDIR
 * (non-dispersive infrared) CO2 sensor with built-in temperature
 * compensation and Automatic Baseline Correction (ABC).
 *
 * Protocol: each frame is:
 *   [Start byte 0xFE] [Command/Address] [Function] [Data...] [Checksum]
 * The checksum is the inverted 8-bit sum of all bytes including start.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#include "co2_ndir.h"
#include "../board.h"
#include "../registers.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "co2_ndir";

/* ------------------------------------------------------------------- */
/* CRC / Checksum calculation (inverted sum of bytes)                  */
/* ------------------------------------------------------------------- */

static uint8_t s8_checksum(const uint8_t *data, int len)
{
    uint8_t sum = 0;
    for (int i = 0; i < len; i++)
        sum += data[i];
    return (uint8_t)(0xFF - sum + 1);
}

/* ------------------------------------------------------------------- */
/* Send a request frame and read the response                          */
/* ------------------------------------------------------------------- */

static int s8_transaction(const uint8_t *tx, int tx_len,
                          uint8_t *rx, int rx_expect)
{
    /* Flush RX buffer */
    uart_flush_input(UART_NUM_2);

    /* Send request */
    int written = uart_write_bytes(UART_NUM_2, tx, tx_len);
    if (written != tx_len) {
        ESP_LOGE(TAG, "UART write failed (%d/%d)", written, tx_len);
        return -1;
    }
    uart_wait_tx_done(UART_NUM_2, pdMS_TO_TICKS(100));

    /* Read response */
    int total = 0;
    int retries = 0;
    while (total < rx_expect && retries < 50) {
        int n = uart_read_bytes(UART_NUM_2, rx + total,
                                rx_expect - total, pdMS_TO_TICKS(50));
        if (n > 0) total += n;
        else retries++;
    }

    if (total < rx_expect) {
        ESP_LOGW(TAG, "Response timeout (%d/%d bytes)", total, rx_expect);
        return -1;
    }

    /* Verify checksum */
    uint8_t calc = s8_checksum(rx, rx_expect - 1);
    if (calc != rx[rx_expect - 1]) {
        ESP_LOGW(TAG, "Checksum mismatch: calc=0x%02X recv=0x%02X",
                 calc, rx[rx_expect - 1]);
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------- */
/* Initialization                                                      */
/* ------------------------------------------------------------------- */

int co2_ndir_init(void)
{
    /* Verify communication by reading the serial number */
    uint32_t serial = 0;
    if (co2_ndir_read_serial(&serial) != 0) {
        ESP_LOGE(TAG, "S8 not responding on UART2");
        return -1;
    }
    ESP_LOGI(TAG, "Senseair S8 initialized, serial=%lu", (unsigned long)serial);

    /* Read firmware version */
    uint16_t version = 0;
    if (co2_ndir_read_version(&version) == 0) {
        ESP_LOGI(TAG, "S8 firmware version: 0x%04X", version);
    }

    return 0;
}

/* ------------------------------------------------------------------- */
/* Read CO2 concentration (ppm)                                        */
/* ------------------------------------------------------------------- */

int co2_ndir_read_ppm(uint16_t *ppm)
{
    /* Request: FE 04 00 00 00 01 25 C5 | FE 84 00 00 01 00 7A */
    /* S8 command 0x8404: read CO2, 2 data bytes returned */
    uint8_t tx[8] = { S8_FRAME_START, 0x04, 0x00, 0x00, 0x00, 0x01, 0x25, 0xC5 };

    /* Build request with checksum */
    tx[7] = s8_checksum(tx, 7);

    uint8_t rx[7];
    if (s8_transaction(tx, 8, rx, 7) != 0) {
        /* Retry with correct S8 protocol format:
         * Header(1) + Command(2) + Data(4) + Checksum(1) = 8 bytes tx
         * Header(1) + Command(2) + Data(2) + Checksum(1) = 6 bytes rx */
        uint8_t tx2[4] = { S8_FRAME_START, 0x84, 0x04, 0x00 };
        uint8_t chk = s8_checksum(tx2, 3);
        uint8_t tx_full[5] = { S8_FRAME_START, 0x84, 0x04, 0x00, chk };
        if (s8_transaction(tx_full, 5, rx, 6) != 0)
            return -1;
    }

    /* Extract CO2 value from response (2 bytes, big-endian) */
    *ppm = ((uint16_t)rx[2] << 8) | rx[3];

    /* Sanity check */
    if (*ppm < S8_CO2_MIN || *ppm > S8_CO2_MAX) {
        ESP_LOGW(TAG, "CO2 reading out of range: %u", *ppm);
        *ppm = S8_CO2_MIN;
    }

    return 0;
}

/* ------------------------------------------------------------------- */
/* Read serial number                                                  */
/* ------------------------------------------------------------------- */

int co2_ndir_read_serial(uint32_t *serial)
{
    uint8_t tx[5] = { S8_FRAME_START, 0xB0, 0xD0, 0x00, 0x00 };
    tx[4] = s8_checksum(tx, 4);

    uint8_t rx[8];
    if (s8_transaction(tx, 5, rx, 8) != 0)
        return -1;

    *serial = ((uint32_t)rx[2] << 24) | ((uint32_t)rx[3] << 16) |
              ((uint32_t)rx[4] << 8)  | (uint32_t)rx[5];
    return 0;
}

/* ------------------------------------------------------------------- */
/* Read firmware version                                               */
/* ------------------------------------------------------------------- */

int co2_ndir_read_version(uint16_t *version)
{
    uint8_t tx[5] = { S8_FRAME_START, 0xB0, 0xF0, 0x00, 0x00 };
    tx[4] = s8_checksum(tx, 4);

    uint8_t rx[6];
    if (s8_transaction(tx, 5, rx, 6) != 0)
        return -1;

    *version = ((uint16_t)rx[2] << 8) | rx[3];
    return 0;
}

/* ------------------------------------------------------------------- */
/* Manual background calibration (place sensor in fresh air, 400 ppm) */
/* ------------------------------------------------------------------- */

int co2_ndir_calibration(uint16_t target_ppm)
{
    ESP_LOGI(TAG, "Starting CO2 calibration to %u ppm...", target_ppm);

    /* Command 0x7306: background calibration with target value */
    uint8_t tx[8];
    tx[0] = S8_FRAME_START;
    tx[1] = 0x73;
    tx[2] = 0x06;
    tx[3] = (target_ppm >> 8) & 0xFF;
    tx[4] = target_ppm & 0xFF;
    tx[5] = 0x00;
    tx[6] = 0x00;
    tx[7] = s8_checksum(tx, 7);

    uint8_t rx[4];
    if (s8_transaction(tx, 8, rx, 4) != 0) {
        ESP_LOGE(TAG, "Calibration command failed");
        return -1;
    }

    ESP_LOGI(TAG, "Calibration acknowledged. Wait 2 minutes for completion.");
    return 0;
}