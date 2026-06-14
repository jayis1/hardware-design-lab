/*
 * usb_descriptors.h — USB Device Descriptors for HexaScope
 * USB 2.0 High-Speed Device (VID: 0x1209, PID: 0xSCO6)
 * Interface: Vendor-specific bulk transfer for waveform data
 *
 * Device Class: 0xFF (Vendor Specific)
 * Configurations: 1
 * Interfaces: 2
 *   Interface 0: Bulk data transfer (waveform streaming)
 *   Interface 1: Control (FPGA register access, trigger control)
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* ============================================================
 * USB Vendor/Product IDs
 * ============================================================ */
#define USB_VID                 0x1209     /* pid.codes open VID */
#define USB_PID                 0x5C06     /* HexaScope PID */
#define USB_VERSION_BCD         0x0100    /* v1.0.0 */
#define USB_LANGUAGE            0x0409     /* English (US) */

/* ============================================================
 * Device Descriptor (18 bytes)
 * ============================================================ */
static const uint8_t usb_device_descriptor[] = {
    0x12,                       /* bLength: 18 bytes */
    0x01,                       /* bDescriptorType: DEVICE */
    0x00, 0x02,                 /* bcdUSB: USB 2.0 */
    0xFF,                       /* bDeviceClass: Vendor Specific */
    0xFF,                       /* bDeviceSubClass: Vendor Specific */
    0xFF,                       /* bDeviceProtocol: Vendor Specific */
    0x40,                       /* bMaxPacketSize0: 64 bytes (FS) or 512 (HS) */
    0x09, 0x12,                 /* idVendor: 0x1209 */
    0x06, 0x5C,                 /* idProduct: 0x5C06 */
    0x00, 0x01,                 /* bcdDevice: 1.0.0 */
    0x01,                       /* iManufacturer: String index 1 */
    0x02,                       /* iProduct: String index 2 */
    0x03,                       /* iSerialNumber: String index 3 */
    0x01                        /* bNumConfigurations: 1 */
};

/* ============================================================
 * Device Qualifier Descriptor (for high-speed capable device)
 * ============================================================ */
static const uint8_t usb_device_qualifier[] = {
    0x0A,                       /* bLength: 10 bytes */
    0x06,                       /* bDescriptorType: DEVICE_QUALIFIER */
    0x00, 0x02,                 /* bcdUSB: USB 2.0 */
    0xFF,                       /* bDeviceClass: Vendor Specific */
    0xFF,                       /* bDeviceSubClass */
    0xFF,                       /* bDeviceProtocol */
    0x40,                       /* bMaxPacketSize0: 64 */
    0x01,                       /* bNumConfigurations: 1 */
    0x00                        /* bReserved */
};

/* ============================================================
 * Configuration Descriptor (9+9+7+7+9+7+7 = 55 bytes)
 * Interface 0: Bulk data transfer
 * Interface 1: Control (vendor-specific commands)
 * ============================================================ */

/* Configuration descriptor */
static const uint8_t usb_config_descriptor[] = {
    /* Configuration Descriptor (9 bytes) */
    0x09,                       /* bLength */
    0x02,                       /* bDescriptorType: CONFIGURATION */
    0x37, 0x00,                 /* wTotalLength: 55 bytes */
    0x02,                       /* bNumInterfaces: 2 */
    0x01,                       /* bConfigurationValue: 1 */
    0x00,                       /* iConfiguration: no string */
    0x80,                       /* bmAttributes: bus-powered */
    0xFA,                       /* bMaxPower: 500 mA */

    /* Interface 0 Descriptor (9 bytes) — Bulk Data Transfer */
    0x09,                       /* bLength */
    0x04,                       /* bDescriptorType: INTERFACE */
    0x00,                       /* bInterfaceNumber: 0 */
    0x00,                       /* bAlternateSetting: 0 */
    0x02,                       /* bNumEndpoints: 2 */
    0xFF,                       /* bInterfaceClass: Vendor Specific */
    0x01,                       /* bInterfaceSubClass: Data Streaming */
    0x00,                       /* bInterfaceProtocol: Bulk */
    0x00,                       /* iInterface: no string */

    /* Endpoint 1 IN (Bulk) — Waveform data to host (7 bytes) */
    0x07,                       /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x81,                       /* bEndpointAddress: EP1 IN */
    0x02,                       /* bmAttributes: Bulk */
    0x00, 0x02,                 /* wMaxPacketSize: 512 bytes (HS) */
    0x00,                       /* bInterval: N/A for bulk */

    /* Endpoint 1 OUT (Bulk) — Host commands (7 bytes) */
    0x07,                       /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x01,                       /* bEndpointAddress: EP1 OUT */
    0x02,                       /* bmAttributes: Bulk */
    0x00, 0x02,                 /* wMaxPacketSize: 512 bytes (HS) */
    0x00,                       /* bInterval: N/A for bulk */

    /* Interface 1 Descriptor (9 bytes) — Control */
    0x09,                       /* bLength */
    0x04,                       /* bDescriptorType: INTERFACE */
    0x01,                       /* bInterfaceNumber: 1 */
    0x00,                       /* bAlternateSetting: 0 */
    0x02,                       /* bNumEndpoints: 2 */
    0xFF,                       /* bInterfaceClass: Vendor Specific */
    0x02,                       /* bInterfaceSubClass: Control */
    0x00,                       /* bInterfaceProtocol: None */
    0x00,                       /* iInterface: no string */

    /* Endpoint 2 IN (Bulk) — Status/registers to host (7 bytes) */
    0x07,                       /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x82,                       /* bEndpointAddress: EP2 IN */
    0x02,                       /* bmAttributes: Bulk */
    0x00, 0x02,                 /* wMaxPacketSize: 512 bytes (HS) */
    0x00,                       /* bInterval: N/A */

    /* Endpoint 2 OUT (Bulk) — Control commands from host (7 bytes) */
    0x07,                       /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x02,                       /* bEndpointAddress: EP2 OUT */
    0x02,                       /* bmAttributes: Bulk */
    0x00, 0x02,                 /* wMaxPacketSize: 512 bytes (HS) */
    0x00,                       /* bInterval: N/A */
};

