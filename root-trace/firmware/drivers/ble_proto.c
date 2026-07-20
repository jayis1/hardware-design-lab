/*
 * ble_proto.c — BLE Protocol Handler
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Manages UART communication with the STM32WB55 BLE co-processor.
 * Implements a simple framed protocol with CRC-16 for reliability.
 *
 * Packet format (UART):
 *   Offset  Size  Field
 *   0x00    1     Start byte: 0xAA
 *   0x01    1     Opcode
 *   0x02    1     Status (0 for command, nonzero for response)
 *   0x03    1     Sequence number
 *   0x04    2     Payload length (little-endian, max 240)
 *   0x06    N     Payload (N = length)
 *   0x06+N  2     CRC-16 (CCITT, little-endian)
 *   0x08+N 1     End byte: 0x55
 *
 * The co-processor forwards BLE GATT writes as command packets and
 * expects response packets to send back via GATT notifications.
 */

#include "ble_proto.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* CRC-16 CCITT (0x1021 polynomial) */
static uint16_t crc16_update(uint16_t crc, uint8_t data)
{
    crc ^= (uint16_t)data << 8;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x8000)
            crc = (crc << 1) ^ 0x1021;
        else
            crc <<= 1;
    }
    return crc;
}

static uint16_t crc16_compute(const uint8_t *data, int len)
{
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < len; i++)
        crc = crc16_update(crc, data[i]);
    return crc;
}

/* ---------------------------------------------------------------------
 * UART ring buffer (256 bytes)
 * --------------------------------------------------------------------- */

#define UART_BUF_SIZE 256
static volatile uint8_t s_rx_buf[UART_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;
static volatile uint8_t s_rx_overflow = 0;

static volatile int s_connected = 0;
static uint8_t s_tx_seq = 0;

/* ---------------------------------------------------------------------
 * USART2 initialization (BLE co-processor link)
 * --------------------------------------------------------------------- */

static void ble_uart_init(void)
{
    RCC->AHB4ENR |= BIT(0) | BIT(3);  /* GPIOA, GPIOD */
    RCC->APB1LENR |= BIT(17);  /* USART2 */

    /* PA2 = TX (AF7), PA3 = RX (AF7) */
    int pins[] = {2, 3};
    for (int i = 0; i < 2; i++) {
        int p = pins[i];
        GPIOA->MODER &= ~(3U << (p*2));
        GPIOA->MODER |= (GPIO_MODE_AF << (p*2));
        GPIOA->OSPEEDR |= (GPIO_SPEED_HIGH << (p*2));
        GPIOA->AFRL &= ~(0xFU << (p*4));
        GPIOA->AFRL |= (BLE_UART_AF << (p*4));
    }

    /* PD3 = CTS, PD4 = RTS (AF7) */
    int pd_pins[] = {3, 4};
    for (int i = 0; i < 2; i++) {
        int p = pd_pins[i];
        GPIOD->MODER &= ~(3U << (p*2));
        GPIOD->MODER |= (GPIO_MODE_AF << (p*2));
        GPIOD->OSPEEDR |= (GPIO_SPEED_HIGH << (p*2));
        GPIOD->AFRL &= ~(0xFU << (p*4));
        GPIOD->AFRL |= (BLE_UART_AF << (p*4));
    }

    /* PG10 = BLE RESET (output) */
    RCC->AHB4ENR |= BIT(6);  /* GPIOG */
    GPIOG->MODER &= ~(3U << (10*2));
    GPIOG->MODER |= (GPIO_MODE_OUT << (10*2));
    PIN_SET(BLE_RESET_PORT, BLE_RESET_PIN);

    /* Configure USART2: 921600 baud, 8N1, with RTS/CTS flow control */
    BLE_UART->BRR = BOARD_PCLK1_HZ / BLE_BAUD;
    BLE_UART->CR2 = 0;  /* 1 stop bit */
    BLE_UART->CR3 = BIT(8) | BIT(9);  /* RTSE, CTSE */
    BLE_UART->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE
                  | USART_CR1_RXNEIE;

    /* Enable USART2 interrupt in NVIC */
    NVIC_ISER0 |= BIT(USART2_IRQn - 0);
}

/* ---------------------------------------------------------------------
 * USART2 interrupt handler
 * --------------------------------------------------------------------- */

void USART2_IRQHandler(void) __attribute__((interrupt("IRQ")));
void USART2_IRQHandler(void)
{
    if (BLE_UART->ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)BLE_UART->RDR;
        uint16_t next = (s_rx_head + 1) % UART_BUF_SIZE;
        if (next != s_rx_tail) {
            s_rx_buf[s_rx_head] = byte;
            s_rx_head = next;
        } else {
            s_rx_overflow = 1;
        }
    }
    /* Clear flags */
    BLE_UART->ICR = 0xFFFFFFFF;
}

