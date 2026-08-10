/**
 * @file    ble_link.c
 * @brief   TideBand — BLE UART protocol implementation for nRF52840 module.
 *          Handles packet framing, CRC, RX/TX buffering, and dispatches
 *          incoming commands to a registered callback.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * UART: USART1 @ 115200 baud, 8N1, hardware flow control (CTS/RTS).
 * Protocol: [SYNC=0xA5] [OPCODE] [LEN] [PAYLOAD...] [CRC8]
 * CRC8: XOR of all bytes from SYNC through last payload byte.
 */

#include <string.h>
#include "board.h"
#include "registers.h"
#include "ble_link.h"

/* ---- State ---- */
static uint8_t rx_buf[BLE_RX_BUF_SIZE];
static volatile uint16_t rx_head, rx_tail;
static uint8_t tx_buf[BLE_TX_BUF_SIZE];
static volatile uint16_t tx_head, tx_tail;
static volatile uint8_t tx_busy;
static volatile uint8_t ble_connected;
static ble_cmd_callback_t cmd_callback;

/* ---- Local functions ---- */
static uint8_t crc8(const uint8_t *data, uint8_t len);
static void uart_send_byte(uint8_t b);
static void uart_send_packet(const ble_packet_t *pkt);
static void process_rx(void);
static void handle_packet(const ble_packet_t *pkt);

/* ---- Public API ---- */

void ble_link_init(void)
{
    /* Enable USART1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_USART1;

    /* Configure UART pins:
     * PA9 (TX, AF7), PA10 (RX, AF7)
     * PA11 (CTS, AF7), PA12 (RTS, AF7) */
    gpio_set_mode(BLE_TX_GPIO, BLE_TX_PIN, GPIO_MODE_AF);
    gpio_set_af(BLE_TX_GPIO, BLE_TX_PIN, BLE_TX_AF);
    gpio_set_speed(BLE_TX_GPIO, BLE_TX_PIN, GPIO_SPEED_HIGH);

    gpio_set_mode(BLE_RX_GPIO, BLE_RX_PIN, GPIO_MODE_AF);
    gpio_set_af(BLE_RX_GPIO, BLE_RX_PIN, BLE_RX_AF);
    gpio_set_speed(BLE_RX_GPIO, BLE_RX_PIN, GPIO_SPEED_HIGH);

    gpio_set_mode(BLE_CTS_GPIO, BLE_CTS_PIN, GPIO_MODE_AF);
    gpio_set_af(BLE_CTS_GPIO, BLE_CTS_PIN, BLE_CTS_AF);
    gpio_set_mode(BLE_RTS_GPIO, BLE_RTS_PIN, GPIO_MODE_AF);
    gpio_set_af(BLE_RTS_GPIO, BLE_RTS_PIN, BLE_RTS_AF);

    /* BLE reset pin — output, high (not reset) */
    gpio_set_mode(BLE_RESET_GPIO, BLE_RESET_PIN, GPIO_MODE_OUTPUT);
    gpio_set(BLE_RESET_GPIO, BLE_RESET_PIN);

    /* BLE interrupt pin — input */
    gpio_set_mode(BLE_INT_GPIO, BLE_INT_PIN, GPIO_MODE_INPUT);

    /* Configure USART1: 115200 baud, 8N1, hardware flow control
     * BRR = 140 MHz / 115200 = 1215 → mantissa 12, fraction 16 (×16) */
    USART1_BRR = (140000000u / 115200u);

    /* Enable TX, RX, RXNE interrupt, hardware flow control */
    USART1_CR2 = 0;  /* 1 stop bit */
    USART1_CR3 = (1u << 8) | (1u << 9);  /* CTSE + RTSE (hardware flow control) */
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;

    /* Enable USART1 IRQ in NVIC */
    NVIC_ISER0 = (1u << (IRQ_USART1 & 0x1F));

    rx_head = rx_tail = 0;
    tx_head = tx_tail = 0;
    tx_busy = 0;
    ble_connected = 0;
    cmd_callback = NULL;
}

