/*
 * usb_descriptors.h — Vortex-SDR USB device descriptors
 * Composite device: CDC-ACM (command) + Bulk (data)
 */

#ifndef VORTEX_SDR_USB_DESCRIPTORS_H
#define VORTEX_SDR_USB_DESCRIPTORS_H

#include <stdint.h>

/* USB VID/PID */
#define VORTEX_VID             0x1209    /* pid.codes */
#define VORTEX_PID             0x7SDR    /* Vortex-SDR */

/* USB descriptor types */
#define USB_DESC_DEVICE        0x01
#define USB_DESC_CONFIG        0x02
#define USB_DESC_STRING         0x03
#define USB_DESC_INTERFACE      0x04
#define USB_DESC_ENDPOINT      0x05
#define USB_DESC_DEVICE_QUAL   0x06
#define USB_DESC_ASSOCIATION   0x0B

/* USB classes */
#define USB_CLASS_CDC          0x02
#define USB_CLASS_DATA         0x0A
#define USB_CLASS_VENDOR       0xFF
#define USB_CLASS_MISCELLANEOUS 0xEF

/* CDC subclass codes */
#define CDC_SUBCLASS_ACM       0x02
#define CDC_PROTOCOL_NONE      0x00

/* Endpoint directions */
#define USB_EP_DIR_OUT         0x00
#define USB_EP_DIR_IN          0x80

/* Endpoint types */
#define USB_EP_TYPE_CTRL       0x00
#define USB_EP_TYPE_ISO        0x01
#define USB_EP_TYPE_BULK       0x02
#define USB_EP_TYPE_INTR       0x03

/* Configuration */
#define USB_CONFIG_VALUE       0x01
#define USB_MAX_POWER          250      /* 500 mA */

/* Endpoint numbers */
#define USB_EP0_CTRL           0x00
#define USB_EP1_BULK_IN        0x01     /* Spectrum data to host */
#define USB_EP2_BULK_OUT       0x02     /* Commands from host */
#define USB_EP3_INTR_IN       0x83     /* Status notifications */
#define USB_EP4_CDC_INTR_IN   0x84     /* CDC interrupt */
#define USB_EP5_CDC_DATA_IN   0x85     /* CDC data to host */
#define USB_EP6_CDC_DATA_OUT  0x06     /* CDC data from host */

/* String descriptor indices */
#define USB_STR_LANG           0x00
#define USB_STR_MANUFACTURER   0x01
#define USB_STR_PRODUCT        0x02
#define USB_STR_SERIAL         0x03
#define USB_STR_CDC_IFACE     0x04
#define USB_STR_DATA_IFACE     0x05

/* ======================================================================== */
/* Device Descriptor                                                        */
/* ======================================================================== */

static const uint8_t device_descriptor[] = {
    0x12,       /* bLength */
    0x01,       /* bDescriptorType: Device */
    0x00, 0x02, /* bcdUSB: 2.00 */
    0xEF,       /* bDeviceClass: Miscellaneous */
    0x02,       /* bDeviceSubClass: Common class */
    0x01,       /* bDeviceProtocol: Interface Association */
    0x40,       /* bMaxPacketSize0: 64 bytes */
    0x09, 0x12, /* idVendor: 0x1209 (pid.codes) */
    0x52, 0x07, /* idProduct: 0x7SDR (Vortex-SDR) */
    0x00, 0x01, /* bcdDevice: 1.00 */
    0x01,       /* iManufacturer */
    0x02,       /* iProduct */
    0x03,       /* iSerialNumber */
    0x01,       /* bNumConfigurations: 1 */
};

/* ======================================================================== */
/* Configuration Descriptor (Composite: CDC-ACM + Bulk)                     */
/* ======================================================================== */

