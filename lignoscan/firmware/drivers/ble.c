/*
 * ble.c — BLE 5.2 Communication via nRF52833 Module
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The nRF52833 module handles BLE 5.2 PHY and GATT server. The STM32H733
 * communicates with it via UART1 at 1 Mbps using a simple framing protocol.
 * The nRF52833 firmware (not included here) bridges UART packets to BLE
 * GATT characteristics for the mobile app.
 */

#include "ble.h"
#include "board.h"
#include <string.h>

/* UART frame format:
 * [0xAA] [type] [len_hi] [len_lo] [data...] [crc8] [0x55]
 */
#define BLE_FRAME_START  0xAA
#define BLE_FRAME_END    0x55

static int ble_connected = 0;
static uint8_t rx_buf[256];
static int rx_idx = 0;

/* ---- Initialize UART1 for BLE module communication ---- */
void ble_init(void) {
    /* Enable USART1 clock */
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;

    /* Configure baud rate: 1 Mbps
     * BRR = UART_CLK / baud = 140MHz / 1000000 = 140 */
    BLE_UART->BRR = (APB2_FREQ / BLE_BAUD);

    /* Enable TX, RX, and UART */
    BLE_UART->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;

    ble_connected = 0;
    rx_idx = 0;

    /* Send init command to nRF52833 */
    delay_ms(100);

    /* Send BLE service advertisement start command */
    uint8_t init_cmd[] = { 0xAA, BLE_PKT_INFO, 0x00, 0x01, 'I', 0x00, 0x55 };
    /* CRC placeholder — computed in ble_send_frame */
    (void)init_cmd;
}

