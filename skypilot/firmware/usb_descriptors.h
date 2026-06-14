/*
 * SkyPilot — USB Descriptors
 * STM32H743 USB OTG Full-Speed Device
 * Config port: Mass Storage + CDC (composite)
 */

#ifndef SKYPILOT_USB_DESCRIPTORS_H
#define SKYPILOT_USB_DESCRIPTORS_H

#include <stdint.h>

/* ---- VID/PID ---- */
#define USB_VID                 0x1209    /* pid.codes */
#define USB_PID_SKYPILOT        0x0001    /* SkyPilot FC */
#define USB_DEVICE_VERSION      0x0100    /* v1.0 */

/* ---- Device descriptor ---- */
static const uint8_t device_descriptor[] = {
    18,                         /* bLength */
    0x01,                       /* bDescriptorType: DEVICE */
    0x00, 0x02,                 /* bcdUSB: 2.00 */
    0xEF,                       /* bDeviceClass: Misc (IAD) */
    0x02,                       /* bDeviceSubClass: CDC */
    0x01,                       /* bDeviceProtocol: IAD */
    64,                         /* bMaxPacketSize0 */
    0x09, 0x12,                 /* idVendor: 0x1209 */
    0x01, 0x00,                 /* idProduct: 0x0001 */
    0x00, 0x01,                 /* bcdDevice: 1.0 */
    1,                          /* iManufacturer */
    2,                          /* iProduct */
    3,                          /* iSerialNumber */
    1,                          /* bNumConfigurations */
};

/* ---- Configuration descriptor (CDC + MSC composite) ---- */
#define CFG_TOTAL_LENGTH       98
#define CFG_NUM_INTERFACES     3  /* CDC (2) + MSC (1) */

static const uint8_t config_descriptor[] = {
    /* ---- Configuration descriptor ---- */
    9,                          /* bLength */
    0x02,                       /* bDescriptorType: CONFIGURATION */
    CFG_TOTAL_LENGTH & 0xFF,
    CFG_TOTAL_LENGTH >> 8,
    CFG_NUM_INTERFACES,         /* bNumInterfaces */
    1,                          /* bConfigurationValue */
    0,                          /* iConfiguration */
    0x80,                       /* bmAttributes: bus-powered */
    250,                        /* bMaxPower: 500mA */

    /* ---- IAD: CDC ---- */
    8,                          /* bLength */
    0x0B,                       /* bDescriptorType: IAD */
    0,                          /* bFirstInterface: 0 */
    2,                          /* bInterfaceCount: 2 */
    0x02,                       /* bFunctionClass: CDC */
    0x02,                       /* bFunctionSubClass: ACM */
    0x01,                       /* bFunctionProtocol: V.250 */
    0,                          /* iFunction */

    /* ---- Interface 0: CDC Control ---- */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: INTERFACE */
    0,                          /* bInterfaceNumber: 0 */
    0,                          /* bAlternateSetting */
    1,                          /* bNumEndpoints: 1 */
    0x02,                       /* bInterfaceClass: CDC */
    0x02,                       /* bInterfaceSubClass: ACM */
    0x01,                       /* bInterfaceProtocol: V.250 */
    0,                          /* iInterface */

    /* ---- CDC Header Functional Descriptor ---- */
    5, 0x24, 0x00, 0x10, 0x01,

    /* ---- CDC ACM Functional Descriptor ---- */
    4, 0x24, 0x02, 0x02,

    /* ---- CDC Union Functional Descriptor ---- */
    5, 0x24, 0x06, 0, 1,

    /* ---- CDC Call Management Functional Descriptor ---- */
    5, 0x24, 0x01, 0x01, 0, 1,

    /* ---- Endpoint 1: CDC Control (Interrupt IN) ---- */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x81,                       /* bEndpointAddress: IN 1 */
    0x03,                       /* bmAttributes: Interrupt */
    8, 0,                       /* wMaxPacketSize: 8 */
    16,                         /* bInterval: 16ms */

    /* ---- Interface 1: CDC Data ---- */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: INTERFACE */
    1,                          /* bInterfaceNumber: 1 */
    0,                          /* bAlternateSetting */
    2,                          /* bNumEndpoints: 2 */
    0x0A,                       /* bInterfaceClass: CDC Data */
    0x00,                       /* bInterfaceSubClass */
    0x00,                       /* bInterfaceProtocol */
    0,                          /* iInterface */

    /* ---- Endpoint 2: CDC Data OUT ---- */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x02,                       /* bEndpointAddress: OUT 2 */
    0x02,                       /* bmAttributes: Bulk */
    64, 0,                      /* wMaxPacketSize: 64 */
    0,                          /* bInterval */

    /* ---- Endpoint 3: CDC Data IN ---- */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x83,                       /* bEndpointAddress: IN 3 */
    0x02,                       /* bmAttributes: Bulk */
    64, 0,                      /* wMaxPacketSize: 64 */
    0,                          /* bInterval */

    /* ---- Interface 2: Mass Storage ---- */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: INTERFACE */
    2,                          /* bInterfaceNumber: 2 */
    0,                          /* bAlternateSetting */
    2,                          /* bNumEndpoints: 2 */
    0x08,                       /* bInterfaceClass: Mass Storage */
    0x06,                       /* bInterfaceSubClass: SCSI */
    0x50,                       /* bInterfaceProtocol: BBB */
    4,                          /* iInterface */

    /* ---- Endpoint 4: MSC Data OUT ---- */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x04,                       /* bEndpointAddress: OUT 4 */
    0x02,                       /* bmAttributes: Bulk */
    64, 0,                      /* wMaxPacketSize: 64 */
    0,                          /* bInterval */

    /* ---- Endpoint 5: MSC Data IN ---- */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x85,                       /* bEndpointAddress: IN 5 */
    0x02,                       /* bmAttributes: Bulk */
    64, 0,                      /* wMaxPacketSize: 64 */
    0,                          /* bInterval */
};

