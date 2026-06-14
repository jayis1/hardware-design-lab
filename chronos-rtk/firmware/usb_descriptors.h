/*
 * usb_descriptors.h — USB CDC-ACM descriptors for Chronos-RTK
 * STM32G474 USB FS device (12 Mbps)
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* VID/PID */
#define USB_VID                    0x0483   /* STMicroelectronics */
#define USB_PID                    0x5740   /* Custom CDC-ACM device */
#define USB_DEVICE_VERSION          0x0100   /* v1.0 */
#define USB_CONFIG_NUM             1        /* One configuration */
#define USB_INTERFACE_NUM           2        /* CDC: 2 interfaces (ACM + data) */

/* Endpoint addresses */
#define EP0_IN                     0x80
#define EP0_OUT                    0x00
#define EP1_IN                     0x81      /* CDC data IN */
#define EP2_OUT                    0x02      /* CDC data OUT */
#define EP3_IN                     0x83      /* CDC interrupt IN */

/* Packet sizes */
#define USB_CTRL_MAX_PACKET        64
#define USB_CDC_DATA_MAX_PACKET    64
#define USB_CDC_INT_MAX_PACKET     8

/* String descriptor indices */
#define USB_STRING_LANG_ID         0
#define USB_STRING_MANUFACTURER    1
#define USB_STRING_PRODUCT         2
#define USB_STRING_SERIAL          3
#define USB_STRING_CONFIG          4
#define USB_STRING_INTERFACE       5

/* ========================================================================== */
/* Device descriptor                                                            */
/* ========================================================================== */
static const uint8_t usb_device_descriptor[] = {
    0x12,                          /* bLength */
    0x01,                          /* bDescriptorType (DEVICE) */
    0x00, 0x02,                    /* bcdUSB (2.00) */
    0x02,                          /* bDeviceClass (CDC) */
    0x00,                          /* bDeviceSubClass */
    0x00,                          /* bDeviceProtocol */
    USB_CTRL_MAX_PACKET,           /* bMaxPacketSize0 */
    USB_VID & 0xFF, USB_VID >> 8,  /* idVendor */
    USB_PID & 0xFF, USB_PID >> 8,  /* idProduct */
    USB_DEVICE_VERSION & 0xFF, USB_DEVICE_VERSION >> 8, /* bcdDevice */
    0x01,                          /* iManufacturer */
    0x02,                          /* iProduct */
    0x03,                          /* iSerialNumber */
    USB_CONFIG_NUM                  /* bNumConfigurations */
};

/* ========================================================================== */
/* Configuration descriptor (CDC-ACM)                                          */
/* ========================================================================== */
static const uint8_t usb_config_descriptor[] = {
    /* Configuration descriptor */
    0x09,                          /* bLength */
    0x02,                          /* bDescriptorType (CONFIG) */
    0x43, 0x00,                    /* wTotalLength (67 bytes) */
    0x02,                          /* bNumInterfaces (2) */
    0x01,                          /* bConfigurationValue */
    0x04,                          /* iConfiguration */
    0x80,                          /* bmAttributes (bus powered) */
    0x32,                          /* bMaxPower (100 mA) */

    /* Interface 0: CDC ACM (Communication Class) */
    0x09,                          /* bLength */
    0x04,                          /* bDescriptorType (INTERFACE) */
    0x00,                          /* bInterfaceNumber */
    0x00,                          /* bAlternateSetting */
    0x01,                          /* bNumEndpoints (1: interrupt IN) */
    0x02,                          /* bInterfaceClass (CDC) */
    0x02,                          /* bInterfaceSubClass (ACM) */
    0x01,                          /* bInterfaceProtocol (V.250) */
    0x05,                          /* iInterface */

    /* CDC Header Functional Descriptor */
    0x05,                          /* bFunctionLength */
    0x24,                          /* bDescriptorType (CS_INTERFACE) */
    0x00,                          /* bDescriptorSubtype (HEADER) */
    0x10, 0x01,                    /* bcdCDC (1.10) */

    /* CDC Call Management Functional Descriptor */
    0x05,                          /* bFunctionLength */
    0x24,                          /* bDescriptorType (CS_INTERFACE) */
    0x01,                          /* bDescriptorSubtype (CALL_MANAGEMENT) */
    0x00,                          /* bmCapabilities */
    0x01,                          /* bDataInterface (1) */

    /* CDC ACM Functional Descriptor */
    0x04,                          /* bFunctionLength */
    0x24,                          /* bDescriptorType (CS_INTERFACE) */
    0x02,                          /* bDescriptorSubtype (ACM) */
    0x02,                          /* bmCapabilities (support line coding) */

    /* CDC Union Functional Descriptor */
    0x05,                          /* bFunctionLength */
    0x24,                          /* bDescriptorType (CS_INTERFACE) */
    0x06,                          /* bDescriptorSubtype (UNION) */
    0x00,                          /* bMasterInterface (0) */
    0x01,                          /* bSlaveInterface (1) */

    /* Endpoint 3 IN: Interrupt */
    0x07,                          /* bLength */
    0x05,                          /* bDescriptorType (ENDPOINT) */
    EP3_IN,                         /* bEndpointAddress (IN 3) */
    0x03,                          /* bmAttributes (Interrupt) */
    USB_CDC_INT_MAX_PACKET, 0x00,  /* wMaxPacketSize */
    0x10,                          /* bInterval (16 frames) */

    /* Interface 1: CDC Data */
    0x09,                          /* bLength */
    0x04,                          /* bDescriptorType (INTERFACE) */
    0x01,                          /* bInterfaceNumber */
    0x00,                          /* bAlternateSetting */
    0x02,                          /* bNumEndpoints (2: bulk IN + OUT) */
    0x0A,                          /* bInterfaceClass (CDC Data) */
    0x00,                          /* bInterfaceSubClass */
    0x00,                          /* bInterfaceProtocol */
    0x05,                          /* iInterface */

    /* Endpoint 1 IN: Bulk Data */
    0x07,                          /* bLength */
    0x05,                          /* bDescriptorType (ENDPOINT) */
    EP1_IN,                         /* bEndpointAddress (IN 1) */
    0x02,                          /* bmAttributes (Bulk) */
    USB_CDC_DATA_MAX_PACKET, 0x00,  /* wMaxPacketSize */
    0x00,                          /* bInterval */

    /* Endpoint 2 OUT: Bulk Data */
    0x07,                          /* bLength */
    0x05,                          /* bDescriptorType (ENDPOINT) */
    EP2_OUT,                        /* bEndpointAddress (OUT 2) */
    0x02,                          /* bmAttributes (Bulk) */
    USB_CDC_DATA_MAX_PACKET, 0x00,  /* wMaxPacketSize */
    0x00,                          /* bInterval */
};

