/*
 * ble.c — BLE 5.2 communication via CC2640R2F co-processor for HivePulse
 *
 * The CC2640R2F handles BLE 5.2 protocol while the STM32H733 handles
 * application logic. They communicate over UART4 with hardware flow control.
 *
 * Protocol: Frame-based UART with CRC-8
 *   [SOF][LEN][CMD][DATA...][CRC][EOF]
 *   SOF = 0xAA, EOF = 0x55, LEN = payload length (CMD + DATA + CRC)
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "ble.h"
#include "board.h"
#include "registers.h"
#include "audio.h"
#include <string.h>

/* ---- UART Protocol Constants ---- */
#define BLE_SOF           0xAA
#define BLE_EOF           0x55
#define BLE_MAX_PAYLOAD   64
#define BLE_BAUD_RATE     115200
#define BLE_UART_TIMEOUT  1000  /* ms */

/* ---- Command opcodes (STM32 -> CC2640) ---- */
#define BLE_CMD_ADVERTISE   0x01  /* Start advertising */
#define BLE_CMD_STOP_ADV    0x02  /* Stop advertising */
#define BLE_CMD_SEND_DATA   0x03  /* Send data to connected phone */
#define BLE_CMD_DISCONNECT  0x04  /* Disconnect from phone */
#define BLE_CMD_SET_NAME    0x05  /* Set BLE device name */
#define BLE_CMD_DFU_MODE    0x06  /* Enter DFU mode */
#define BLE_CMD_GET_STATUS  0x07  /* Get CC2640 status */

/* ---- Command opcodes (CC2640 -> STM32) ---- */
#define BLE_EVT_CONNECTED    0x81  /* Phone connected */
#define BLE_EVT_DISCONNECTED 0x82  /* Phone disconnected */
#define BLE_EVT_DATA_RX      0x83  /* Data received from phone */
#define BLE_EVT_ADV_DONE     0x84  /* Advertising started */

/* ---- Internal State ---- */
static bool ble_initialized = false;
static bool ble_connected = false;

/* RX buffer for incoming BLE events */
static uint8_t  rx_buffer[BLE_MAX_PAYLOAD + 6];
static uint8_t  rx_index = 0;
static bool     rx_frame_ready = false;
static uint8_t  rx_frame_len = 0;

/* Pending command from phone */
static ble_command_t pending_cmd;
static bool has_pending_cmd = false;

/* ---- CRC-8 Calculation (poly 0x07, init 0x00) ---- */
static uint8_t crc8(const uint8_t *data, int len)
{
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

/* ---- UART4 Initialization ---- */
static int ble_uart_init(void)
{
    /* Enable UART4 clock */
    RCC_APB1LENR |= RCC_APB1LENR_UART4EN;

    /* Configure PC0 (TX), PC1 (RX) as AF8 (UART4) */
    for (int pin = 0; pin <= 3; pin++) {
        GPIOC->MODER &= ~(0x3U << (pin * 2));
        GPIOC->MODER |= (GPIO_MODE_AF << (pin * 2));
        GPIOC->OSPEEDR |= (GPIO_SPEED_HIGH << (pin * 2));
    }
    /* Set AF8 for PC0-PC3 */
    GPIOC->AFR[0] &= ~(0xFFFFU << 0);
    GPIOC->AFR[0] |= (8U << 0) | (8U << 4) | (8U << 8) | (8U << 12);

    /* Configure BLE reset pin (PB5) as output */
    GPIOB->MODER &= ~(0x3U << 10);
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << 10);
    GPIOB->OTYPER &= ~(1U << 5);

    /* Configure UART4: 8N1 with flow control */
    UART4->CR1 = 0;
    UART4->CR2 = 0;
    UART4->CR3 = (1U << 8) | (1U << 9); /* RTSE, CTSE — hardware flow control */
    UART4->BRR = (APB1_FREQ + BLE_BAUD_RATE / 2) / BLE_BAUD_RATE;
    UART4->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;

    /* Enable UART4 interrupt in NVIC */
    NVIC_ISER0 |= (1U << (UART4_IRQn & 0x1F));

    return 0;
}

