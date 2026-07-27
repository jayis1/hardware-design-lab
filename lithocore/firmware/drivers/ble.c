/*
 * ble.c — BLE co-processor protocol.
 *
 * Communicates with the BL654 (Nordic nRF52840) BLE 5.2 module over UART1
 * (PA9 TX, PA10 RX, 115200 baud). The BL654 runs a pre-flashed GATT
 * server firmware that exposes the LithoCore service with 4 characteristics.
 * The STM32G474 sends data over UART and the BL654 forwards it as BLE
 * notifications.
 *
 * Protocol: simple framed binary over UART.
 *   Frame: [SYNC1][SYNC2][CMD][LEN][DATA...][CRC8]
 *   SYNC1 = 0xAA, SYNC2 = 0x55
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "ble.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

#define BLE_SYNC1      0xAA
#define BLE_SYNC2      0x55
#define BLE_MAX_FRAME  64

/* UART ring buffer for received data */
#define BLE_RX_BUF_SIZE 128
static volatile uint8_t  ble_rx_buf[BLE_RX_BUF_SIZE];
static volatile uint16_t ble_rx_head = 0;
static volatile uint16_t ble_rx_tail = 0;
static volatile uint8_t  ble_connected = 0;

/* -------------------------------------------------------------------------
 * UART1 initialization
 *
 * PA9 = TX, PA10 = RX, 115200 baud, 8N1.
 * BRR = PCLK / baud = 163840000 / 115200 = 1422
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int ble_init(const litho_config_t *config)
{
    (void)config;

    /* Enable USART1 */
    USART1->BRR = 1422;   /* 163.84 MHz / 115200 */
    USART1->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE |
                  USART_CR1_RXNEIE;

    /* Enable USART1 interrupt in NVIC */
    NVIC_ISER0 = (1U << IRQ_USART1);

    ble_connected = 0;
    ble_rx_head = 0;
    ble_rx_tail = 0;

    return BLE_OK;
}

/* -------------------------------------------------------------------------
 * UART send byte
 * ------------------------------------------------------------------------- */
static void ble_uart_send(uint8_t byte)
{
    while (!(USART1->ISR & USART_ISR_TXE)) { }
    USART1->TDR = byte;
}

/* -------------------------------------------------------------------------
 * UART send buffer
 * ------------------------------------------------------------------------- */
static void ble_uart_send_buf(const uint8_t *data, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++)
        ble_uart_send(data[i]);
}

/* -------------------------------------------------------------------------
 * CRC-8 (Dallas/Maxim 1-Wire polynomial)
 * ------------------------------------------------------------------------- */
static uint8_t crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* -------------------------------------------------------------------------
 * Send a framed message to the BLE module
 * ------------------------------------------------------------------------- */
static void ble_send_frame(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    uint8_t frame[BLE_MAX_FRAME];
    if (len + 5 > BLE_MAX_FRAME)
        len = BLE_MAX_FRAME - 5;

    frame[0] = BLE_SYNC1;
    frame[1] = BLE_SYNC2;
    frame[2] = cmd;
    frame[3] = len;
    if (data && len > 0)
        memcpy(&frame[4], data, len);
    frame[4 + len] = crc8(&frame[2], 2 + len);  /* CRC over cmd+len+data */

    ble_uart_send_buf(frame, 5 + len);
}

/* -------------------------------------------------------------------------
 * USART1 interrupt handler — receive data into ring buffer
 * ------------------------------------------------------------------------- */
void USART1_Handler(void)
{
    if (USART1->ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)USART1->RDR;
        uint16_t next = (ble_rx_head + 1) % BLE_RX_BUF_SIZE;
        if (next != ble_rx_tail) {
            ble_rx_buf[ble_rx_head] = byte;
            ble_rx_head = next;
        }
    }
    if (USART1->ISR & USART_ISR_IDLE) {
        /* BLE module sends a connection-state notification on IDLE */
        USART1->ICR = USART_ISR_IDLE;  /* clear IDLE flag */
    }
    /* Clear error flags */
    if (USART1->ISR & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE)) {
        USART1->ICR = 0xFF;
    }
}

/* -------------------------------------------------------------------------
 * Read a byte from the ring buffer
 * ------------------------------------------------------------------------- */
static int ble_rx_byte(uint8_t *byte, uint32_t timeout)
{
    uint32_t count = 0;
    while (ble_rx_tail == ble_rx_head) {
        if (++count > timeout * 1000)
            return BLE_TIMEOUT;
    }
    *byte = ble_rx_buf[ble_rx_tail];
    ble_rx_tail = (ble_rx_tail + 1) % BLE_RX_BUF_SIZE;
    return BLE_OK;
}

/* -------------------------------------------------------------------------
 * Parse a received frame
 * ------------------------------------------------------------------------- */
static int ble_rx_frame(uint8_t *cmd, uint8_t *data, uint8_t *len)
{
    uint8_t byte;
    int ret;

    /* Wait for SYNC1 */
    do {
        ret = ble_rx_byte(&byte, 100);
        if (ret != BLE_OK) return ret;
    } while (byte != BLE_SYNC1);

    /* SYNC2 */
    ret = ble_rx_byte(&byte, 10);
    if (ret != BLE_OK || byte != BLE_SYNC2) return BLE_ERROR;

    /* CMD */
    ret = ble_rx_byte(cmd, 10);
    if (ret != BLE_OK) return BLE_ERROR;

    /* LEN */
    ret = ble_rx_byte(len, 10);
    if (ret != BLE_OK) return BLE_ERROR;

    /* DATA */
    for (uint8_t i = 0; i < *len; i++) {
        ret = ble_rx_byte(&data[i], 10);
        if (ret != BLE_OK) return BLE_ERROR;
    }

    /* CRC */
    uint8_t crc_recv;
    ret = ble_rx_byte(&crc_recv, 10);
    if (ret != BLE_OK) return BLE_ERROR;

    /* Verify CRC */
    uint8_t crc_buf[2 + 64];
    crc_buf[0] = *cmd;
    crc_buf[1] = *len;
    memcpy(&crc_buf[2], data, *len);
    if (crc8(crc_buf, 2 + *len) != crc_recv)
        return BLE_ERROR;

    return BLE_OK;
}

/* -------------------------------------------------------------------------
 * Get a command from the BLE module
 * ------------------------------------------------------------------------- */
int ble_get_command(uint8_t *cmd)
{
    uint8_t data[32];
    uint8_t len;
    int ret = ble_rx_frame(cmd, data, &len);
    if (ret != BLE_OK)
        return BLE_ERROR;

    /* Handle connection-state commands from the BLE module */
    if (*cmd == 0xF0) {
        /* Connection state: data[0] = 1 (connected) or 0 (disconnected) */
        ble_connected = (len > 0) ? data[0] : 0;
        return BLE_ERROR;  /* not a user command, consume it */
    }

    return BLE_OK;
}

/* -------------------------------------------------------------------------
 * Send status notification
 * ------------------------------------------------------------------------- */
int ble_send_status(uint8_t state, uint8_t progress, uint8_t result_valid)
{
    uint8_t data[3] = { state, progress, result_valid };
    ble_send_frame(0x10, data, 3);
    return BLE_OK;
}

/* -------------------------------------------------------------------------
 * Send a sweep data point (frequency + impedance)
 *
 * Packed as 20 bytes:
 *   freq_hz (4) | re_z (4) | im_z (4) | mag (4) | phase (4)
 * All as int32 (Q-format or physical units).
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int ble_send_sweep_point(uint32_t freq_hz, int32_t re_z, int32_t im_z,
                         int32_t mag, int32_t phase, uint8_t flags)
{
    uint8_t data[21];
    memcpy(&data[0], &freq_hz, 4);
    memcpy(&data[4], &re_z, 4);
    memcpy(&data[8], &im_z, 4);
    memcpy(&data[12], &mag, 4);
    memcpy(&data[16], &phase, 4);
    data[20] = flags;
    ble_send_frame(0x11, data, 21);
    return BLE_OK;
}

/* -------------------------------------------------------------------------
 * Send the final result
 * ------------------------------------------------------------------------- */
int ble_send_result(const soh_result_t *result)
{
    /* Pack the key results into a compact frame.
     * Full result is large (contains the entire sweep data), so we send
     * the summary first, then sweep points individually. */
    uint8_t data[32];
    uint16_t idx = 0;

    /* SoH score */
    data[idx++] = result->soh_score;
    /* Degradation mode */
    data[idx++] = (uint8_t)result->degradation;
    /* Verdict */
    data[idx++] = (uint8_t)result->verdict;
    /* OCV mV */
    memcpy(&data[idx], &result->ocv_mv, 2); idx += 2;
    /* Temperature */
    memcpy(&data[idx], &result->temp_dc, 2); idx += 2;
    /* DCIR mΩ */
    memcpy(&data[idx], &result->dcir_mohm, 2); idx += 2;
    /* Self-discharge rate */
    memcpy(&data[idx], &result->self_discharge_uv_per_min, 4); idx += 4;
    /* Chemistry index */
    data[idx++] = result->chemistry_idx;
    /* Randles parameters (if fit valid) */
    if (result->fit_valid) {
        memcpy(&data[idx], &result->randles, sizeof(randles_params_t));
        idx += sizeof(randles_params_t);
        /* Clamp to frame size */
        if (idx > 60) idx = 60;
    }
    data[idx++] = result->fit_valid;

    ble_send_frame(0x12, data, idx);

    /* Send each sweep point as a separate notification */
    for (uint16_t i = 0; i < result->sweep_data.num_points; i++) {
        const lockin_result_t *pt = &result->sweep_data.points[i];
        if (pt->valid) {
            ble_send_sweep_point(pt->freq_hz, pt->re_z, pt->im_z,
                                pt->mag_z, pt->phase_mdeg, 0);
        }
    }

    /* Send end-of-data marker */
    ble_send_frame(0x13, NULL, 0);

    return BLE_OK;
}

/* -------------------------------------------------------------------------
 * Send error message
 * ------------------------------------------------------------------------- */
int ble_send_error(const char *msg)
{
    uint8_t len = 0;
    while (msg[len] && len < 30) len++;
    ble_send_frame(0xFE, (const uint8_t *)msg, len);
    return BLE_OK;
}

/* -------------------------------------------------------------------------
 * Receive configuration from the app
 * ------------------------------------------------------------------------- */
int ble_receive_config(litho_config_t *config)
{
    /* The config data was received in the command frame.
     * This is called after ble_get_command returned BLE_CMD_SET_CONFIG.
     * In a real implementation, we'd store the received frame data.
     * For now, this is a stub that would parse the frame. */
    (void)config;
    return BLE_OK;
}

/* -------------------------------------------------------------------------
 * Check if BLE is connected
 * ------------------------------------------------------------------------- */
uint8_t ble_is_connected(void)
{
    return ble_connected;
}

/* -------------------------------------------------------------------------
 * Send raw notification (for the BLE module to forward)
 * ------------------------------------------------------------------------- */
void ble_send_notification(const uint8_t *data, uint8_t len)
{
    ble_send_frame(0x20, data, len);
}