void ble_link_send_profile(const doppler_result_t *doppler,
                           const depth_data_t *depth,
                           const attitude_t *att,
                           uint32_t timestamp)
{
    ble_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.sync = BLE_SYNC_BYTE;
    pkt.opcode = BLE_OP_PROFILE_DATA;

    /* Pack profile data into payload (24 bytes) */
    uint8_t *p = pkt.payload;
    memcpy(p, &timestamp, 4); p += 4;
    memcpy(p, &depth->depth_m, 4); p += 4;
    memcpy(p, &depth->temp_c, 4); p += 4;
    memcpy(p, &doppler->vx, 4); p += 4;
    memcpy(p, &doppler->vy, 4); p += 4;
    memcpy(p, &doppler->vz, 4); p += 4;
    /* Quality packed into 1 byte */
    *p = (doppler->quality & 0x03) | ((doppler->valid & 0x01) << 2);

    pkt.length = 25;  /* 4+4+4+4+4+4+1 */
    pkt.crc = crc8((uint8_t *)&pkt, 3 + pkt.length);

    uart_send_packet(&pkt);
}

void ble_link_send_status(uint8_t battery_pct, uint8_t dive_active,
                          uint16_t dive_count, float depth, float temp,
                          float speed, float heading, uint8_t quality)
{
    ble_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.sync = BLE_SYNC_BYTE;
    pkt.opcode = BLE_OP_STATUS_RSP;

    ble_status_payload_t *pl = (ble_status_payload_t *)pkt.payload;
    pl->battery_pct = battery_pct;
    pl->dive_active = dive_active;
    pl->dive_count = dive_count;
    pl->current_depth_m = depth;
    pl->current_temp_c = temp;
    pl->current_speed_ms = speed;
    pl->current_heading_deg = heading;
    pl->quality = quality;
    pl->free_dive_slots = 0xFFFF;

    pkt.length = sizeof(ble_status_payload_t);
    pkt.crc = crc8((uint8_t *)&pkt, 3 + pkt.length);

    uart_send_packet(&pkt);
}

void ble_link_send_dive_event(uint8_t start, uint32_t timestamp,
                               uint32_t dive_id)
{
    ble_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.sync = BLE_SYNC_BYTE;
    pkt.opcode = start ? BLE_OP_DIVE_START : BLE_OP_DIVE_END;

    uint8_t *p = pkt.payload;
    memcpy(p, &timestamp, 4); p += 4;
    memcpy(p, &dive_id, 4); p += 4;

    pkt.length = 8;
    pkt.crc = crc8((uint8_t *)&pkt, 3 + pkt.length);

    uart_send_packet(&pkt);
}

void ble_link_process(void)
{
    if (rx_head != rx_tail) {
        process_rx();
    }
}

uint8_t ble_link_is_connected(void)
{
    /* The BLE_INT pin goes low when a connection is established */
    return gpio_read(BLE_INT_GPIO, BLE_INT_PIN) == 0 ? 1 : 0;
}

void ble_link_set_callback(ble_cmd_callback_t cb)
{
    cmd_callback = cb;
}

/* ---- Local functions ---- */

static uint8_t crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
    }
    return crc;
}

static void uart_send_byte(uint8_t b)
{
    while ((USART1_ISR & USART_ISR_TXE) == 0) { }
    USART1_TDR = b;
}

static void uart_send_packet(const ble_packet_t *pkt)
{
    /* Send sync, opcode, length */
    uart_send_byte(pkt->sync);
    uart_send_byte(pkt->opcode);
    uart_send_byte(pkt->length);

    /* Send payload */
    for (uint8_t i = 0; i < pkt->length; i++) {
        uart_send_byte(pkt->payload[i]);
    }

    /* Send CRC */
    uart_send_byte(pkt->crc);

    /* Wait for transmission to complete */
    while ((USART1_ISR & USART_ISR_TC) == 0) { }
}

