/*
 * ble.c — nRF52840 BLE module communication driver
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Communicates with an nRF52840 BLE module via USART1.  The module runs
 * a UART-to-BLE transparent bridge firmware (e.g., Nordic's UART Service),
 * so data written to the UART appears as notifications on the connected
 * phone's BLE characteristic.
 *
 * Frame format:
 *   [SOF=0xAA] [LEN] [OPCODE] [PAYLOAD...] [CRC] [EOF=0x55]
 *
 * CRC is a simple XOR checksum over LEN + OPCODE + PAYLOAD.
 * Maximum payload: 128 bytes (BLE_MTU - framing overhead).
 */

#include "ble.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================== */
/*  Internal state                                                       */
/* ===================================================================== */

#define BLE_RX_BUF_SIZE  256U

static uint8_t  rx_buf[BLE_RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

static uint8_t  frame_buf[BLE_RX_BUF_SIZE];
static uint16_t frame_idx = 0;
static uint8_t  frame_state = 0;   /* 0=idle, 1=got_sof, 2=got_len, 3=got_payload, 4=got_crc */
static uint8_t  frame_len = 0;
static uint8_t  frame_opcode = 0;

static ble_cmd_handler_t cmd_handler = 0;

/* Connection status (from module's status pin — we use PC13 as BLE_CONN)
 * Actually, the nRF52840 doesn't have a dedicated CONN pin.  We infer
 * connection state from the module's response to a PING command.
 */
static uint8_t ble_connected = 0;

/* ===================================================================== */
/*  USART1 ISR (RX)                                                      */
/* ===================================================================== */

/* USART1_IRQHandler: read incoming bytes into ring buffer */
void USART1_IRQHandler(void)
{
    if (USART_REG(USART1_BASE, USART_ISR) & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)USART_REG(USART1_BASE, USART_RDR);
        uint16_t next = (rx_head + 1) % BLE_RX_BUF_SIZE;
        if (next != rx_tail) {
            rx_buf[rx_head] = byte;
            rx_head = next;
        }
    }
    /* Handle errors */
    if (USART_REG(USART1_BASE, USART_ISR) & USART_ISR_ORE) {
        USART_REG(USART1_BASE, USART_ICR) = USART_ISR_ORE;
    }
}

/* ===================================================================== */
/*  Initialization                                                       */
/* ===================================================================== */

void ble_init(void)
{
    /* Enable GPIOA and USART1 clocks */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;

    /* PA9 (TX), PA10 (RX) as AF7 */
    volatile uint32_t *pa_moder = (volatile uint32_t *)(GPIOA_BASE + GPIO_MODER);
    volatile uint32_t *pa_afrh  = (volatile uint32_t *)(GPIOA_BASE + GPIO_AFRH);
    volatile uint32_t *pa_ospeed = (volatile uint32_t *)(GPIOA_BASE + GPIO_OSPEEDR);

    /* PA9 (TX) */
    *pa_moder &= ~(0x3U << (9 * 2));
    *pa_moder |= (0x2U << (9 * 2));
    *pa_afrh &= ~(0xFU << ((9 - 8) * 4));
    *pa_afrh |= ((uint32_t)GPIO_AF7 << ((9 - 8) * 4));
    *pa_ospeed |= (0x3U << (9 * 2));

    /* PA10 (RX) */
    *pa_moder &= ~(0x3U << (10 * 2));
    *pa_moder |= (0x2U << (10 * 2));
    *pa_afrh &= ~(0xFU << ((10 - 8) * 4));
    *pa_afrh |= ((uint32_t)GPIO_AF7 << ((10 - 8) * 4));

    /* Configure USART1: 115200 8N1, enable RX interrupt */
    USART_REG(USART1_BASE, USART_CR1) = 0;
    USART_REG(USART1_BASE, USART_CR2) = 0;
    USART_REG(USART1_BASE, USART_CR3) = 0;
    USART_REG(USART1_BASE, USART_BRR) = BLE_BRR_VAL;
    USART_REG(USART1_BASE, USART_CR1) = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE |
                                         USART_CR1_RXNEIE;

    /* Enable USART1 interrupt in NVIC */
    NVIC_ISER0 |= (1U << 37);  /* USART1_IRQn = 37 on L4 */

    rx_head = 0;
    rx_tail = 0;
    frame_state = 0;
    frame_idx = 0;
}