/* ---------------------------------------------------------------------
 * Send a single byte via UART (blocking with timeout)
 * --------------------------------------------------------------------- */

static void uart_send_byte(uint8_t b)
{
    uint32_t timeout = 1000000;
    while (!(BLE_UART->ISR & USART_ISR_TXE) && timeout-- > 0) { }
    BLE_UART->TDR = b;
}

static void uart_send(const uint8_t *data, int len)
{
    for (int i = 0; i < len; i++)
        uart_send_byte(data[i]);
}

/* ---------------------------------------------------------------------
 * Read a byte from the ring buffer (non-blocking)
 * --------------------------------------------------------------------- */

static int uart_read_byte(uint8_t *b)
{
    if (s_rx_head == s_rx_tail) return -1;  /* empty */
    *b = s_rx_buf[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1) % UART_BUF_SIZE;
    return 0;
}

/* ---------------------------------------------------------------------
 * Packet parser state machine
 * --------------------------------------------------------------------- */

typedef enum {
    PKT_IDLE,
    PKT_START,
    PKT_OPCODE,
    PKT_STATUS,
    PKT_SEQ,
    PKT_LEN_LO,
    PKT_LEN_HI,
    PKT_PAYLOAD,
    PKT_CRC_LO,
    PKT_CRC_HI,
    PKT_END,
} pkt_state_t;

static pkt_state_t s_pkt_state = PKT_IDLE;
static uint8_t s_pkt_opcode = 0;
static uint8_t s_pkt_status = 0;
static uint8_t s_pkt_seq = 0;
static uint16_t s_pkt_len = 0;
static uint16_t s_pkt_idx = 0;
static uint8_t s_pkt_payload[240];
static uint16_t s_pkt_crc = 0;
static uint16_t s_pkt_crc_calc = 0;

static ble_cmd_t s_last_cmd;
static volatile int s_cmd_ready = 0;

static void process_byte(uint8_t b)
{
    switch (s_pkt_state) {
    case PKT_IDLE:
        if (b == 0xAA) {
            s_pkt_state = PKT_START;
            s_pkt_crc_calc = 0xFFFF;
            s_pkt_crc_calc = crc16_update(s_pkt_crc_calc, b);
        }
        break;
    case PKT_START:
        s_pkt_opcode = b;
        s_pkt_crc_calc = crc16_update(s_pkt_crc_calc, b);
        s_pkt_state = PKT_OPCODE;
        break;
    case PKT_OPCODE:
        s_pkt_status = b;
        s_pkt_crc_calc = crc16_update(s_pkt_crc_calc, b);
        s_pkt_state = PKT_STATUS;
        break;
    case PKT_STATUS:
        s_pkt_seq = b;
        s_pkt_crc_calc = crc16_update(s_pkt_crc_calc, b);
        s_pkt_state = PKT_SEQ;
        break;
    case PKT_SEQ:
        s_pkt_len = b;
        s_pkt_crc_calc = crc16_update(s_pkt_crc_calc, b);
        s_pkt_state = PKT_LEN_LO;
        break;
    case PKT_LEN_LO:
        s_pkt_len |= (b << 8);
        s_pkt_crc_calc = crc16_update(s_pkt_crc_calc, b);
        s_pkt_idx = 0;
        if (s_pkt_len > 240) {
            s_pkt_state = PKT_IDLE;  /* invalid */
        } else if (s_pkt_len == 0) {
            s_pkt_state = PKT_CRC_LO;
        } else {
            s_pkt_state = PKT_PAYLOAD;
        }
        break;
    case PKT_PAYLOAD:
        s_pkt_payload[s_pkt_idx++] = b;
        s_pkt_crc_calc = crc16_update(s_pkt_crc_calc, b);
        if (s_pkt_idx >= s_pkt_len)
            s_pkt_state = PKT_CRC_LO;
        break;
    case PKT_CRC_LO:
        s_pkt_crc = b;
        s_pkt_state = PKT_CRC_HI;
        break;
    case PKT_CRC_HI:
        s_pkt_crc |= (b << 8);
        s_pkt_state = PKT_END;
        break;
    case PKT_END:
        if (b == 0x55 && s_pkt_crc == s_pkt_crc_calc) {
            /* Valid packet received */
            s_last_cmd.opcode = s_pkt_opcode;
            s_last_cmd.seq = s_pkt_seq;
            s_last_cmd.length = s_pkt_len;
            if (s_pkt_len > 0)
                memcpy(s_last_cmd.payload, s_pkt_payload, s_pkt_len);
            s_cmd_ready = 1;
        }
        s_pkt_state = PKT_IDLE;
        break;
    }
}

