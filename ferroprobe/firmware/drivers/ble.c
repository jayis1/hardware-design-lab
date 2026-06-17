/*
 * ble.c — BLE UART protocol driver (nRF52840)
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Communicates with the Nordic nRF52840 BLE module over UART4 at
 * 1 Mbps with hardware flow control.  The nRF52840 runs a transparent
 * UART-over-BLE service that bridges the UART to a BLE GATT
 * characteristic, allowing the React Native app to send/receive
 * framed data.
 *
 * Frame format:
 *   [SOF=0xAA] [LEN] [OPCODE] [PAYLOAD...] [CRC] [EOF=0x55]
 *   LEN = length of OPCODE + PAYLOAD (not including CRC or SOF/EOF)
 *   CRC = XOR of all bytes from LEN through PAYLOAD
 */

#include "ble.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================== */
/*  UART4 init (nRF52840 at 1 Mbps)                                       */
/* ===================================================================== */

#define BLE_TX_BUF_SIZE 256
#define BLE_RX_BUF_SIZE 128

static uint8_t ble_tx_buf[BLE_TX_BUF_SIZE];
static uint16_t ble_tx_len = 0;
static uint16_t ble_tx_idx = 0;
static volatile uint8_t ble_tx_busy = 0;

static char ble_rx_buf[BLE_RX_BUF_SIZE];
static volatile uint16_t ble_rx_idx = 0;
static volatile uint8_t ble_frame_ready = 0;
static volatile uint16_t ble_frame_len = 0;

static ble_mode_callback_t mode_cb = 0;
static ble_threshold_callback_t threshold_cb = 0;
static ble_rate_callback_t rate_cb = 0;

/* Device status (filled by main loop) */
static uint8_t dev_battery = 100;
static uint8_t dev_gps_fix = 0;
static uint8_t dev_mode = MODE_IDLE;
static uint8_t dev_calib_valid = 0;
static uint32_t dev_record_count = 0;
static int8_t dev_temp = 25;

static void ble_uart_init(void)
{
    /* Enable GPIOA and UART4 clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
    RCC_APB1LENR |= RCC_APB1LENR_UART4EN;

    /* PA0=TX (AF8), PA1=RX (AF8), PA2=RTS (AF8), PA3=CTS (AF8) */
    volatile uint32_t *afrl = (volatile uint32_t *)(GPIOA_BASE + GPIO_AFRL);
    volatile uint32_t *moder = (volatile uint32_t *)(GPIOA_BASE + GPIO_MODER);
    volatile uint32_t *ospeedr = (volatile uint32_t *)(GPIOA_BASE + GPIO_OSPEEDR);

    for (uint8_t pin = 0; pin <= 3; pin++) {
        *afrl &= ~(0xFU << (pin * 4));
        *afrl |= ((uint32_t)GPIO_AF8 << (pin * 4));
        *moder &= ~(0x3U << (pin * 2));
        *moder |= (0x2U << (pin * 2));    /* AF mode */
        *ospeedr |= (0x3U << (pin * 2)); /* Very high speed */
    }

    /* Configure UART4: 1 Mbps, 8N1, hardware flow control (CTS/RTS) */
    USART_REG(UART4_BASE, USART_CR1) = 0;
    USART_REG(UART4_BASE, USART_BRR) = BLE_BRR_VAL;   /* 120 MHz / 1M = 120 */
    USART_REG(UART4_BASE, USART_CR2) = 0;
    USART_REG(UART4_BASE, USART_CR3) = (1U << 9) | (1U << 8);  /* CTSE + RTSE */

    /* Enable TX, RX, RXNE interrupt, TXE interrupt */
    USART_REG(UART4_BASE, USART_CR1) |= USART_CR1_TE | USART_CR1_RE;
    USART_REG(UART4_BASE, USART_CR1) |= USART_CR1_RXNEIE;
    USART_REG(UART4_BASE, USART_CR1) |= USART_CR1_UE;

    /* Enable NVIC for UART4 (IRQ 52) */
    NVIC_ISER0 |= (1U << 52);
}

/* ===================================================================== */
/*  Frame construction and transmission                                   */
/* ===================================================================== */

static uint8_t compute_crc(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0;
    for (uint16_t i = 0; i < len; i++)
        crc ^= data[i];
    return crc;
}

/* Send a frame: [SOF][LEN][OPCODE][payload...][CRC][EOF] */
static void ble_send_frame(uint8_t opcode, const uint8_t *payload, uint16_t payload_len)
{
    uint16_t total = 1 + 1 + 1 + payload_len + 1 + 1;  /* SOF + LEN + OP + payload + CRC + EOF */
    if (total > BLE_TX_BUF_SIZE)
        return;

    uint16_t idx = 0;
    ble_tx_buf[idx++] = BLE_FRAME_SOF;
    ble_tx_buf[idx++] = 1 + payload_len;  /* LEN = opcode + payload */
    ble_tx_buf[idx++] = opcode;
    if (payload_len > 0) {
        memcpy(&ble_tx_buf[idx], payload, payload_len);
        idx += payload_len;
    }
    /* CRC = XOR of [LEN, OPCODE, payload...] */
    ble_tx_buf[idx++] = compute_crc(&ble_tx_buf[1], 1 + 1 + payload_len);
    ble_tx_buf[idx++] = BLE_FRAME_EOF;

    ble_tx_len = idx;
    ble_tx_idx = 0;
    ble_tx_busy = 1;

    /* Start transmission by sending first byte */
    USART_REG(UART4_BASE, USART_CR1) |= USART_CR1_TCIE;  /* Enable TXE interrupt */
    USART_REG(UART4_BASE, USART_TDR) = ble_tx_buf[0];
    ble_tx_idx = 1;
}

