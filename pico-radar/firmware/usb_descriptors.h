/*
 * usb_descriptors.h — USB device descriptors for PicoRadar
 * USB 2.0 Full-Speed CDC (Communication Device Class)
 * VID: 0x1209 (pid.codes), PID: 0xCA60
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* ---- USB VID/PID ---- */
#define USB_VID                  0x1209   /* pid.codes open vendor ID */
#define USB_PID                  0xCA60   /* PicoRadar CDC */
#define USB_DEVICE_VERSION       0x0100   /* v1.0 */

/* ---- Configuration values ---- */
#define USB_CONFIG_VALUE         1
#define USB_MAX_POWER_MA        100      /* 100 mA (self-powered with PoE) */

/* ---- String descriptor indices ---- */
#define USB_STRING_LANG_ID       0
#define USB_STRING_MANUFACTURER  1
#define USB_STRING_PRODUCT       2
#define USB_STRING_SERIAL        3
#define USB_STRING_CONFIG        4
#define USB_STRING_INTERFACE     5

/* ---- Endpoint numbers ---- */
#define CDC0_INT_EP              1       /* CDC notification (EP1 IN) */
#define CDC0_DATA_OUT_EP         2       /* CDC data out (EP2 OUT) */
#define CDC0_DATA_IN_EP          2       /* CDC data in (EP2 IN) */

/* ---- Endpoint sizes ---- */
#define CDC_INT_EP_SIZE          8
#define CDC_DATA_EP_SIZE         64

/* ---- Device Descriptor (18 bytes) ---- */
static const uint8_t device_descriptor[18] = {
    0x12,        /* bLength = 18 */
    0x01,        /* bDescriptorType = DEVICE */
    0x00, 0x02,  /* bcdUSB = 2.00 */
    0x02,        /* bDeviceClass = CDC */
    0x00,        /* bDeviceSubClass = 0 */
    0x00,        /* bDeviceProtocol = 0 */
    0x40,        /* bMaxPacketSize0 = 64 */
    0x09, 0x12,  /* idVendor = 0x1209 */
    0x60, 0xCA,  /* idProduct = 0xCA60 */
    0x00, 0x01,  /* bcdDevice = 1.00 */
    0x01,        /* iManufacturer = string 1 */
    0x02,        /* iProduct = string 2 */
    0x03,        /* iSerialNumber = string 3 */
    0x01         /* bNumConfigurations = 1 */
};

/* ---- Configuration Descriptor (9 + 8 + 9 + 5 + 4 + 5 + 7 + 9 + 7 = 67 bytes) ---- */
static const uint8_t config_descriptor[67] = {
    /* Configuration descriptor (9 bytes) */
    0x09,        /* bLength */
    0x02,        /* bDescriptorType = CONFIGURATION */
    67, 0x00,    /* wTotalLength = 67 */
    0x02,        /* bNumInterfaces = 2 (CDC Control + CDC Data) */
    0x01,        /* bConfigurationValue = 1 */
    0x00,        /* iConfiguration = 0 */
    0x80,        /* bmAttributes = bus-powered */
    0x32,        /* bMaxPower = 100 mA */

    /* Interface Association Descriptor (8 bytes) */
    0x08,        /* bLength */
    0x0B,        /* bDescriptorType = IAD */
    0x00,        /* bFirstInterface = 0 */
    0x02,        /* bInterfaceCount = 2 */
    0x02,        /* bFunctionClass = CDC */
    0x02,        /* bFunctionSubClass = ACM */
    0x01,        /* bFunctionProtocol = AT commands */
    0x00,        /* iFunction = 0 */

    /* CDC Control Interface (9 bytes) */
    0x09,        /* bLength */
    0x04,        /* bDescriptorType = INTERFACE */
    0x00,        /* bInterfaceNumber = 0 */
    0x00,        /* bAlternateSetting = 0 */
    0x01,        /* bNumEndpoints = 1 (notification) */
    0x02,        /* bInterfaceClass = CDC */
    0x02,        /* bInterfaceSubClass = ACM */
    0x01,        /* bInterfaceProtocol = AT commands */
    0x00,        /* iInterface = 0 */

    /* CDC Header Functional Descriptor (5 bytes) */
    0x05, 0x24, 0x00, 0x10, 0x01,

    /* CDC ACM Functional Descriptor (4 bytes) */
    0x04, 0x24, 0x02, 0x02,

    /* CDC Union Functional Descriptor (5 bytes) */
    0x05, 0x24, 0x06, 0x00, 0x01,

    /* CDC Call Management Functional Descriptor (5 bytes) */
    0x05, 0x24, 0x01, 0x00, 0x01,

    /* Notification Endpoint (7 bytes) */
    0x07,        /* bLength */
    0x05,        /* bDescriptorType = ENDPOINT */
    0x81,        /* bEndpointAddress = EP1 IN */
    0x03,        /* bmAttributes = Interrupt */
    0x08, 0x00,  /* wMaxPacketSize = 8 */
    0x08,        /* bInterval = 8 ms */

    /* CDC Data Interface (9 bytes) */
    0x09,        /* bLength */
    0x04,        /* bDescriptorType = INTERFACE */
    0x01,        /* bInterfaceNumber = 1 */
    0x00,        /* bAlternateSetting = 0 */
    0x02,        /* bNumEndpoints = 2 (bulk in/out) */
    0x0A,        /* bInterfaceClass = CDC Data */
    0x00,        /* bInterfaceSubClass = 0 */
    0x00,        /* bInterfaceProtocol = 0 */
    0x00,        /* iInterface = 0 */

    /* Data OUT Endpoint (7 bytes) */
    0x07,        /* bLength */
    0x05,        /* bDescriptorType = ENDPOINT */
    0x02,        /* bEndpointAddress = EP2 OUT */
    0x02,        /* bmAttributes = Bulk */
    0x40, 0x00,  /* wMaxPacketSize = 64 */
    0x00,        /* bInterval = 0 */

    /* Data IN Endpoint (7 bytes) */
    0x07,        /* bLength */
    0x05,        /* bDescriptorType = ENDPOINT */
    0x82,        /* bEndpointAddress = EP2 IN */
    0x02,        /* bmAttributes = Bulk */
    0x40, 0x00,  /* wMaxPacketSize = 64 */
    0x00,        /* bInterval = 0 */
};

/* ---- String Descriptors ---- */

/* Language ID: English (US) = 0x0409 */
static const uint8_t string_lang_id[4] = {
    0x04, 0x03, 0x09, 0x04
};

/* Manufacturer: "Nous Research" */
static const uint8_t string_manufacturer[28] = {
    28, 0x03,
    'N', 0, 'o', 0, 'u', 0, 's', 0, ' ', 0,
    'R', 0, 'e', 0, 's', 0, 'e', 0, 'a', 0, 'r', 0, 'c', 0, 'h', 0
};

/* Product: "PicoRadar 60GHz" */
static const uint8_t string_product[30] = {
    30, 0x03,
    'P', 0, 'i', 0, 'c', 0, 'o', 0, 'R', 0,
    'a', 0, 'd', 0, 'a', 0, 'r', 0, ' ', 0,
    '6', 0, '0', 0, 'G', 0, 'H', 0, 'z', 0
};

/* Serial: "PR-00000001" (placeholder, should be unique per device) */
static const uint8_t string_serial[24] = {
    24, 0x03,
    'P', 0, 'R', 0, '-', 0, '0', 0, '0', 0,
    '0', 0, '0', 0, '0', 0, '0', 0, '1', 0, '1', 0
};

#endif /* USB_DESCRIPTORS_H */