/* ---------------------------------------------------------------------
 * Command handler registry
 * --------------------------------------------------------------------- */

#define MAX_HANDLERS 32
static ble_cmd_handler_t s_handlers[MAX_HANDLERS];

void ble_proto_register_handler(uint8_t opcode, ble_cmd_handler_t handler)
{
    if (opcode < MAX_HANDLERS)
        s_handlers[opcode] = handler;
}

/* ---------------------------------------------------------------------
 * Send a response packet
 * --------------------------------------------------------------------- */

int ble_proto_send_response(const ble_resp_t *resp)
{
    uint8_t pkt[6 + 240 + 3];
    int idx = 0;
    uint16_t crc = 0xFFFF;

    pkt[idx++] = 0xAA;
    pkt[idx++] = resp->opcode;
    pkt[idx++] = resp->status;
    pkt[idx++] = resp->seq;
    pkt[idx++] = resp->length & 0xFF;
    pkt[idx++] = (resp->length >> 8) & 0xFF;
    for (int i = 0; i < resp->length; i++)
        pkt[idx++] = resp->payload[i];

    /* Compute CRC over all bytes so far */
    crc = crc16_compute(pkt, idx);
    pkt[idx++] = crc & 0xFF;
    pkt[idx++] = (crc >> 8) & 0xFF;
    pkt[idx++] = 0x55;

    uart_send(pkt, idx);
    return 0;
}

/* ---------------------------------------------------------------------
 * Send an EIT frame (chunked across multiple packets)
 * --------------------------------------------------------------------- */

int ble_proto_send_frame(const eit_frame_t *frame)
{
    if (!frame) return -1;

    /* Send frame header */
    ble_resp_t resp;
    resp.opcode = BLE_CMD_GET_FRAME;
    resp.status = BLE_RESP_OK;
    resp.seq = s_tx_seq++;
    resp.length = 16;
    resp.payload[0] = (frame->frame_seq) & 0xFF;
    resp.payload[1] = (frame->frame_seq >> 8) & 0xFF;
    *(uint32_t *)&resp.payload[2] = frame->timestamp_ms;
    resp.payload[6] = frame->freq_index;
    resp.payload[7] = frame->status;
    *(uint16_t *)&resp.payload[8] = frame->electrode_contact_mask;
    /* Reconstruction summary: mean conductivity */
    float mean_cond = 0;
    for (int i = 0; i < EIT_MESH_NODES; i++)
        mean_cond += frame->conductivity[i];
    mean_cond /= EIT_MESH_NODES;
    memcpy(&resp.payload[10], &mean_cond, 4);
    ble_proto_send_response(&resp);

    /* Send measurement data in chunks of 180 bytes */
    int offset = 0;
    int total = EIT_FRAME_SIZE * 20;  /* 4160 bytes */
    while (offset < total) {
        int chunk = MIN(BLE_FRAME_CHUNK_SIZE, total - offset);
        resp.opcode = BLE_CMD_GET_FRAME;
        resp.status = 0x01;  /* data chunk */
        resp.seq = s_tx_seq++;
        resp.length = chunk;
        memcpy(resp.payload, (const uint8_t *)frame->meas + offset, chunk);
        ble_proto_send_response(&resp);
        offset += chunk;
    }

    /* Send conductivity array in chunks */
    offset = 0;
    total = EIT_MESH_NODES * 4;  /* 2304 bytes */
    while (offset < total) {
        int chunk = MIN(BLE_FRAME_CHUNK_SIZE, total - offset);
        resp.opcode = BLE_CMD_GET_FRAME;
        resp.status = 0x02;  /* conductivity chunk */
        resp.seq = s_tx_seq++;
        resp.length = chunk;
        memcpy(resp.payload, (const uint8_t *)frame->conductivity + offset, chunk);
        ble_proto_send_response(&resp);
        offset += chunk;
    }

    /* Send end marker */
    resp.opcode = BLE_CMD_GET_FRAME;
    resp.status = 0xFF;  /* end of frame */
    resp.seq = s_tx_seq++;
    resp.length = 0;
    ble_proto_send_response(&resp);

    return 0;
}