/* ---- UART4 Interrupt Handler ---- */
void UART4_IRQHandler(void)
{
    if (UART4->ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)UART4->RDR;

        if (rx_index == 0 && byte != BLE_SOF) {
            /* Waiting for start-of-frame */
            return;
        }

        if (rx_index < sizeof(rx_buffer)) {
            rx_buffer[rx_index++] = byte;
        }

        /* Check for complete frame: SOF + LEN + payload + EOF */
        if (rx_index >= 2) {
            uint8_t expected_len = rx_buffer[1];
            uint8_t total_frame = 2 + expected_len + 1; /* SOF + LEN + payload + EOF */

            if (rx_index >= total_frame) {
                /* Check EOF */
                if (rx_buffer[rx_index - 1] == BLE_EOF) {
                    /* Validate CRC */
                    uint8_t expected_crc = crc8(&rx_buffer[2], expected_len - 1);
                    if (expected_crc == rx_buffer[2 + expected_len - 1]) {
                        rx_frame_ready = true;
                        rx_frame_len = expected_len;
                    }
                }
                /* Reset for next frame */
                rx_index = 0;
            }
        }
    }
}

/* ---- UART Send/Receive ---- */
int ble_uart_send(const uint8_t *data, int len)
{
    for (int i = 0; i < len; i++) {
        while (!(UART4->ISR & USART_ISR_TXE));
        UART4->TDR = data[i];
    }
    while (!(UART4->ISR & USART_ISR_TC));
    return 0;
}

int ble_uart_receive(uint8_t *buf, int len, int timeout_ms)
{
    uint32_t start = systick_ms;
    int received = 0;

    while (received < len && (systick_ms - start) < (uint32_t)timeout_ms) {
        if (UART4->ISR & USART_ISR_RXNE) {
            buf[received++] = (uint8_t)UART4->RDR;
        }
    }
    return received;
}

/* ---- Send Frame to CC2640 ---- */
static void ble_send_frame(uint8_t cmd, const uint8_t *data, int data_len)
{
    uint8_t frame[BLE_MAX_PAYLOAD + 6];
    int idx = 0;

    frame[idx++] = BLE_SOF;
    uint8_t payload_len = 1 + data_len + 1; /* CMD + DATA + CRC */
    frame[idx++] = payload_len;
    frame[idx++] = cmd;
    memcpy(&frame[idx], data, data_len);
    idx += data_len;

    /* CRC over CMD + DATA */
    uint8_t crc = crc8(&frame[2], 1 + data_len);
    frame[idx++] = crc;
    frame[idx++] = BLE_EOF;

    ble_uart_send(frame, idx);
}

/* ---- Parse Received Frame ---- */
static void ble_process_frame(const uint8_t *frame, uint8_t len)
{
    uint8_t event = frame[0];

    switch (event) {
    case BLE_EVT_CONNECTED:
        ble_connected = true;
        break;

    case BLE_EVT_DISCONNECTED:
        ble_connected = false;
        break;

    case BLE_EVT_DATA_RX:
        /* Data from phone app — parse as command */
        if (len >= 2 && !has_pending_cmd) {
            pending_cmd.type = (ble_cmd_t)frame[1];
            if (len > 2) {
                pending_cmd.param = frame[2];
                int data_len = len - 3;
                if (data_len > 0 && data_len <= 16) {
                    memcpy(pending_cmd.data, &frame[3], data_len);
                    pending_cmd.data_len = data_len;
                } else {
                    pending_cmd.data_len = 0;
                }
            } else {
                pending_cmd.param = 0;
                pending_cmd.data_len = 0;
            }
            has_pending_cmd = true;
        }
        break;

    case BLE_EVT_ADV_DONE:
        /* Advertising successfully started */
        break;

    default:
        break;
    }
}

