// usb_protocol.c — USB Binary Protocol Implementation
// Frame-based protocol over USB bulk endpoints with CRC-16
// Handles command parsing, response framing, and DMA transfers

#include "usb_protocol.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

static volatile uint32_t *usb_base = (volatile uint32_t *)USB_BASE;
static bool driver_initialized = false;
static uint8_t rx_buffer[USB_MAX_PAYLOAD + 4];  // +4 for header
static uint8_t tx_buffer[USB_MAX_PAYLOAD + 4];

// CRC-16-CCITT lookup table
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

static uint16_t crc16_compute(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ data[i]) & 0xFF];
    }
    return crc;
}

int usb_protocol_init(void) {
    if (driver_initialized) return 0;

    // Reset USB bridge
    REG_SET_BITS(SYS_RESET_REG, SYS_RESET_USB);
    for (volatile int d = 0; d < 100; d++) NOP();
    REG_CLR_BITS(SYS_RESET_REG, SYS_RESET_USB);

    // Reset FT601
    REG_SET_BITS(USB_CTRL_REG, USB_CTRL_FT_RESET);
    for (volatile int d = 0; d < 10000; d++) NOP();
    REG_CLR_BITS(USB_CTRL_REG, USB_CTRL_FT_RESET);

    // Enable USB bridge
    REG_WRITE(USB_CTRL_REG, USB_CTRL_ENABLE);

    // Wait for FT601 to configure
    uint32_t timeout = 50000000;  // ~500 ms
    while (timeout > 0) {
        if (REG_READ(USB_STATUS_REG) & USB_STATUS_CONFIGURED) break;
        timeout--;
    }

    driver_initialized = true;
    return 0;
}

int usb_protocol_poll(usb_command_t *cmd) {
    if (!driver_initialized) return -1;

    // Check for incoming data on RX endpoint
    uint32_t status = REG_READ(USB_STATUS_REG);
    if (status & USB_STATUS_RXF) return -2;  // RX FIFO full — shouldn't happen

    // Check EP0 for setup packets
    uint32_t ep0_ctrl = REG_READ(USB_EP0_CTRL_REG);
    if (ep0_ctrl & USB_EP0_SETUP_RCVD) {
        // Handle control transfer
        uint32_t ep0_data = REG_READ(USB_EP0_DATA_REG);
        REG_WRITE(USB_EP0_CTRL_REG, 0);  // Clear setup received
        // Control transfers handled separately
    }

    // Try to receive a command frame via bulk endpoint
    // Set up RX DMA
    REG_WRITE(USB_RX_BASE_REG, DMA_USB_RX_BUFFER);
    REG_WRITE(USB_RX_LEN_REG, USB_MAX_PAYLOAD + 4);
    REG_WRITE(USB_RX_START_REG, 1);

    // Wait for RX DMA to complete
    uint32_t timeout = 100000;  // ~1 ms
    while (timeout > 0) {
        uint32_t rx_status = REG_READ(USB_RX_STATUS_REG);
        if (rx_status & USB_DMA_DONE) break;
        if (rx_status & USB_DMA_ERROR) return -3;
        timeout--;
    }

    if (timeout == 0) {
        // No data available — that's fine
        return -4;
    }

    // Copy received data from DMA buffer
    volatile uint8_t *dma_buf = (volatile uint8_t *)DMA_USB_RX_BUFFER;
    uint32_t rx_len = REG_READ(USB_RX_LEN_REG);

    if (rx_len < 4) return -5;  // Frame too short

    // Check sync byte
    if (dma_buf[0] != USB_SYNC_CMD) return -6;

    // Parse header
    cmd->command_id = dma_buf[1];
    cmd->payload_length = dma_buf[2] | (dma_buf[3] << 8);

    if (cmd->payload_length > USB_MAX_PAYLOAD) return -7;

    // Copy payload
    for (uint16_t i = 0; i < cmd->payload_length; i++) {
        cmd->payload[i] = dma_buf[4 + i];
    }

    // Verify CRC
    uint16_t received_crc = dma_buf[4 + cmd->payload_length]
                          | (dma_buf[5 + cmd->payload_length] << 8);
    uint16_t computed_crc = crc16_compute(dma_buf + 1, 3 + cmd->payload_length);

    if (received_crc != computed_crc) {
        usb_protocol_send_response(ERR_CRC, NULL, 0);
        return -8;
    }

    return 0;
}

int usb_protocol_send_response(uint8_t error_code, const uint8_t *data, uint16_t len) {
    if (!driver_initialized) return -1;

    if (len > USB_MAX_PAYLOAD) return -2;

    // Build response frame in TX buffer
    tx_buffer[0] = USB_SYNC_RESP;
    tx_buffer[1] = error_code;
    tx_buffer[2] = len & 0xFF;
    tx_buffer[3] = (len >> 8) & 0xFF;

    if (data && len > 0) {
        memcpy(tx_buffer + 4, data, len);
    }

    // Compute CRC over bytes 1 through 3+len
    uint16_t crc = crc16_compute(tx_buffer + 1, 3 + len);
    tx_buffer[4 + len] = crc & 0xFF;
    tx_buffer[5 + len] = (crc >> 8) & 0xFF;

    uint16_t total_len = 6 + len;

    // Copy to DMA buffer and send
    volatile uint8_t *dma_buf = (volatile uint8_t *)DMA_USB_TX_BUFFER;
    for (uint16_t i = 0; i < total_len; i++) {
        dma_buf[i] = tx_buffer[i];
    }

    REG_WRITE(USB_TX_BASE_REG, DMA_USB_TX_BUFFER);
    REG_WRITE(USB_TX_LEN_REG, total_len);
    REG_WRITE(USB_TX_START_REG, 1);

    // Wait for TX DMA to complete
    uint32_t timeout = 100000;
    while (timeout > 0) {
        uint32_t tx_status = REG_READ(USB_TX_STATUS_REG);
        if (tx_status & USB_DMA_DONE) return 0;
        if (tx_status & USB_DMA_ERROR) return -3;
        timeout--;
    }

    return -4;
}

int usb_protocol_send_data_packet(const uint8_t *data, uint32_t len) {
    if (!driver_initialized) return -1;
    if (len > (64 * 1024)) return -2;  // Max 64 KB per packet

    // Copy to DMA buffer
    volatile uint8_t *dma_buf = (volatile uint8_t *)DMA_USB_TX_BUFFER;
    for (uint32_t i = 0; i < len; i++) {
        dma_buf[i] = data[i];
    }

    REG_WRITE(USB_TX_BASE_REG, DMA_USB_TX_BUFFER);
    REG_WRITE(USB_TX_LEN_REG, len);
    REG_WRITE(USB_TX_START_REG, 1);

    uint32_t timeout = len * 10;  // Scale timeout with data size
    while (timeout > 0) {
        uint32_t tx_status = REG_READ(USB_TX_STATUS_REG);
        if (tx_status & USB_DMA_DONE) return 0;
        if (tx_status & USB_DMA_ERROR) return -3;
        timeout--;
    }

    return -4;
}

bool usb_is_connected(void) {
    if (!driver_initialized) return false;
    uint32_t status = REG_READ(USB_STATUS_REG);
    return (status & USB_STATUS_CONFIGURED) ? true : false;
}