int ble_proto_send_image(const float *image, int w, int h)
{
    if (!image) return -1;
    /* Send a downsampled 64×64 image (quantized to 8-bit) */
    ble_resp_t resp;
    resp.opcode = BLE_CMD_GET_FRAME;
    resp.status = 0x03;  /* image data */
    resp.seq = s_tx_seq++;
    resp.length = 64;  /* one row at a time */
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            int sx = (int)((float)x / 64 * w);
            int sy = (int)((float)y / 64 * h);
            if (sx >= w) sx = w - 1;
            if (sy >= h) sy = h - 1;
            float val = image[sy * w + sx];
            int q = (int)(val * 255.0f);
            if (q < 0) q = 0;
            if (q > 255) q = 255;
            resp.payload[x] = (uint8_t)q;
        }
        ble_proto_send_response(&resp);
        resp.seq = s_tx_seq++;
    }
    return 0;
}

int ble_proto_is_connected(void) { return s_connected; }

void ble_proto_reset_co_processor(void)
{
    PIN_CLR(BLE_RESET_PORT, BLE_RESET_PIN);
    for (volatile int i = 0; i < 100000; i++) { /* 1ms */ }
    PIN_SET(BLE_RESET_PORT, BLE_RESET_PIN);
    for (volatile int i = 0; i < 1000000; i++) { /* 10ms */ }
}

/* ---------------------------------------------------------------------
 * Process incoming commands (called from main loop)
 * --------------------------------------------------------------------- */

void ble_proto_process(void)
{
    /* Read all available bytes and process them */
    uint8_t b;
    while (uart_read_byte(&b) == 0) {
        process_byte(b);
    }

    /* If a command is ready, dispatch to handler */
    if (s_cmd_ready) {
        s_cmd_ready = 0;
        ble_cmd_t *cmd = &s_last_cmd;

        /* Handle connection state commands */
        if (cmd->opcode == BLE_CMD_PING) {
            ble_resp_t resp = {
                .opcode = BLE_CMD_PING,
                .status = BLE_RESP_OK,
                .seq = cmd->seq,
                .length = 4,
            };
            resp.payload[0] = 'R';
            resp.payload[1] = 'T';
            resp.payload[2] = '0';
            resp.payload[3] = '1';
            s_connected = 1;
            ble_proto_send_response(&resp);
            return;
        }

        /* Dispatch to registered handler */
        if (cmd->opcode < MAX_HANDLERS && s_handlers[cmd->opcode]) {
            ble_resp_t resp;
            memset(&resp, 0, sizeof(resp));
            resp.opcode = cmd->opcode;
            resp.seq = cmd->seq;
            int result = s_handlers[cmd->opcode](cmd, &resp);
            ble_proto_send_response(&resp);
            (void)result;
        } else {
            /* Unknown command: send error response */
            ble_resp_t resp = {
                .opcode = cmd->opcode,
                .status = BLE_RESP_ERR,
                .seq = cmd->seq,
                .length = 1,
            };
            resp.payload[0] = 0x01;  /* unknown command */
            ble_proto_send_response(&resp);
        }
    }
}

void ble_proto_init(void)
{
    memset(s_handlers, 0, sizeof(s_handlers));
    ble_uart_init();
    ble_proto_reset_co_processor();
    s_pkt_state = PKT_IDLE;
    s_connected = 0;
}