// ============================================================================
// drivers/ble_uart.h — nRF52832 BLE UART Communication Driver
// ============================================================================

#ifndef BLE_UART_H
#define BLE_UART_H

#include <stdint.h>

// UART baud rate for BLE communication
#define BLE_UART_BAUD      1000000UL  // 1 Mbps

// BLE protocol command IDs
#define BLE_CMD_PING           0x01
#define BLE_CMD_GET_STATUS     0x02
#define BLE_CMD_SET_THROTTLE   0x10
#define BLE_CMD_SET_BRAKE      0x11
#define BLE_CMD_GET_CONFIG     0x20
#define BLE_CMD_SET_CONFIG     0x21
#define BLE_CMD_SAVE_CONFIG    0x22
#define BLE_CMD_FAULT_CLEAR    0x30
#define BLE_CMD_FW_UPDATE      0x40

// BLE response IDs
#define BLE_RSP_ACK            0x80
#define BLE_RSP_NACK           0x81
#define BLE_RSP_STATUS         0x82
#define BLE_RSP_CONFIG         0x83
#define BLE_RSP_FAULT          0x84

// BLE frame structure
#define BLE_FRAME_HEADER       0xAA
#define BLE_FRAME_MAX_PAYLOAD  128

typedef struct {
    uint8_t  header;      // 0xAA
    uint8_t  cmd_id;
    uint8_t  length;      // Payload length
    uint8_t  payload[BLE_FRAME_MAX_PAYLOAD];
    uint16_t crc16;
} __attribute__((packed)) ble_frame_t;

// Ring buffer for UART RX
#define BLE_RX_BUF_SIZE  256

void ble_uart_init(void);
void ble_uart_process(void);
int ble_uart_send_frame(const ble_frame_t *frame);
int ble_uart_send_status(float vbus, float iq, float omega, uint8_t faults, uint8_t running);

// Callback: called when a valid BLE frame is received
void ble_on_frame_received(const ble_frame_t *frame);

#endif // BLE_UART_H