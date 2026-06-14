// ============================================================================
// drivers/ble_uart.c — nRF52832 BLE UART Communication Driver Implementation
// ============================================================================

#include "ble_uart.h"
#include "registers.h"
#include "board.h"
#include <string.h>

// ---- RX Ring Buffer ----
static volatile uint8_t rx_buffer[BLE_RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

// ---- USART2 RX Interrupt Handler ----
void USART2_IRQHandler(void) {
    if (USART2->ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)USART2->RDR;
        uint16_t next_head = (rx_head + 1) % BLE_RX_BUF_SIZE;
        if (next_head != rx_tail) {
            rx_buffer[rx_head] = byte;
            rx_head = next_head;
        }
        // Overflow: drop byte
    }
}

// ---- CRC16-CCITT ----
static uint16_t crc16_ccitt(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

// ---- Read one byte from RX buffer ----
static int rx_read_byte(uint8_t *byte) {
    if (rx_head == rx_tail) return -1;  // Empty
    *byte = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % BLE_RX_BUF_SIZE;
    return 0;
}

// ---- Init ----
void ble_uart_init(void) {
    // Reset BLE module
    BLE_RESET_LOW();
    for (volatile int i = 0; i < 100000; i++);
    BLE_RESET_HIGH();
    for (volatile int i = 0; i < 500000; i++);

    // Configure USART2: 1 Mbps, 8N1
    // BRR = APB1_CLK / baud = 170000000 / 1000000 = 170
    USART2->BRR = 170;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    
    // Enable USART2 interrupt in NVIC
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_SetPriority(USART2_IRQn, 6);  // Medium priority
}

// ---- Send byte ----
static void uart_putc(uint8_t c) {
    while (!(USART2->ISR & USART_ISR_TXE));
    USART2->TDR = c;
}

// ---- Send Frame ----
int ble_uart_send_frame(const ble_frame_t *frame) {
    // Send header
    uart_putc(frame->header);
    uart_putc(frame->cmd_id);
    uart_putc(frame->length);
    
    // Send payload
    for (uint8_t i = 0; i < frame->length; i++) {
        uart_putc(frame->payload[i]);
    }
    
    // Send CRC16 (little-endian)
    uart_putc(frame->crc16 & 0xFF);
    uart_putc((frame->crc16 >> 8) & 0xFF);
    
    return 0;
}

// ---- Send Status Response ----
int ble_uart_send_status(float vbus, float iq, float omega, uint8_t faults, uint8_t running) {
    ble_frame_t frame;
    frame.header = BLE_FRAME_HEADER;
    frame.cmd_id = BLE_RSP_STATUS;
    frame.length = 10;
    
    // Pack status
    uint16_t vbus_10mv = (uint16_t)(vbus * 100.0f);
    int16_t iq_100ma = (int16_t)(iq * 10.0f);
    int16_t omega_10 = (int16_t)(omega * 10.0f);
    
    frame.payload[0] = vbus_10mv & 0xFF;
    frame.payload[1] = (vbus_10mv >> 8) & 0xFF;
    frame.payload[2] = iq_100ma & 0xFF;
    frame.payload[3] = (iq_100ma >> 8) & 0xFF;
    frame.payload[4] = omega_10 & 0xFF;
    frame.payload[5] = (omega_10 >> 8) & 0xFF;
    frame.payload[6] = faults;
    frame.payload[7] = running;
    frame.payload[8] = 0;  // reserved
    frame.payload[9] = 0;  // reserved
    
    // Compute CRC over cmd_id + length + payload
    uint8_t crc_buf[2 + BLE_FRAME_MAX_PAYLOAD];
    crc_buf[0] = frame.cmd_id;
    crc_buf[1] = frame.length;
    memcpy(&crc_buf[2], frame.payload, frame.length);
    frame.crc16 = crc16_ccitt(crc_buf, 2 + frame.length);
    
    return ble_uart_send_frame(&frame);
}

// ---- Frame Parser State Machine ----
typedef enum {
    PARSE_HEADER = 0,
    PARSE_CMD_ID,
    PARSE_LENGTH,
    PARSE_PAYLOAD,
    PARSE_CRC_LO,
    PARSE_CRC_HI
} parse_state_t;

static parse_state_t parse_state = PARSE_HEADER;
static ble_frame_t rx_frame;
static uint8_t payload_idx = 0;
static uint8_t crc_lo = 0;

void ble_uart_process(void) {
    uint8_t byte;
    while (rx_read_byte(&byte) == 0) {
        switch (parse_state) {
        case PARSE_HEADER:
            if (byte == BLE_FRAME_HEADER) {
                rx_frame.header = byte;
                parse_state = PARSE_CMD_ID;
            }
            break;
            
        case PARSE_CMD_ID:
            rx_frame.cmd_id = byte;
            parse_state = PARSE_LENGTH;
            break;
            
        case PARSE_LENGTH:
            rx_frame.length = byte;
            if (rx_frame.length > BLE_FRAME_MAX_PAYLOAD) {
                parse_state = PARSE_HEADER;  // Invalid, reset
            } else {
                payload_idx = 0;
                parse_state = (rx_frame.length > 0) ? PARSE_PAYLOAD : PARSE_CRC_LO;
            }
            break;
            
        case PARSE_PAYLOAD:
            rx_frame.payload[payload_idx++] = byte;
            if (payload_idx >= rx_frame.length) {
                parse_state = PARSE_CRC_LO;
            }
            break;
            
        case PARSE_CRC_LO:
            crc_lo = byte;
            parse_state = PARSE_CRC_HI;
            break;
            
        case PARSE_CRC_HI:
            rx_frame.crc16 = (uint16_t)crc_lo | ((uint16_t)byte << 8);
            
            // Validate CRC
            uint8_t crc_buf[2 + BLE_FRAME_MAX_PAYLOAD];
            crc_buf[0] = rx_frame.cmd_id;
            crc_buf[1] = rx_frame.length;
            memcpy(&crc_buf[2], rx_frame.payload, rx_frame.length);
            uint16_t calc_crc = crc16_ccitt(crc_buf, 2 + rx_frame.length);
            
            if (calc_crc == rx_frame.crc16) {
                ble_on_frame_received(&rx_frame);
            }
            // else: CRC fail, silently drop
            
            parse_state = PARSE_HEADER;
            break;
        }
    }
}

// ---- Weak default handler (override in main or application) ----
__attribute__((weak)) void ble_on_frame_received(const ble_frame_t *frame) {
    (void)frame;
    // Default: send ACK
    ble_frame_t ack;
    ack.header = BLE_FRAME_HEADER;
    ack.cmd_id = BLE_RSP_ACK;
    ack.length = 1;
    ack.payload[0] = frame->cmd_id;
    
    uint8_t crc_buf[3];
    crc_buf[0] = ack.cmd_id;
    crc_buf[1] = ack.length;
    crc_buf[2] = ack.payload[0];
    ack.crc16 = crc16_ccitt(crc_buf, 3);
    
    ble_uart_send_frame(&ack);
}