/* ===================================================================== */
/*  Public send functions                                                  */
/* ===================================================================== */

void ble_send_status(uint8_t battery, uint8_t gps_fix,
                      uint8_t mode, uint8_t calib_valid,
                      uint32_t records, int8_t temp)
{
    uint8_t payload[10];
    payload[0] = battery;
    payload[1] = gps_fix;
    payload[2] = mode;
    payload[3] = calib_valid;
    payload[4] = temp;
    payload[5] = records & 0xFF;
    payload[6] = (records >> 8) & 0xFF;
    payload[7] = (records >> 16) & 0xFF;
    payload[8] = (records >> 24) & 0xFF;
    payload[9] = 0;  /* Reserved */
    ble_send_frame(BLE_RSP_STATUS, payload, 10);
}

void ble_send_field(const field_measurement_t *field,
                     const gps_data_t *gps, uint8_t flags)
{
    uint8_t payload[24];
    /* Field values: Bx, By, Bz, Btotal (int32 × 10 = 0.1 nT LSB) */
    int32_t bx = (int32_t)(field->bx_nt * 10.0f);
    int32_t by = (int32_t)(field->by_nt * 10.0f);
    int32_t bz = (int32_t)(field->bz_nt * 10.0f);
    int32_t bt = (int32_t)(field->b_total * 10.0f);
    memcpy(&payload[0], &bx, 4);
    memcpy(&payload[4], &by, 4);
    memcpy(&payload[8], &bz, 4);
    memcpy(&payload[12], &bt, 4);
    /* GPS fix status */
    payload[16] = gps->fix_quality;
    payload[17] = gps->satellites;
    /* Flags */
    payload[18] = flags;
    /* Temperature */
    payload[19] = (uint8_t)dev_temp;
    /* Timestamp */
    uint32_t ts = field->timestamp_ms;
    memcpy(&payload[20], &ts, 4);
    ble_send_frame(BLE_RSP_STREAM, payload, 24);
}

void ble_send_calib(const float offset[3], const float scale[3],
                     const float cross[3][3])
{
    uint8_t payload[60];  /* 3×4 + 3×4 + 9×4 = 60 bytes */
    uint16_t idx = 0;
    /* Offset (3 × float32) */
    memcpy(&payload[idx], offset, 12);  idx += 12;
    /* Scale (3 × float32) */
    memcpy(&payload[idx], scale, 12);  idx += 12;
    /* Cross-axis matrix (9 × float32) */
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            memcpy(&payload[idx], &cross[i][j], 4);
            idx += 4;
        }
    ble_send_frame(BLE_RSP_CALIB_DATA, payload, 60);
}

void ble_send_log_chunk(uint32_t offset, const uint8_t *data, uint8_t len)
{
    uint8_t payload[134];
    payload[0] = offset & 0xFF;
    payload[1] = (offset >> 8) & 0xFF;
    payload[2] = (offset >> 16) & 0xFF;
    payload[3] = (offset >> 24) & 0xFF;
    payload[4] = len;
    memcpy(&payload[5], data, len);
    ble_send_frame(BLE_RSP_LOG_CHUNK, payload, 5 + len);
}

void ble_send_version(void)
{
    uint8_t payload[8];
    payload[0] = FW_VERSION_MAJOR;
    payload[1] = FW_VERSION_MINOR;
    payload[2] = FW_VERSION_PATCH;
    payload[3] = 0;  /* Hardware revision */
    memcpy(&payload[4], FW_AUTHOR, 4);  /* "jayis1" */
    ble_send_frame(BLE_RSP_VERSION, payload, 8);
}

void ble_send_ok(uint8_t opcode)
{
    ble_send_frame(BLE_RSP_OK, &opcode, 1);
}

void ble_send_error(uint8_t opcode, uint8_t error_code)
{
    uint8_t payload[2] = {opcode, error_code};
    ble_send_frame(BLE_RSP_ERROR, payload, 2);
}

/* ===================================================================== */
/*  Command processing                                                     */
/* ===================================================================== */

