/*
 * NeuroLink — USB Descriptors
 * USB 2.0 High-Speed Bulk Device for biosignal data streaming
 * VID: 0x1209 (pid.codes) PID: 0xNL01 (custom)
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* Device descriptors */
#define USB_VID                0x1209   /* pid.codes */
#define USB_PID                0x4E4C   /* "NL" = NeuroLink */
#define USB_DEVICE_VERSION     0x0100   /* v1.0.0 */
#define USB_MANUFACTURER       "NeuroLink Labs"
#define USB_PRODUCT            "NeuroLink Biosignal Acquisition"
#define USB_SERIAL_NUMBER      "NL00000001"

/* USB Device Descriptor */
static const uint8_t usb_device_descriptor[] = {
    18,                         /* bLength */
    0x01,                       /* bDescriptorType: DEVICE */
    0x00, 0x02,                 /* bcdUSB: 2.00 (HS) */
    0x00,                       /* bDeviceClass: Interface-defined */
    0x00,                       /* bDeviceSubClass */
    0x00,                       /* bDeviceProtocol */
    64,                         /* bMaxPacketSize0 (ep0) */
    0x09, 0x12,                 /* idVendor: 0x1209 */
    0x4C, 0x4E,                 /* idProduct: 0x4E4C */
    0x00, 0x01,                 /* bcdDevice: 1.00 */
    0x01,                       /* iManufacturer (string index 1) */
    0x02,                       /* iProduct (string index 2) */
    0x03,                       /* iSerialNumber (string index 3) */
    0x01,                       /* bNumConfigurations: 1 */
};

/* Configuration Descriptor (with interface, endpoints) */
static const uint8_t usb_config_descriptor[] = {
    /* Configuration Descriptor */
    9,                          /* bLength */
    0x02,                       /* bDescriptorType: CONFIGURATION */
    0x20, 0x00,                 /* wTotalLength: 32 bytes */
    0x01,                       /* bNumInterfaces: 1 */
    0x01,                       /* bConfigurationValue: 1 */
    0x00,                       /* iConfiguration */
    0x80,                       /* bmAttributes: bus-powered */
    250,                        /* bMaxPower: 500mA */

    /* Interface Descriptor */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: INTERFACE */
    0x00,                       /* bInterfaceNumber: 0 */
    0x00,                       /* bAlternateSetting: 0 */
    0x02,                       /* bNumEndpoints: 2 (IN + OUT) */
    0xFF,                       /* bInterfaceClass: Vendor */
    0x00,                       /* bInterfaceSubClass */
    0x01,                       /* bInterfaceProtocol: Biosignal */
    0x00,                       /* iInterface */

    /* Endpoint 1 IN (Bulk, biosignal data stream) */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x81,                       /* bEndpointAddress: IN 1 */
    0x02,                       /* bmAttributes: Bulk */
    0x00, 0x02,                 /* wMaxPacketSize: 512 (HS bulk) */
    0x00,                       /* bInterval: N/A for bulk */

    /* Endpoint 2 OUT (Bulk, host commands) */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x02,                       /* bEndpointAddress: OUT 2 */
    0x02,                       /* bmAttributes: Bulk */
    0x00, 0x02,                 /* wMaxPacketSize: 512 (HS bulk) */
    0x00,                       /* bInterval: N/A for bulk */
};

/* Device Qualifier Descriptor (for HS) */
static const uint8_t usb_device_qualifier[] = {
    10,                         /* bLength */
    0x06,                       /* bDescriptorType: DEVICE_QUALIFIER */
    0x00, 0x02,                 /* bcdUSB: 2.00 */
    0x00,                       /* bDeviceClass */
    0x00,                       /* bDeviceSubClass */
    0x00,                       /* bDeviceProtocol */
    64,                         /* bMaxPacketSize0 */
    0x01,                       /* bNumConfigurations */
    0x00,                       /* bReserved */
};

/* String Descriptor 0 (Language ID) */
static const uint8_t usb_string_0[] = {
    4,                          /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    0x09, 0x04,                 /* wLANGID: English (US) */
};

/* String Descriptor 1 (Manufacturer) */
static const uint8_t usb_string_1[] = {
    30,                         /* bLength (2 + 14 chars × 2) */
    0x03,                       /* bDescriptorType: STRING */
    'N', 0x00, 'e', 0x00, 'u', 0x00, 'r', 0x00,
    'o', 0x00, 'L', 0x00, 'i', 0x00, 'n', 0x00,
    'k', 0x00, ' ', 0x00, 'L', 0x00, 'a', 0x00,
    'b', 0x00, 's', 0x00,
};

/* String Descriptor 2 (Product) */
static const uint8_t usb_string_2[] = {
    62,                         /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    'N', 0x00, 'e', 0x00, 'u', 0x00, 'r', 0x00,
    'o', 0x00, 'L', 0x00, 'i', 0x00, 'n', 0x00,
    'k', 0x00, ' ', 0x00, 'B', 0x00, 'i', 0x00,
    'o', 0x00, 's', 0x00, 'i', 0x00, 'g', 0x00,
    'n', 0x00, 'a', 0x00, 'l', 0x00, ' ', 0x00,
    'A', 0x00, 'c', 0x00, 'q', 0x00, 'u', 0x00,
    'i', 0x00, 's', 0x00, 'i', 0x00, 't', 0x00,
    'i', 0x00, 'o', 0x00, 'n', 0x00,
};

/* String Descriptor 3 (Serial Number) */
static const uint8_t usb_string_3[] = {
    22,                         /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    'N', 0x00, 'L', 0x00, '0', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00,
    '1', 0x00,
};

/* ============================================================
 * USB Command Protocol (Endpoint 2 OUT)
 * ============================================================ */

/* Command IDs */
#define USB_CMD_START_STREAM      0x01
#define USB_CMD_STOP_STREAM       0x02
#define USB_CMD_SET_SAMPLE_RATE   0x03
#define USB_CMD_SET_CHANNEL_GAIN  0x04
#define USB_CMD_SET_FILTER        0x05
#define USB_CMD_ENABLE_CHANNELS   0x06
#define USB_CMD_DISABLE_CHANNELS  0x07
#define USB_CMD_IMPEDANCE_CHECK   0x08
#define USB_CMD_GET_STATUS        0x09
#define USB_CMD_SET_FEATURE_CFG   0x0A
#define USB_CMD_RESET_DEVICE      0xFF

/* Data frame header (Endpoint 1 IN) */
typedef struct __attribute__((packed)) {
    uint8_t  sync[2];           /* 0xAA55 sync word */
    uint8_t  cmd;               /* Command/status byte */
    uint8_t  frame_id;         /* Frame counter (wraps at 255) */
    uint16_t timestamp_ms;     /* Milliseconds since start */
    uint16_t payload_len;       /* Payload length in bytes */
    uint8_t  num_channels;      /* Number of active channels */
    uint8_t  sample_rate_code;  /* Sample rate enum */
    uint16_t crc16;             /* CRC-16/CCITT of header */
} usb_frame_header_t;

/* Maximum payload size */
#define USB_MAX_PAYLOAD  480   /* 512 - 32 header */

/* ============================================================
 * USB Bulk Transfer Callbacks (to be implemented in main)
 * ============================================================ */
void usb_ep1_in_send(const uint8_t *data, uint16_t len);
void usb_ep2_out_receive(uint8_t *data, uint16_t len);
void usb_handle_command(uint8_t cmd, const uint8_t *payload, uint16_t len);

#endif /* USB_DESCRIPTORS_H */