/* ============================================================
 * String Descriptors
 * ============================================================ */

/* String 0: Language ID */
static const uint8_t usb_string_0[] = {
    0x04,                       /* bLength: 4 bytes */
    0x03,                       /* bDescriptorType: STRING */
    0x09, 0x04                  /* wLANGID: English (US) */
};

/* String 1: Manufacturer */
static const uint8_t usb_string_1[] = {
    0x1A,                       /* bLength: 26 bytes */
    0x03,                       /* bDescriptorType: STRING */
    'H', 0x00, 'e', 0x00, 'x', 0x00, 'a', 0x00,
    'S', 0x00, 'c', 0x00, 'o', 0x00, 'p', 0x00,
    'e', 0x00, ' ', 0x00, 'L', 0x00, 'a', 0x00,
    'b', 0x00, 's', 0x00
};

/* String 2: Product */
static const uint8_t usb_string_2[] = {
    0x2C,                       /* bLength: 44 bytes */
    0x03,                       /* bDescriptorType: STRING */
    'H', 0x00, 'e', 0x00, 'x', 0x00, 'a', 0x00,
    'S', 0x00, 'c', 0x00, 'o', 0x00, 'p', 0x00,
    'e', 0x00, ' ', 0x00, '6', 0x00, '-', 0x00,
    'C', 0x00, 'h', 0x00, 'a', 0x00, 'n', 0x00,
    'n', 0x00, 'e', 0x00, 'l', 0x00, ' ', 0x00,
    'O', 0x00, 's', 0x00, 'c', 0x00, 'i', 0x00,
    'l', 0x00, 'l', 0x00, 'o', 0x00, 's', 0x00,
    'c', 0x00, 'o', 0x00, 'p', 0x00, 'e', 0x00
};

/* String 3: Serial Number (placeholder — should be unique per device) */
static const uint8_t usb_string_3[] = {
    0x1A,                       /* bLength: 26 bytes */
    0x03,                       /* bDescriptorType: STRING */
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '0', 0x00, '1', 0x00
};

/* ============================================================
 * USB Vendor Request Codes
 * ============================================================ */
#define USB_REQ_ARM_TRIGGER        0x01    /* Arm trigger (wValue=trigger_type) */
#define USB_REQ_STOP_ACQUISITION   0x02    /* Stop current acquisition */
#define USB_REQ_READ_STATUS        0x03    /* Read FPGA status register */
#define USB_REQ_SET_TRIGGER_THRESH 0x04    /* Set trigger threshold (wValue=channel) */
#define USB_REQ_SET_DECIMATION     0x05    /* Set decimation rate */
#define USB_REQ_SET_CHANNEL_ENABLE 0x06    /* Enable/disable channels */
#define USB_REQ_SET_PROTOCOL       0x07    /* Set protocol decoder */
#define USB_REQ_READ_REGISTER      0x10    /* Read FPGA register (wIndex=address) */
#define USB_REQ_WRITE_REGISTER     0x11    /* Write FPGA register (wIndex=addr, wValue=data) */
#define USB_REQ_CALIBRATION        0x20    /* Set calibration values */
#define USB_REQ_GET_INFO           0x30    /* Get device info (version, capabilities) */

/* ============================================================
 * Waveform Data Frame Format (Interface 0, EP1 IN)
 * ============================================================ */
typedef struct __attribute__((packed)) {
    uint32_t magic;             /* 0x48584600 = "HXf\0" */
    uint8_t  channel_mask;      /* Bit mask of active channels */
    uint8_t  trigger_type;      /* Trigger type enum */
    uint32_t sample_count;      /* Number of samples per channel */
    uint32_t timestamp_ns;      /* Trigger timestamp (ns since boot) */
    uint16_t sample_rate_hz;    /* Effective sample rate (Hz / 1000) */
    uint16_t flags;              /* Bit 0: overflow, Bit 1: compressed */
    uint8_t  data[];             /* Variable-length sample data */
} usb_waveform_header_t;

#define USB_WAVEFORM_MAGIC          0x48584600U

/* ============================================================
 * Control Command Format (Interface 1, EP2 OUT)
 * ============================================================ */
typedef struct __attribute__((packed)) {
    uint8_t  command;            /* Command code (USB_REQ_*) */
    uint8_t  reserved;
    uint16_t value;              /* Command-specific value */
    uint32_t index;              /* Command-specific index/address */
    uint32_t length;             /* Data length (0 if no data follows) */
} usb_control_cmd_t;

/* ============================================================
 * Device Info Response (for USB_REQ_GET_INFO)
 * ============================================================ */
typedef struct __attribute__((packed)) {
    uint32_t firmware_version;   /* 0x00010000 = v1.0.0 */
    uint8_t  num_analog_ch;      /* 4 */
    uint8_t  num_digital_ch;     /* 2 */
    uint32_t max_sample_rate;    /* 250000000 */
    uint32_t memory_depth;        /* 134217728 (128 Mpts) */
    uint16_t adc_resolution;     /* 8 */
    uint8_t  protocol_version;   /* 1 */
    uint8_t  reserved[3];
} usb_device_info_t;

#endif /* USB_DESCRIPTORS_H */