/* ---- Public API ---- */
int ble_init(void)
{
    ble_uart_init();

    /* Reset CC2640R2F */
    GPIOB->BSRR = (1U << 5) << 16;  /* Reset low */
    for (volatile int i = 0; i < 100000; i++); /* ~10ms */
    GPIOB->BSRR = (1U << 5);  /* Reset high */
    for (volatile int i = 0; i < 500000; i++); /* ~50ms boot time */

    /* Set BLE device name */
    const char *name = "HivePulse-0001";
    ble_send_frame(BLE_CMD_SET_NAME, (const uint8_t *)name, strlen(name));

    for (volatile int i = 0; i < 100000; i++); /* Wait for response */

    /* Start advertising */
    ble_send_frame(BLE_CMD_ADVERTISE, NULL, 0);

    ble_initialized = true;
    ble_connected = false;
    return 0;
}

void ble_service(void)
{
    if (!ble_initialized) return;

    /* Check for received frames from CC2640 */
    if (rx_frame_ready) {
        __disable_irq();
        uint8_t frame_copy[BLE_MAX_PAYLOAD];
        uint8_t len_copy = rx_frame_len;
        memcpy(frame_copy, &rx_buffer[2], rx_frame_len);
        rx_frame_ready = false;
        __enable_irq();

        ble_process_frame(frame_copy, len_copy);
    }
}

bool ble_get_command(ble_command_t *cmd)
{
    if (!has_pending_cmd) return false;

    __disable_irq();
    *cmd = pending_cmd;
    has_pending_cmd = false;
    __enable_irq();

    return true;
}

void ble_send_sensor_data(const sensor_data_t *data)
{
    if (!ble_connected || !data) return;

    /* Send sensor data in a compressed format:
     * 8 temperatures (each 2 bytes, int16 = temp * 100)
     * weight (2 bytes, int16 = kg * 100)
     * humidity (1 byte, uint8 = %RH)
     * co2 (2 bytes, uint16 ppm)
     * = 16 + 2 + 1 + 2 = 21 bytes */
    uint8_t payload[21];
    int idx = 0;

    for (int i = 0; i < 8; i++) {
        int16_t t = (int16_t)(data->temps[i] * 100);
        payload[idx++] = (uint8_t)(t >> 8);
        payload[idx++] = (uint8_t)(t & 0xFF);
    }

    int16_t w = (int16_t)(data->weight_kg * 100);
    payload[idx++] = (uint8_t)(w >> 8);
    payload[idx++] = (uint8_t)(w & 0xFF);

    payload[idx++] = (uint8_t)data->humidity;

    uint16_t co2 = (uint16_t)data->co2_ppm;
    payload[idx++] = (uint8_t)(co2 >> 8);
    payload[idx++] = (uint8_t)(co2 & 0xFF);

    ble_send_frame(BLE_CMD_SEND_DATA, payload, idx);
}

void ble_send_colony_state(uint8_t state, float confidence)
{
    if (!ble_connected) return;

    uint8_t payload[2];
    payload[0] = state;
    payload[1] = (uint8_t)(confidence * 100); /* 0-100 */
    ble_send_frame(BLE_CMD_SEND_DATA, payload, 2);
}

void ble_send_audio_levels(const float levels[4])
{
    if (!ble_connected) return;

    uint8_t payload[4];
    for (int i = 0; i < 4; i++) {
        /* Convert float level to uint8 (0-255 = 0-1.0) */
        uint32_t val = (uint32_t)(levels[i] * 255.0f);
        payload[i] = (val > 255) ? 255 : (uint8_t)val;
    }
    ble_send_frame(BLE_CMD_SEND_DATA, payload, 4);
}

void ble_enter_dfu(void)
{
    /* Tell CC2640 to enter DFU mode, then STM32 enters its own DFU */
    ble_send_frame(BLE_CMD_DFU_MODE, NULL, 0);

    /* Wait for CC2640 to acknowledge */
    for (volatile int i = 0; i < 1000000; i++);

    /* STM32 enters DFU via USB (if USB is available) or
     * sets a flag in RTC backup register and triggers reset */
    /* In production: enter STM32 system bootloader */
}