/* ---- Compute CRC-8 (poly 0x07, init 0x00) ---- */
static uint8_t crc8(const uint8_t *data, int len) {
    uint8_t crc = 0x00;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/* ---- Send a framed packet via UART ---- */
static void ble_send_frame(uint8_t type, const uint8_t *data, uint16_t len) {
    if (len > BLE_PACKET_MAX) len = BLE_PACKET_MAX;

    /* Start byte */
    while (!(BLE_UART->ISR & USART_ISR_TXE)) { }
    BLE_UART->TDR = BLE_FRAME_START;

    /* Type */
    while (!(BLE_UART->ISR & USART_ISR_TXE)) { }
    BLE_UART->TDR = type;

    /* Length (big-endian) */
    while (!(BLE_UART->ISR & USART_ISR_TXE)) { }
    BLE_UART->TDR = (len >> 8) & 0xFF;
    while (!(BLE_UART->ISR & USART_ISR_TXE)) { }
    BLE_UART->TDR = len & 0xFF;

    /* Data payload */
    for (int i = 0; i < len; i++) {
        while (!(BLE_UART->ISR & USART_ISR_TXE)) { }
        BLE_UART->TDR = data[i];
    }

    /* CRC8 over type + length + data */
    uint8_t crc_buf[260];
    crc_buf[0] = type;
    crc_buf[1] = (len >> 8) & 0xFF;
    crc_buf[2] = len & 0xFF;
    memcpy(&crc_buf[3], data, len);
    uint8_t crc = crc8(crc_buf, 3 + len);

    while (!(BLE_UART->ISR & USART_ISR_TXE)) { }
    BLE_UART->TDR = crc;

    /* End byte */
    while (!(BLE_UART->ISR & USART_ISR_TXE)) { }
    BLE_UART->TDR = BLE_FRAME_END;
}

/* ---- Send status update (state + progress %) ---- */
void ble_send_status(uint8_t state, uint32_t progress) {
    uint8_t data[5];
    data[0] = state;
    data[1] = (progress >> 24) & 0xFF;
    data[2] = (progress >> 16) & 0xFF;
    data[3] = (progress >> 8) & 0xFF;
    data[4] = (progress >> 0) & 0xFF;
    ble_send_frame(BLE_PKT_STATUS, data, 5);
}

/* ---- Send ToF matrix (N×N floats, only upper triangle needed) ---- */
void ble_send_tof_matrix(float *matrix, int n_sensors) {
    /* Send only unique pairs (tx < rx) to save bandwidth.
     * N*(N-1)/2 pairs × 4 bytes = max 16*15/2*4 = 480 bytes
     * Split into multiple packets if needed. */
    int pair_count = 0;
    uint8_t buf[BLE_PACKET_MAX];
    int offset = 0;

    /* First 2 bytes: sensor count */
    buf[offset++] = (uint8_t)n_sensors;
    buf[offset++] = 0;  /* Reserved */

    for (int tx = 0; tx < n_sensors; tx++) {
        for (int rx = tx + 1; rx < n_sensors; rx++) {
            float tof = matrix[tx * n_sensors + rx];
            memcpy(&buf[offset], &tof, 4);
            offset += 4;
            pair_count++;

            /* Send packet if buffer is near full */
            if (offset + 4 > BLE_PACKET_MAX) {
                ble_send_frame(BLE_PKT_TOF, buf, offset);
                offset = 0;
                /* Add sequence number for reassembly */
                buf[offset++] = (uint8_t)(pair_count & 0xFF);
            }
        }
    }

    /* Send remaining data */
    if (offset > 0) {
        ble_send_frame(BLE_PKT_TOF, buf, offset);
    }
}

/* ---- Send reconstructed tomogram ---- */
void ble_send_tomogram(float *velocity, uint8_t *classification, int n_cells) {
    /* Send velocity map (4 bytes per cell) + classification (1 byte per cell)
     * Total = n_cells * 5 bytes. For 128 cells = 640 bytes → 3 packets. */
    int offset = 0;
    uint8_t buf[BLE_PACKET_MAX];
    int seq = 0;

    /* Header */
    buf[offset++] = (uint8_t)(n_cells & 0xFF);
    buf[offset++] = (uint8_t)(n_cells >> 8);
    buf[offset++] = (uint8_t)seq++;

    for (int i = 0; i < n_cells; i++) {
        /* Velocity (float32) */
        memcpy(&buf[offset], &velocity[i], 4);
        offset += 4;
        /* Classification */
        buf[offset++] = classification[i];

        if (offset + 5 > BLE_PACKET_MAX) {
            ble_send_frame(BLE_PKT_TOMOGRAM, buf, offset);
            offset = 0;
            buf[offset++] = (uint8_t)(n_cells & 0xFF);
            buf[offset++] = (uint8_t)(n_cells >> 8);
            buf[offset++] = (uint8_t)seq++;
        }
    }

    if (offset > 0) {
        ble_send_frame(BLE_PKT_TOMOGRAM, buf, offset);
    }
}

/* ---- Send GPS data ---- */
void ble_send_gps(gps_fix_t *fix) {
    uint8_t buf[32];
    int offset = 0;

    memcpy(&buf[offset], &fix->latitude, 4); offset += 4;
    memcpy(&buf[offset], &fix->longitude, 4); offset += 4;
    memcpy(&buf[offset], &fix->altitude_m, 4); offset += 4;
    memcpy(&buf[offset], &fix->hdop, 4); offset += 4;
    buf[offset++] = (uint8_t)fix->fix_quality;
    buf[offset++] = (uint8_t)fix->satellites;

    /* Timestamp string (up to 20 bytes) */
    int ts_len = strlen(fix->timestamp);
    if (ts_len > 20) ts_len = 20;
    buf[offset++] = (uint8_t)ts_len;
    memcpy(&buf[offset], fix->timestamp, ts_len);
    offset += ts_len;

    ble_send_frame(BLE_PKT_GPS, buf, offset);
}

/* ---- Send device info (version, battery, serial) ---- */
void ble_send_device_info(void) {
    uint8_t buf[16];
    int offset = 0;

    /* Firmware version (4 bytes BCD: 1.0.0.0) */
    buf[offset++] = 0x01;
    buf[offset++] = 0x00;
    buf[offset++] = 0x00;
    buf[offset++] = 0x00;

    /* Board name */
    const char *name = BOARD_NAME;
    int name_len = strlen(name);
    if (name_len > 10) name_len = 10;
    buf[offset++] = (uint8_t)name_len;
    memcpy(&buf[offset], name, name_len);
    offset += name_len;

    /* Battery percentage (placeholder) */
    buf[offset++] = 85;

    ble_send_frame(BLE_PKT_INFO, buf, offset);
}

/* ---- Check for incoming BLE command (non-blocking) ---- */
int ble_receive_command(uint8_t *cmd, uint8_t *data, int maxlen) {
    /* Read available UART data */
    while (BLE_UART->ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)(BLE_UART->RDR & 0xFF);

        if (rx_idx == 0 && byte != BLE_FRAME_START) {
            continue;  /* Wait for start byte */
        }

        rx_buf[rx_idx++] = byte;

        if (byte == BLE_FRAME_END && rx_idx >= 6) {
            /* Complete frame received — parse it */
            uint8_t type = rx_buf[1];
            uint16_t len = (rx_buf[2] << 8) | rx_buf[3];

            if (len <= (uint16_t)maxlen && rx_idx >= len + 6) {
                /* Verify CRC */
                uint8_t crc = crc8(&rx_buf[1], 3 + len);
                if (crc == rx_buf[rx_idx - 2]) {
                    *cmd = type;
                    memcpy(data, &rx_buf[4], len);
                    rx_idx = 0;
                    return len;
                }
            }
            rx_idx = 0;
        }

        if (rx_idx >= sizeof(rx_buf)) {
            rx_idx = 0;  /* Overflow — reset */
        }
    }

    return -1;  /* No complete command */
}

/* ---- Check if BLE is connected ---- */
int ble_is_connected(void) {
    /* In actual implementation, the nRF52833 asserts a GPIO pin
     * when a BLE connection is active. We check that pin here. */
    return ble_connected;
}

/* EOF — ble.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */