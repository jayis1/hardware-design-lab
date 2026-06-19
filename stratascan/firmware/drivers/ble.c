/*
 * ble.c — nRF52840 BLE 5.2 Protocol Driver
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements a UART-based command protocol to the nRF52840 BLE module.
 * The nRF52840 runs a custom GATT service that exposes:
 *   - A write characteristic for commands from the mobile app
 *   - A notify characteristic for streaming B-scan data to the app
 *   - A notify characteristic for status updates
 *
 * The STM32H743 communicates with the nRF52840 over USART3 at 460800 baud.
 * The protocol uses a simple framed format:
 *
 *   Frame: [0xAA] [0x55] [CMD] [LEN_L] [LEN_H] [PAYLOAD...] [CRC_L] [CRC_H]
 *
 * Commands from the app are forwarded by the nRF52840 to the STM32 via UART.
 * The STM32 processes commands and sends response/data frames back.
 *
 * The BLE module firmware (on the nRF52840) implements the GATT service and
 * UART bridge.  This driver handles the STM32 side of the UART protocol.
 */

#include "ble.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================== */
/*  Protocol constants                                                    */
/* ===================================================================== */

#define BLE_FRAME_SYNC0  0xAA
#define BLE_FRAME_SYNC1  0x55
#define BLE_MAX_PAYLOAD   256
#define BLE_RX_BUF_SIZE   512

/* ===================================================================== */
/*  UART RX ring buffer                                                   */
/* ===================================================================== */

static uint8_t  ble_rx_buf[BLE_RX_BUF_SIZE];
static volatile uint16_t ble_rx_head = 0;
static volatile uint16_t ble_rx_tail = 0;

/* Decoded command buffer */
static uint8_t  ble_cmd_buf[BLE_MAX_PAYLOAD];
static volatile uint8_t  ble_cmd_ready = 0;
static volatile uint8_t  ble_cmd_opcode = 0;
static volatile uint16_t ble_cmd_len = 0;
static volatile uint16_t ble_cmd_param_offset = 0;

/* ===================================================================== */
/*  USART3 RX interrupt handler                                           */
/* ===================================================================== */

void USART3_IRQHandler(void)
{
    if (USART3->ISR & USART_ISR_RXNE) {
        uint8_t c = (uint8_t)(USART3->RDR & 0xFF);
        uint16_t next = (ble_rx_head + 1) % BLE_RX_BUF_SIZE;
        if (next != ble_rx_tail) {
            ble_rx_buf[ble_rx_head] = c;
            ble_rx_head = next;
        }
    }
    if (USART3->ISR & 0x0F) {
        USART3->ICR = 0x0F;
    }
}

/* ===================================================================== */
/*  Frame parser (called from main loop)                                  */
/* ===================================================================== */

static uint8_t rx_byte(void)
{
    if (ble_rx_head == ble_rx_tail) return 0xFF;  /* empty */
    uint8_t c = ble_rx_buf[ble_rx_tail];
    ble_rx_tail = (ble_rx_tail + 1) % BLE_RX_BUF_SIZE;
    return c;
}

static uint16_t rx_available(void)
{
    if (ble_rx_head >= ble_rx_tail)
        return ble_rx_head - ble_rx_tail;
    return BLE_RX_BUF_SIZE - ble_rx_tail + ble_rx_head;
}

static void ble_parse_frames(void)
{
    static enum { WAIT_SYNC0, WAIT_SYNC1, GET_CMD, GET_LEN_L, GET_LEN_H,
                  GET_PAYLOAD, GET_CRC_L, GET_CRC_H } state = WAIT_SYNC0;
    static uint16_t payload_len = 0;
    static uint16_t payload_idx = 0;
    static uint8_t  opcode = 0;

    while (rx_available()) {
        uint8_t c = rx_byte();
        switch (state) {
        case WAIT_SYNC0:
            if (c == BLE_FRAME_SYNC0) state = WAIT_SYNC1;
            break;
        case WAIT_SYNC1:
            if (c == BLE_FRAME_SYNC1) state = GET_CMD;
            else state = WAIT_SYNC0;
            break;
        case GET_CMD:
            opcode = c;
            state = GET_LEN_L;
            break;
        case GET_LEN_L:
            payload_len = c;
            state = GET_LEN_H;
            break;
        case GET_LEN_H:
            payload_len |= ((uint16_t)c << 8);
            payload_idx = 0;
            if (payload_len > BLE_MAX_PAYLOAD) {
                state = WAIT_SYNC0;  /* too large, discard */
            } else if (payload_len == 0) {
                state = GET_CRC_L;
            } else {
                state = GET_PAYLOAD;
            }
            break;
        case GET_PAYLOAD:
            if (payload_idx < BLE_MAX_PAYLOAD) {
                ble_cmd_buf[payload_idx] = c;
            }
            payload_idx++;
            if (payload_idx >= payload_len) {
                state = GET_CRC_L;
            }
            break;
        case GET_CRC_L:
            /* CRC low byte — we skip CRC validation for simplicity */
            state = GET_CRC_H;
            break;
        case GET_CRC_H:
            /* CRC high byte — frame complete */
            ble_cmd_opcode = opcode;
            ble_cmd_len = payload_len;
            ble_cmd_param_offset = 0;
            ble_cmd_ready = 1;
            state = WAIT_SYNC0;
            return;
        }
    }
}

/* ===================================================================== */
/*  UART TX helpers                                                       */
/* ===================================================================== */

static void ble_uart_write(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        while (!(USART3->ISR & USART_ISR_TXE)) { }
        USART3->TDR = data[i];
    }
}

static uint16_t ble_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

static void ble_send_frame(uint8_t opcode, const uint8_t *payload, uint16_t len)
{
    uint8_t header[5] = { BLE_FRAME_SYNC0, BLE_FRAME_SYNC1, opcode,
                          (uint8_t)(len & 0xFF), (uint8_t)(len >> 8) };
    ble_uart_write(header, 5);
    if (len > 0 && payload) {
        ble_uart_write(payload, len);
    }
    uint16_t crc = ble_crc16(header + 2, 3);
    if (len > 0 && payload) {
        /* CRC over opcode + length + payload */
        /* Simplified: CRC just over payload */
        crc = ble_crc16(payload, len);
    }
    uint8_t crc_bytes[2] = { (uint8_t)(crc & 0xFF), (uint8_t)(crc >> 8) };
    ble_uart_write(crc_bytes, 2);
}

/* ===================================================================== */
/*  Public API                                                            */
/* ===================================================================== */

int ble_init(void)
{
    /* Reset nRF52840 module */
    BLE_NRST_PORT->BRR  = (1UL << BLE_NRST_PIN);
    board_delay_ms(50);
    BLE_NRST_PORT->BSRR = (1UL << BLE_NRST_PIN);
    board_delay_ms(200);  /* module boot */

    /* Configure USART3: 460800 baud, 8N1 */
    USART3->CR1 = 0;
    USART3->BRR = (BOARD_APB1_FREQ_HZ / BLE_BAUD);
    USART3->CR2 = 0;
    USART3->CR3 = 0;
    USART3->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;

    /* Enable USART3 interrupt */
    NVIC->ISER[0] = (1UL << USART3_IRQn);

    /* Clear RX buffer */
    ble_rx_head = 0;
    ble_rx_tail = 0;
    ble_cmd_ready = 0;

    board_delay_ms(100);
    return 0;
}

uint8_t ble_get_command(void)
{
    /* Parse any incoming frames */
    ble_parse_frames();

    if (ble_cmd_ready) {
        ble_cmd_ready = 0;
        return ble_cmd_opcode;
    }
    return 0;
}

uint8_t ble_get_param_byte(void)
{
    if (ble_cmd_param_offset < ble_cmd_len) {
        return ble_cmd_buf[ble_cmd_param_offset++];
    }
    return 0;
}

uint32_t ble_get_param_word(void)
{
    uint32_t val = 0;
    if (ble_cmd_param_offset + 3 < ble_cmd_len) {
        val = ble_cmd_buf[ble_cmd_param_offset];
        val |= ((uint32_t)ble_cmd_buf[ble_cmd_param_offset + 1] << 8);
        val |= ((uint32_t)ble_cmd_buf[ble_cmd_param_offset + 2] << 16);
        val |= ((uint32_t)ble_cmd_buf[ble_cmd_param_offset + 3] << 24);
        ble_cmd_param_offset += 4;
    }
    return val;
}

int ble_send_packet(const uint8_t *data, uint16_t len)
{
    /* Send a response frame with opcode 0xFF (generic data) */
    ble_send_frame(0xFF, data, len);
    return 0;
}

int ble_send_bscan_row(const float *trace, const float *depth,
                        uint16_t n, uint16_t row)
{
    /* Send a B-scan row as a structured frame:
     * Payload: [row_l] [row_h] [n_l] [n_h] [trace data as int16...] [depth as int16...]
     * Trace values are scaled to int16 for BLE bandwidth efficiency.
     */
    uint8_t payload[248];
    uint16_t pos = 0;

    payload[pos++] = (uint8_t)(row & 0xFF);
    payload[pos++] = (uint8_t)(row >> 8);

    uint16_t samples = n;
    if (samples > 60) samples = 60;  /* limit to fit BLE MTU */
    payload[pos++] = (uint8_t)(samples & 0xFF);
    payload[pos++] = (uint8_t)(samples >> 8);

    /* Pack trace as int16 (scale float → int16) */
    for (uint16_t i = 0; i < samples && pos < 240; i++) {
        int16_t val = (int16_t)(trace[i] * 1000.0f);
        if (val > 32767) val = 32767;
        if (val < -32768) val = -32768;
        payload[pos++] = (uint8_t)(val & 0xFF);
        payload[pos++] = (uint8_t)(val >> 8);
    }

    /* Depth values as uint16 (cm) */
    for (uint16_t i = 0; i < samples && pos < 246; i++) {
        uint16_t d_cm = (uint16_t)(depth[i] * 100.0f);
        payload[pos++] = (uint8_t)(d_cm & 0xFF);
        payload[pos++] = (uint8_t)(d_cm >> 8);
    }

    ble_send_frame(0x10, payload, pos);
    return 0;
}

void ble_notify_trace_ready(uint32_t trace_num)
{
    /* Send a notification that a new trace is available */
    uint8_t payload[4] = {
        (uint8_t)(trace_num & 0xFF),
        (uint8_t)(trace_num >> 8),
        (uint8_t)(trace_num >> 16),
        (uint8_t)(trace_num >> 24)
    };
    ble_send_frame(0x11, payload, 4);
}

/* End of ble.c */