static void process_rx(void)
{
    static ble_packet_t rx_pkt;
    static uint8_t rx_state = 0;
    static uint8_t rx_payload_idx = 0;

    while (rx_head != rx_tail) {
        uint8_t byte = rx_buf[rx_tail];
        rx_tail = (rx_tail + 1) % BLE_RX_BUF_SIZE;

        switch (rx_state) {
            case 0:  /* Waiting for sync */
                if (byte == BLE_SYNC_BYTE) {
                    rx_pkt.sync = byte;
                    rx_state = 1;
                }
                break;
            case 1:  /* Opcode */
                rx_pkt.opcode = byte;
                rx_state = 2;
                break;
            case 2:  /* Length */
                rx_pkt.length = byte;
                rx_payload_idx = 0;
                rx_state = (byte > 0) ? 3 : 4;
                break;
            case 3:  /* Payload */
                rx_pkt.payload[rx_payload_idx++] = byte;
                if (rx_payload_idx >= rx_pkt.length) {
                    rx_state = 4;
                }
                break;
            case 4:  /* CRC */
                rx_pkt.crc = byte;
                rx_state = 0;

                /* Verify CRC */
                uint8_t expected = crc8((uint8_t *)&rx_pkt, 3 + rx_pkt.length);
                if (expected == rx_pkt.crc) {
                    handle_packet(&rx_pkt);
                }
                break;
        }
    }
}

static void handle_packet(const ble_packet_t *pkt)
{
    if (cmd_callback) {
        cmd_callback(pkt->opcode, pkt->payload, pkt->length);
    }

    /* Handle some commands directly */
    switch (pkt->opcode) {
        case BLE_OP_STATUS_REQ:
            /* App is requesting status — main loop will send it */
            break;
        case BLE_OP_SET_RATE:
            /* Set sample rate: payload[0] = rate in Hz (1-4) */
            /* Forwarded to main loop via callback */
            break;
        case BLE_OP_SET_THRESHOLD:
            /* Set haptic threshold: payload as float */
            break;
        case BLE_OP_ERASE_DIVES:
            /* Erase all dive data */
            /* Forwarded to callback for main loop to handle */
            break;
        case BLE_OP_GET_INFO:
            /* Return device info */
            {
                ble_packet_t rsp;
                memset(&rsp, 0, sizeof(rsp));
                rsp.sync = BLE_SYNC_BYTE;
                rsp.opcode = BLE_OP_INFO_RSP;
                /* Payload: version (4 bytes), serial (4 bytes), dives (2 bytes) */
                uint32_t version = 0x01000000u;  /* v1.0.0 */
                uint32_t serial = 0x00000001u;
                uint16_t dives = 0;
                memcpy(&rsp.payload[0], &version, 4);
                memcpy(&rsp.payload[4], &serial, 4);
                memcpy(&rsp.payload[8], &dives, 2);
                rsp.length = 10;
                rsp.crc = crc8((uint8_t *)&rsp, 3 + rsp.length);
                uart_send_packet(&rsp);
            }
            break;
        default:
            break;
    }
}

/* ---- USART1 interrupt handler ---- */
void USART1_IRQHandler(void)
{
    /* RX interrupt */
    if (USART1_ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)USART1_RDR;
        uint16_t next = (rx_head + 1) % BLE_RX_BUF_SIZE;
        if (next != rx_tail) {
            rx_buf[rx_head] = byte;
            rx_head = next;
        }
        /* If buffer is full, the byte is dropped. */
    }

    /* TX interrupt (if enabled) */
    if ((USART1_CR1 & USART_CR1_TXEIE) && (USART1_ISR & USART_ISR_TXE)) {
        if (tx_head != tx_tail) {
            USART1_TDR = tx_buf[tx_tail];
            tx_tail = (tx_tail + 1) % BLE_TX_BUF_SIZE;
        } else {
            /* No more data to send — disable TX interrupt */
            USART1_CR1 &= ~USART_CR1_TXEIE;
            tx_busy = 0;
        }
    }

    /* Clear errors */
    if (USART1_ISR & 0x0Fu) {  /* Overrun, noise, framing, parity errors */
        USART1_ICR = 0x0Fu;
    }
}