static const uint8_t config_descriptor[] = {
    /* Configuration descriptor */
    0x09,       /* bLength */
    0x02,       /* bDescriptorType: Configuration */
    0x58, 0x00, /* wTotalLength: 88 bytes */
    0x03,       /* bNumInterfaces: 3 */
    0x01,       /* bConfigurationValue */
    0x00,       /* iConfiguration */
    0x80,       /* bmAttributes: Bus powered */
    0xFA,       /* bMaxPower: 500 mA */

    /* Interface Association Descriptor (IAD) for CDC-ACM */
    0x08,       /* bLength */
    0x0B,       /* bDescriptorType: IAD */
    0x00,       /* bFirstInterface: 0 */
    0x02,       /* bInterfaceCount: 2 */
    0x02,       /* bFunctionClass: CDC */
    0x02,       /* bFunctionSubClass: ACM */
    0x01,       /* bFunctionProtocol: v.25ter */
    0x04,       /* iFunction */

    /* CDC Interface descriptor (Interface 0) */
    0x09,       /* bLength */
    0x04,       /* bDescriptorType: Interface */
    0x00,       /* bInterfaceNumber: 0 */
    0x00,       /* bAlternateSetting: 0 */
    0x01,       /* bNumEndpoints: 1 (interrupt) */
    0x02,       /* bInterfaceClass: CDC */
    0x02,       /* bInterfaceSubClass: ACM */
    0x01,       /* bInterfaceProtocol: v.25ter */
    0x04,       /* iInterface */

    /* CDC Header Functional descriptor */
    0x05,       /* bLength */
    0x24,       /* bDescriptorType: CS_INTERFACE */
    0x00,       /* bDescriptorSubtype: Header */
    0x10, 0x01, /* bcdCDC: 1.10 */

    /* CDC ACM Functional descriptor */
    0x04,       /* bLength */
    0x24,       /* bDescriptorType: CS_INTERFACE */
    0x02,       /* bDescriptorSubtype: ACM */
    0x02,       /* bmCapabilities: Line coding + DTR/RTS */

    /* CDC Union Functional descriptor */
    0x05,       /* bLength */
    0x24,       /* bDescriptorType: CS_INTERFACE */
    0x06,       /* bDescriptorSubtype: Union */
    0x00,       /* bControlInterface: 0 */
    0x01,       /* bSubordinateInterface: 1 */

    /* CDC Call Management Functional descriptor */
    0x05,       /* bLength */
    0x24,       /* bDescriptorType: CS_INTERFACE */
    0x01,       /* bDescriptorSubtype: Call Management */
    0x00,       /* bmCapabilities: None */
    0x01,       /* bDataInterface: 1 */

    /* CDC Interrupt IN endpoint descriptor */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: Endpoint */
    0x84,       /* bEndpointAddress: EP4 IN (interrupt) */
    0x03,       /* bmAttributes: Interrupt */
    0x40, 0x00, /* wMaxPacketSize: 64 bytes */
    0x10,       /* bInterval: 16 frames */

    /* CDC Data Interface descriptor (Interface 1) */
    0x09,       /* bLength */
    0x04,       /* bDescriptorType: Interface */
    0x01,       /* bInterfaceNumber: 1 */
    0x00,       /* bAlternateSetting: 0 */
    0x02,       /* bNumEndpoints: 2 (bulk in/out) */
    0x0A,       /* bInterfaceClass: Data */
    0x00,       /* bInterfaceSubClass: None */
    0x00,       /* bInterfaceProtocol: None */
    0x05,       /* iInterface */

    /* CDC Data IN endpoint descriptor */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: Endpoint */
    0x85,       /* bEndpointAddress: EP5 IN (bulk) */
    0x02,       /* bmAttributes: Bulk */
    0x00, 0x02, /* wMaxPacketSize: 512 bytes */
    0x00,       /* bInterval: N/A */

    /* CDC Data OUT endpoint descriptor */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: Endpoint */
    0x06,       /* bEndpointAddress: EP6 OUT (bulk) */
    0x02,       /* bmAttributes: Bulk */
    0x00, 0x02, /* wMaxPacketSize: 512 bytes */
    0x00,       /* bInterval: N/A */

    /* Vendor Data Interface descriptor (Interface 2) */
    0x09,       /* bLength */
    0x04,       /* bDescriptorType: Interface */
    0x02,       /* bInterfaceNumber: 2 */
    0x00,       /* bAlternateSetting: 0 */
    0x03,       /* bNumEndpoints: 3 */
    0xFF,       /* bInterfaceClass: Vendor */
    0x00,       /* bInterfaceSubClass: None */
    0x00,       /* bInterfaceProtocol: None */
    0x00,       /* iInterface */

    /* Bulk IN endpoint descriptor (spectrum data) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: Endpoint */
    0x01,       /* bEndpointAddress: EP1 IN (bulk) */
    0x02,       /* bmAttributes: Bulk */
    0x00, 0x02, /* wMaxPacketSize: 512 bytes */
    0x00,       /* bInterval: N/A */

    /* Bulk OUT endpoint descriptor (commands) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: Endpoint */
    0x02,       /* bEndpointAddress: EP2 OUT (bulk) */
    0x02,       /* bmAttributes: Bulk */
    0x00, 0x02, /* wMaxPacketSize: 512 bytes */
    0x00,       /* bInterval: N/A */

    /* Interrupt IN endpoint descriptor (status) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: Endpoint */
    0x83,       /* bEndpointAddress: EP3 IN (interrupt) */
    0x03,       /* bmAttributes: Interrupt */
    0x40, 0x00, /* wMaxPacketSize: 64 bytes */
    0x0A,       /* bInterval: 10 frames */
};