static void ble_process_command(const uint8_t *frame, uint16_t len)
{
    if (len < 2)
        return;

    uint8_t opcode = frame[0];
    const uint8_t *payload = &frame[1];
    uint8_t payload_len = len - 1;

    switch (opcode) {
    case BLE_CMD_PING:
        ble_send_ok(opcode);
        break;

    case BLE_CMD_GET_STATUS:
        ble_send_status(dev_battery, dev_gps_fix, dev_mode,
                         dev_calib_valid, dev_record_count, dev_temp);
        break;

    case BLE_CMD_GET_VERSION:
        ble_send_version();
        break;

    case BLE_CMD_START_SURVEY:
        if (mode_cb) mode_cb(MODE_SURVEY);
        ble_send_ok(opcode);
        break;

    case BLE_CMD_STOP_SURVEY:
        if (mode_cb) mode_cb(MODE_IDLE);
        ble_send_ok(opcode);
        break;

    case BLE_CMD_START_CALIB:
        if (mode_cb) mode_cb(MODE_CALIB);
        ble_send_ok(opcode);
        break;

    case BLE_CMD_SET_RATE:
        if (payload_len >= 1 && rate_cb) {
            rate_cb(payload[0]);
            ble_send_ok(opcode);
        } else {
            ble_send_error(opcode, 1);
        }
        break;

    case BLE_CMD_GET_RATE:
        {
            uint8_t rate = (uint8_t)lockin_get_rate();
            ble_send_frame(BLE_RSP_DATA, &rate, 1);
        }
        break;

    case BLE_CMD_SET_THRESHOLD:
        if (payload_len >= 4 && threshold_cb) {
            int32_t threshold;
            memcpy(&threshold, payload, 4);
            threshold_cb(threshold);
            ble_send_ok(opcode);
        } else {
            ble_send_error(opcode, 1);
        }
        break;

    case BLE_CMD_STREAM_START:
        if (mode_cb) mode_cb(MODE_MONITOR);
        ble_send_ok(opcode);
        break;

    case BLE_CMD_STREAM_STOP:
        if (mode_cb) mode_cb(MODE_IDLE);
        ble_send_ok(opcode);
        break;

    case BLE_CMD_ERASE_LOG:
        ble_send_ok(opcode);
        break;

    default:
        ble_send_error(opcode, 0xFF);
        break;
    }
}

/* ===================================================================== */
/*  UART4 interrupt handler                                                */
/* ===================================================================== */

void UART4_IRQHandler(void)
{
    uint32_t isr = USART_REG(UART4_BASE, USART_ISR);

    /* RX: byte received */
    if (isr & USART_ISR_RXNE) {
        char c = (char)USART_REG(UART4_BASE, USART_RDR);

        if (c == (char)BLE_FRAME_SOF) {
            ble_rx_idx = 0;
        }
        if (ble_rx_idx < BLE_RX_BUF_SIZE) {
            ble_rx_buf[ble_rx_idx++] = c;
        }
        if (c == (char)BLE_FRAME_EOF && ble_rx_idx >= 5) {
            /* Complete frame received: verify CRC */
            uint8_t len = ble_rx_buf[1];
            uint8_t expected_crc = compute_crc((const uint8_t *)&ble_rx_buf[1], 1 + len);
            uint8_t actual_crc = ble_rx_buf[ble_rx_idx - 2];
            if (expected_crc == actual_crc) {
                ble_frame_ready = 1;
                ble_frame_len = len;
            } else {
                ble_frame_ready = 0;  /* CRC error, discard */
            }
        }
    }

    /* TX: transmit buffer empty */
    if ((isr & USART_ISR_TXE) && ble_tx_busy) {
        if (ble_tx_idx < ble_tx_len) {
            USART_REG(UART4_BASE, USART_TDR) = ble_tx_buf[ble_tx_idx++];
        } else {
            ble_tx_busy = 0;
            USART_REG(UART4_BASE, USART_CR1) &= ~USART_CR1_TCIE;  /* Disable TX interrupt */
        }
    }

    /* Clear flags */
    USART_REG(UART4_BASE, USART_ICR) = 0xFFFFFFFF;
}

void ble_process(void)
{
    if (ble_frame_ready) {
        /* Process the frame: skip SOF and EOF, pass [opcode + payload] */
        ble_process_command((const uint8_t *)&ble_rx_buf[2], ble_frame_len);
        ble_frame_ready = 0;
        ble_rx_idx = 0;
    }
}

/* ===================================================================== */
/*  Initialization and callbacks                                           */
/* ===================================================================== */

void ble_init(void)
{
    ble_uart_init();
    ble_tx_busy = 0;
    ble_rx_idx = 0;
    ble_frame_ready = 0;
}

void ble_set_mode_callback(ble_mode_callback_t cb) { mode_cb = cb; }
void ble_set_threshold_callback(ble_threshold_callback_t cb) { threshold_cb = cb; }
void ble_set_rate_callback(ble_rate_callback_t cb) { rate_cb = cb; }

/* Update device status (called from main loop) */
void ble_update_status(uint8_t battery, uint8_t gps_fix, uint8_t mode,
                         uint8_t calib_valid, uint32_t records, int8_t temp)
{
    dev_battery = battery;
    dev_gps_fix = gps_fix;
    dev_mode = mode;
    dev_calib_valid = calib_valid;
    dev_record_count = records;
    dev_temp = temp;
}