/* ===================================================================== */
/*  Frame building / parsing                                             */
/* ===================================================================== */

static uint8_t compute_crc(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++)
        crc ^= data[i];
    return crc;
}

static void ble_tx_byte(uint8_t b)
{
    while (!(USART_REG(USART1_BASE, USART_ISR) & USART_ISR_TXE))
        ;
    USART_REG(USART1_BASE, USART_TDR) = b;
}

uint8_t ble_send(uint8_t opcode, const uint8_t *payload, uint8_t len)
{
    if (len > BLE_MAX_PAYLOAD) return 1;

    uint8_t header[3];
    header[0] = BLE_FRAME_SOF;
    header[1] = len + 1;   /* LEN = opcode + payload */
    header[2] = opcode;

    /* Send header */
    for (int i = 0; i < 3; i++)
        ble_tx_byte(header[i]);

    /* Send payload */
    for (uint8_t i = 0; i < len; i++)
        ble_tx_byte(payload[i]);

    /* Compute and send CRC (over LEN + OPCODE + PAYLOAD) */
    uint8_t crc_buf[2 + 128];
    crc_buf[0] = header[1];
    crc_buf[1] = opcode;
    for (uint8_t i = 0; i < len; i++)
        crc_buf[2 + i] = payload[i];
    uint8_t crc = compute_crc(crc_buf, 2 + len);

    ble_tx_byte(crc);
    ble_tx_byte(BLE_FRAME_EOF);

    /* Wait for TC */
    while (!(USART_REG(USART1_BASE, USART_ISR) & USART_ISR_TC))
        ;

    return 0;
}

uint8_t ble_poll(void)
{
    while (rx_tail != rx_head) {
        uint8_t byte = rx_buf[rx_tail];
        rx_tail = (rx_tail + 1) % BLE_RX_BUF_SIZE;

        switch (frame_state) {
        case 0:  /* Idle, waiting for SOF */
            if (byte == BLE_FRAME_SOF) {
                frame_state = 1;
                frame_idx = 0;
            }
            break;

        case 1:  /* Got SOF, expecting LEN */
            frame_len = byte;
            frame_state = 2;
            frame_idx = 0;
            break;

        case 2:  /* Got LEN, reading OPCODE + PAYLOAD */
            if (frame_idx == 0) {
                frame_opcode = byte;
            } else {
                frame_buf[frame_idx - 1] = byte;
            }
            frame_idx++;
            if (frame_idx >= frame_len) {
                frame_state = 3;
            }
            break;

        case 3:  /* Got payload, expecting CRC */
            {
                uint8_t crc_buf[2 + 128];
                crc_buf[0] = frame_len;
                crc_buf[1] = frame_opcode;
                for (uint8_t i = 0; i < frame_len - 1; i++)
                    crc_buf[2 + i] = frame_buf[i];
                uint8_t expected_crc = compute_crc(crc_buf, 2 + frame_len - 1);
                if (byte == expected_crc) {
                    frame_state = 4;
                } else {
                    frame_state = 0;  /* CRC error, discard */
                }
            }
            break;

        case 4:  /* Got CRC, expecting EOF */
            if (byte == BLE_FRAME_EOF && cmd_handler) {
                cmd_handler(frame_opcode, frame_buf, frame_len - 1);
            }
            frame_state = 0;
            return 1;

        default:
            frame_state = 0;
            break;
        }
    }
    return 0;
}

void ble_set_handler(ble_cmd_handler_t handler)
{
    cmd_handler = handler;
}

void ble_stream_sample(const uint8_t *data, uint8_t len)
{
    ble_send(BLE_RSP_STREAM, data, len);
}

void ble_send_log_chunk(uint32_t offset, const uint8_t *data, uint8_t len)
{
    uint8_t payload[133];
    payload[0] = (offset >> 24) & 0xFF;
    payload[1] = (offset >> 16) & 0xFF;
    payload[2] = (offset >> 8) & 0xFF;
    payload[3] = offset & 0xFF;
    if (len > 128) len = 128;
    for (uint8_t i = 0; i < len; i++)
        payload[4 + i] = data[i];
    ble_send(BLE_RSP_LOG_CHUNK, payload, 4 + len);
}

uint8_t ble_is_connected(void)
{
    return ble_connected;
}