/* ========================================================================== */
/* String descriptors                                                           */
/* ========================================================================== */

/* Language ID (US English) */
static const uint8_t usb_string_lang_id[] = {
    0x04, 0x03,
    0x09, 0x04   /* wLANGID = 0x0409 (US English) */
};

/* Manufacturer string */
static const uint8_t usb_string_manufacturer[] = {
    0x1A, 0x03,   /* bLength = 26 bytes */
    'C', 0x00, 'h', 0x00, 'r', 0x00, 'o', 0x00,
    'n', 0x00, 'o', 0x00, 's', 0x00, ' ', 0x00,
    'S', 0x00, 'y', 0x00, 's', 0x00, 't', 0x00,
    'e', 0x00, 'm', 0x00, 's', 0x00
};

/* Product string */
static const uint8_t usb_string_product[] = {
    0x1C, 0x03,   /* bLength = 28 bytes */
    'C', 0x00, 'h', 0x00, 'r', 0x00, 'o', 0x00,
    'n', 0x00, 'o', 0x00, 's', 0x00, '-', 0x00,
    'R', 0x00, 'T', 0x00, 'K', 0x00, ' ', 0x00,
    'v', 0x00, '1', 0x00, '.', 0x00, '0', 0x00
};

/* Serial number (placeholder, should be unique per device) */
static const uint8_t usb_string_serial[] = {
    0x1A, 0x03,
    'C', 0x00, 'H', 0x00, 'R', 0x00, 'O', 0x00,
    'N', 0x00, 'O', 0x00, 'S', 0x00, '2', 0x00,
    '0', 0x00, '2', 0x00, '6', 0x00, '0', 0x00,
    '0', 0x00, '1', 0x00
};

/* Configuration string */
static const uint8_t usb_string_config[] = {
    0x10, 0x03,
    'C', 0x00, 'D', 0x00, 'C', 0x00, ' ', 0x00,
    'C', 0x00, 'o', 0x00, 'n', 0x00, 'f', 0x00,
    'i', 0x00, 'g', 0x00
};

/* Interface string */
static const uint8_t usb_string_interface[] = {
    0x16, 0x03,
    'C', 0x00, 'D', 0x00, 'C', 0x00, ' ', 0x00,
    'S', 0x00, 'e', 0x00, 'r', 0x00, 'i', 0x00,
    'a', 0x00, 'l', 0x00
};

/* String descriptor table */
static const uint8_t * const usb_string_descriptors[] = {
    usb_string_lang_id,
    usb_string_manufacturer,
    usb_string_product,
    usb_string_serial,
    usb_string_config,
    usb_string_interface,
};

#define USB_STRING_DESCRIPTOR_COUNT  6

/* ========================================================================== */
/* CDC line coding defaults                                                     */
/* ========================================================================== */
#define CDC_LINE_CODING_DTERATE     115200   /* 115200 baud */
#define CDC_LINE_CODING_STOPBITS    0        /* 1 stop bit */
#define CDC_LINE_CODING_PARITY       0        /* No parity */
#define CDC_LINE_CODING_DATABITS    8        /* 8 data bits */

#endif /* USB_DESCRIPTORS_H */