/* ======================================================================== */
/* String Descriptors                                                       */
/* ======================================================================== */

static const uint8_t string_lang_descriptor[] = {
    0x04, 0x03,   /* bLength, bDescriptorType: String */
    0x09, 0x04,   /* wLANGID: English (US) */
};

static const uint8_t string_manufacturer_descriptor[] = {
    0x1C, 0x03,   /* bLength, bDescriptorType: String */
    'V', 0x00, 'o', 0x00, 'r', 0x00, 't', 0x00, 'e', 0x00,
    'x', 0x00, ' ', 0x00, 'I', 0x00, 'n', 0x00, 's', 0x00,
    't', 0x00, 'r', 0x00, 'u', 0x00, 'm', 0x00, 'e', 0x00,
    'n', 0x00, 't', 0x00, 's', 0x00,
};

static const uint8_t string_product_descriptor[] = {
    0x1C, 0x03,   /* bLength, bDescriptorType: String */
    'V', 0x00, 'o', 0x00, 'r', 0x00, 't', 0x00, 'e', 0x00,
    'x', 0x00, '-', 0x00, 'S', 0x00, 'D', 0x00, 'R', 0x00,
    ' ', 0x00, 'A', 0x00, 'n', 0x00, 'a', 0x00, 'l', 0x00,
    'y', 0x00, 'z', 0x00, 'e', 0x00, 'r', 0x00,
};

static const uint8_t string_serial_descriptor[] = {
    0x1A, 0x03,   /* bLength, bDescriptorType: String */
    'V', 0x00, 'S', 0x00, 'D', 0x00, 'R', 0x00, '2', 0x00,
    '0', 0x00, '2', 0x00, '6', 0x00, '0', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '0', 0x00, '1', 0x00,
};

/* ======================================================================== */
/* CDC Line Coding (115200 baud, 8N1)                                       */
/* ======================================================================== */

static const uint8_t cdc_line_coding[] = {
    0x00, 0xC2, 0x01, 0x00,   /* dwDTERate: 115200 baud */
    0x00,                       /* bCharFormat: 1 stop bit */
    0x00,                       /* bParityType: None */
    0x08,                       /* bDataBits: 8 */
};

#endif /* VORTEX_SDR_USB_DESCRIPTORS_H */