// usb_protocol.h — USB Protocol Handler Header
#ifndef USB_PROTOCOL_H
#define USB_PROTOCOL_H
#include <stdint.h>
#include <stdbool.h>

// Command IDs
#define CMD_PING                0x01
#define CMD_GET_INFO            0x02
#define CMD_RESET               0x03
#define CMD_ACQ_START           0x10
#define CMD_ACQ_STOP            0x11
#define CMD_ACQ_STATUS          0x12
#define CMD_ACQ_DATA            0x13
#define CMD_TDC_SINGLE          0x20
#define CMD_TDC_CALIBRATE       0x21
#define CMD_TDC_GET_CAL         0x22
#define CMD_PULSE_CONFIG        0x30
#define CMD_PULSE_SINGLE        0x31
#define CMD_VGA_SET_GAIN        0x40
#define CMD_TEMP_READ           0x50
#define CMD_LED_SET             0x60
#define CMD_FLASH_READ          0x70
#define CMD_FLASH_WRITE         0x71
#define CMD_BOOTLOADER          0xF0
#define CMD_NACK                0xFF

// Error codes
#define ERR_NONE                0x00
#define ERR_BUSY                0x01
#define ERR_TIMEOUT             0x02
#define ERR_OVERFLOW            0x03
#define ERR_PARAM               0x04
#define ERR_STATE               0x05
#define ERR_HARDWARE            0x06
#define ERR_CRC                 0x07
#define ERR_OVERTEMP            0x08
#define ERR_NOT_CALIBRATED      0x09
#define ERR_UNKNOWN             0xFF

#define USB_SYNC_CMD            0xAA
#define USB_SYNC_RESP           0x55
#define USB_MAX_PAYLOAD         1024

typedef struct {
    uint8_t  command_id;
    uint16_t payload_length;
    uint8_t  payload[USB_MAX_PAYLOAD];
} usb_command_t;

int  usb_protocol_init(void);
int  usb_protocol_poll(usb_command_t *cmd);
int  usb_protocol_send_response(uint8_t error_code, const uint8_t *data, uint16_t len);
int  usb_protocol_send_data_packet(const uint8_t *data, uint32_t len);
bool usb_is_connected(void);
#endif