/* ---- String descriptors ---- */
static const uint8_t string_lang_id[] = {
    4,                          /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    0x09, 0x04,                 /* wLANGID: English (US) */
};

static const uint8_t string_manufacturer[] = {
    22,                         /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    'H', 0, 'a', 0, 'r', 0, 'd', 0, 'w', 0, 'a', 0, 'r', 0,
    'e', 0, ' ', 0, 'L', 0, 'a', 0, 'b', 0,
};

static const uint8_t string_product[] = {
    20,                         /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    'S', 0, 'k', 0, 'y', 0, 'P', 0, 'i', 0, 'l', 0, 'o', 0,
    't', 0, ' ', 0, 'F', 0, 'C', 0,
};

static const uint8_t string_serial[] = {
    24,                         /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    'S', 0, 'K', 0, 'Y', 0, '0', 0, '0', 0, '0', 0,
    '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '1', 0,
};

static const uint8_t string_config[] = {
    32,                         /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    'C', 0, 'D', 0, 'C', 0, '+', 0, 'M', 0, 'S', 0, 'C', 0,
    ' ', 0, 'C', 0, 'o', 0, 'n', 0, 'f', 0, 'i', 0, 'g', 0,
    'u', 0, 'r', 0, 'a', 0, 't', 0, 'i', 0, 'o', 0, 'n', 0,
};

static const uint8_t string_msc_interface[] = {
    24,                         /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    'F', 0, 'l', 0, 'a', 0, 's', 0, 'h', 0, ' ', 0, 'D', 0,
    'i', 0, 's', 0, 'k', 0, ' ', 0, ' ', 0, ' ', 0,
};

/* ---- Endpoint addresses ---- */
#define EP0_IN      0x80
#define EP0_OUT     0x00
#define EP1_IN      0x81    /* CDC Control (Interrupt) */
#define EP2_OUT     0x02    /* CDC Data (Bulk OUT) */
#define EP3_IN      0x83    /* CDC Data (Bulk IN) */
#define EP4_OUT     0x04    /* MSC Data (Bulk OUT) */
#define EP5_IN      0x85    /* MSC Data (Bulk IN) */

#define USB_MAX_PACKET     64
#define USB_EP0_MAX_PACKET 64

#endif /* SKYPILOT_USB_DESCRIPTORS_H */