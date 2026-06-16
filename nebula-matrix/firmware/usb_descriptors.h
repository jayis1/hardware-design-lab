/**
 * usb_descriptors.h — USB Descriptors for Nebula Matrix RNDIS Configuration
 *
 * Defines USB device, configuration, and string descriptors for the
 * Nebula Matrix when operating as a USB RNDIS (Ethernet-over-USB) device.
 *
 * The device appears as a CDC RNDIS network adapter to the host,
 * enabling web-based configuration via HTTP over the virtual Ethernet link.
 *
 * USB 2.0 Full Speed (12 Mbps), bus-powered, single configuration.
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include "tusb.h"
#include <stdint.h>

/* =========================================================================
 * USB Vendor & Product IDs
 * ========================================================================= */

#define USB_VID_NOUS_RESEARCH       0x2E8A  /* Placeholder — would be assigned */
#define USB_PID_NEBULA_MATRIX       0x0004
#define USB_BCD_DEVICE              0x0100  /* Version 1.0 */

/* =========================================================================
 * String Descriptor Indices
 * ========================================================================= */

enum {
    STRID_LANGID    = 0,
    STRID_MANUF     = 1,
    STRID_PRODUCT   = 2,
    STRID_SERIAL    = 3,
    STRID_CONFIG    = 4,
    STRID_RNDIS_IF  = 5,
};

/* =========================================================================
 * String Descriptor Content
 * ========================================================================= */

/* Language ID: US English (0x0409) */
#define USB_LANGID_STRING       "\x09\x04"

/* Manufacturer string */
#define USB_MANUF_STRING        "Nous Research"

/* Product string */
#define USB_PRODUCT_STRING      "Nebula Matrix LED Engine"

/* Serial number — unique per device (would be programmed at factory) */
#define USB_SERIAL_STRING       "NM-2026-0001"

/* Configuration string */
#define USB_CONFIG_STRING       "RNDIS Configuration"

/* Interface string */
#define USB_RNDIS_IF_STRING     "Nebula Matrix RNDIS Control"

/* =========================================================================
 * Device Descriptor
 * ========================================================================= */

#define USB_DEVICE_DESCRIPTOR \
    0x12,                       /* bLength: 18 bytes */ \
    TUD_DESC_TYPE_DEVICE,       /* bDescriptorType: DEVICE (0x01) */ \
    0x00, 0x02,                 /* bcdUSB: USB 2.0 */ \
    0x02,                       /* bDeviceClass: CDC (0x02) */ \
    0x00,                       /* bDeviceSubClass: 0 */ \
    0x00,                       /* bDeviceProtocol: 0 */ \
    64,                         /* bMaxPacketSize0: 64 bytes */ \
    U16_TO_U8S_LE(USB_VID_NOUS_RESEARCH),  /* idVendor */ \
    U16_TO_U8S_LE(USB_PID_NEBULA_MATRIX),  /* idProduct */ \
    U16_TO_U8S_LE(USB_BCD_DEVICE),         /* bcdDevice */ \
    STRID_MANUF,                /* iManufacturer */ \
    STRID_PRODUCT,              /* iProduct */ \
    STRID_SERIAL,               /* iSerialNumber */ \
    0x01                        /* bNumConfigurations: 1 */

/* =========================================================================
 * Configuration Descriptor (RNDIS)
 * =========================================================================
 *
 * RNDIS uses two interfaces:
 *   Interface 0: CDC Communication Class (control channel)
 *     - Endpoint 0x82: IN, Interrupt, 8 bytes
 *   Interface 1: CDC Data Class (data channel)
 *     - Endpoint 0x81: IN, Bulk, 64 bytes
 *     - Endpoint 0x01: OUT, Bulk, 64 bytes
 */

#define USB_RNDIS_CONFIG_DESCRIPTOR \
    /* Configuration Descriptor */ \
    0x09,                       /* bLength: 9 */ \
    TUD_DESC_TYPE_CONFIG,       /* bDescriptorType: CONFIGURATION (0x02) */ \
    U16_TO_U8S_LE(62),          /* wTotalLength: 62 bytes */ \
    0x02,                       /* bNumInterfaces: 2 */ \
    0x01,                       /* bConfigurationValue: 1 */ \
    STRID_CONFIG,               /* iConfiguration */ \
    0xC0,                       /* bmAttributes: Self-powered, no remote wakeup */ \
    0x32,                       /* bMaxPower: 100 mA (0x32 * 2mA) */ \
    \
    /* IAD (Interface Association Descriptor) for CDC RNDIS */ \
    0x08,                       /* bLength: 8 */ \
    0x0B,                       /* bDescriptorType: IAD (0x0B) */ \
    0x00,                       /* bFirstInterface: 0 */ \
    0x02,                       /* bInterfaceCount: 2 */ \
    0x02,                       /* bFunctionClass: CDC (0x02) */ \
    0x02,                       /* bFunctionSubClass: ACM (0x02) */ \
    0xFF,                       /* bFunctionProtocol: Vendor-specific (RNDIS) */ \
    STRID_RNDIS_IF,             /* iFunction */ \
    \
    /* Interface 0: CDC Communication Class */ \
    0x09,                       /* bLength: 9 */ \
    TUD_DESC_TYPE_INTERFACE,    /* bDescriptorType: INTERFACE (0x04) */ \
    0x00,                       /* bInterfaceNumber: 0 */ \
    0x00,                       /* bAlternateSetting: 0 */ \
    0x01,                       /* bNumEndpoints: 1 */ \
    0x02,                       /* bInterfaceClass: CDC (0x02) */ \
    0x02,                       /* bInterfaceSubClass: ACM (0x02) */ \
    0xFF,                       /* bInterfaceProtocol: Vendor-specific (RNDIS) */ \
    STRID_RNDIS_IF,             /* iInterface */ \
    \
    /* CDC Header Functional Descriptor */ \
    0x05,                       /* bFunctionalLength: 5 */ \
    0x24,                       /* bDescriptorType: CS_INTERFACE (0x24) */ \
    0x00,                       /* bDescriptorSubtype: Header (0x00) */ \
    0x10, 0x01,                 /* bcdCDC: 1.10 */ \
    \
    /* CDC Call Management Functional Descriptor */ \
    0x05,                       /* bFunctionalLength: 5 */ \
    0x24,                       /* bDescriptorType: CS_INTERFACE */ \
    0x01,                       /* bDescriptorSubtype: Call Management (0x01) */ \
    0x00,                       /* bmCapabilities: No call management */ \
    0x01,                       /* bDataInterface: Interface 1 */ \
    \
    /* CDC ACM Functional Descriptor */ \
    0x04,                       /* bFunctionalLength: 4 */ \
    0x24,                       /* bDescriptorType: CS_INTERFACE */ \
    0x02,                       /* bDescriptorSubtype: ACM (0x02) */ \
    0x00,                       /* bmCapabilities: No ACM features */ \
    \
    /* CDC Union Functional Descriptor */ \
    0x05,                       /* bFunctionalLength: 5 */ \
    0x24,                       /* bDescriptorType: CS_INTERFACE */ \
    0x06,                       /* bDescriptorSubtype: Union (0x06) */ \
    0x00,                       /* bControlInterface: 0 */ \
    0x01,                       /* bSubordinateInterface: 1 */ \
    \
    /* Endpoint 2 IN: Interrupt (notification) */ \
    0x07,                       /* bLength: 7 */ \
    TUD_DESC_TYPE_ENDPOINT,     /* bDescriptorType: ENDPOINT (0x05) */ \
    0x82,                       /* bEndpointAddress: IN, Endpoint 2 */ \
    0x03,                       /* bmAttributes: Interrupt */ \
    U16_TO_U8S_LE(8),           /* wMaxPacketSize: 8 bytes */ \
    0x10,                       /* bInterval: 16 ms (2^4 * 1ms) */ \
    \
    /* Interface 1: CDC Data Class */ \
    0x09,                       /* bLength: 9 */ \
    TUD_DESC_TYPE_INTERFACE,    /* bDescriptorType: INTERFACE */ \
    0x01,                       /* bInterfaceNumber: 1 */ \
    0x00,                       /* bAlternateSetting: 0 */ \
    0x02,                       /* bNumEndpoints: 2 */ \
    0x0A,                       /* bInterfaceClass: CDC Data (0x0A) */ \
    0x00,                       /* bInterfaceSubClass: 0 */ \
    0x00,                       /* bInterfaceProtocol: 0 */ \
    0x00,                       /* iInterface: None */ \
    \
    /* Endpoint 1 IN: Bulk (data from device to host) */ \
    0x07,                       /* bLength: 7 */ \
    TUD_DESC_TYPE_ENDPOINT,     /* bDescriptorType: ENDPOINT */ \
    0x81,                       /* bEndpointAddress: IN, Endpoint 1 */ \
    0x02,                       /* bmAttributes: Bulk */ \
    U16_TO_U8S_LE(64),          /* wMaxPacketSize: 64 bytes */ \
    0x00,                       /* bInterval: N/A for bulk */ \
    \
    /* Endpoint 1 OUT: Bulk (data from host to device) */ \
    0x07,                       /* bLength: 7 */ \
    TUD_DESC_TYPE_ENDPOINT,     /* bDescriptorType: ENDPOINT */ \
    0x01,                       /* bEndpointAddress: OUT, Endpoint 1 */ \
    0x02,                       /* bmAttributes: Bulk */ \
    U16_TO_U8S_LE(64),          /* wMaxPacketSize: 64 bytes */ \
    0x00                        /* bInterval: N/A for bulk */

/* =========================================================================
 * Microsoft OS 2.0 Descriptor (for RNDIS driver matching on Windows)
 * =========================================================================
 *
 * Windows requires a special MS OS 2.0 descriptor set to automatically
 * load the RNDIS driver. This is requested via a vendor-specific
 * BOS descriptor request.
 */

/* BOS Descriptor */
#define USB_BOS_DESCRIPTOR \
    0x05,                       /* bLength: 5 */ \
    0x0F,                       /* bDescriptorType: BOS (0x0F) */ \
    U16_TO_U8S_LE(0x0022),      /* wTotalLength: 34 bytes */ \
    0x02,                       /* bNumDeviceCaps: 2 */ \
    \
    /* Platform Descriptor (Microsoft OS 2.0) */ \
    0x1C,                       /* bLength: 28 bytes */ \
    0x10,                       /* bDescriptorType: Device Capability (0x10) */ \
    0x05,                       /* bDevCapabilityType: Platform (0x05) */ \
    0x00,                       /* bReserved */ \
    0xDF, 0x60, 0xDD, 0xD8,    /* PlatformCapabilityUUID: MS OS 2.0 */ \
    0x89, 0x45, 0xC7, 0x4C,    \
    0x9C, 0xD2, 0x65, 0x9D,    \
    0x9E, 0x64, 0x8A, 0x9F,    \
    0x00, 0x00, 0x00, 0x0A,    /* dwWindowsVersion: Windows 10 */ \
    U16_TO_U8S_LE(0x00F2),      /* wMSOSDescriptorSetTotalLength: 242 bytes */ \
    0x01,                       /* bMS_VendorCode: 0x01 */ \
    0x00,                       /* bAltEnumCode: 0 */ \
    \
    /* WebUSB Descriptor (for browser-based configuration) */ \
    0x05,                       /* bLength: 5 */ \
    0x10,                       /* bDescriptorType: Device Capability */ \
    0x06,                       /* bDevCapabilityType: WebUSB (0x06) */ \
    0x00,                       /* bReserved */ \
    0x01                        /* bcdVersion: 1.0 */

/* =========================================================================
 * MS OS 2.0 Descriptor Set (RNDIS Compatible ID)
 * =========================================================================
 *
 * This tells Windows to use the RNDIS driver for this device.
 */

#define USB_MS_OS2_DESCRIPTOR_SET \
    /* Set Header */ \
    0x0A, 0x00,                 /* wLength: 10 */ \
    0x00, 0x00,                 /* wDescriptorType: MS OS 2.0 Set Header (0x0000) */ \
    0x00, 0x00, 0x00, 0x0A,    /* dwWindowsVersion: Windows 10 */ \
    0xF2, 0x00,                 /* wTotalLength: 242 bytes */ \
    \
    /* Compatible ID Descriptor */ \
    0x28, 0x00,                 /* wLength: 40 */ \
    0x03, 0x00,                 /* wDescriptorType: Compatible ID (0x0003) */ \
    'R', 0x00, 'N', 0x00,      /* CompatibleID[0]: "RNDIS\0\0" */ \
    'D', 0x00, 'I', 0x00,      \
    'S', 0x00, 0x00, 0x00,     \
    0x00, 0x00, 0x00, 0x00,    /* SubCompatibleID[0]: "" */ \
    0x00, 0x00, 0x00, 0x00,    \
    0x00, 0x00, 0x00, 0x00,    \
    0x00, 0x00, 0x00, 0x00,    \
    0x00, 0x00, 0x00, 0x00,    \
    0x00, 0x00, 0x00, 0x00,    \
    \
    /* Registry Property Descriptor */ \
    0xCE, 0x00,                 /* wLength: 206 */ \
    0x04, 0x00,                 /* wDescriptorType: Registry Property (0x0004) */ \
    0x07, 0x00,                 /* wPropertyDataType: REG_MULTI_SZ (0x0007) */ \
    0xBE, 0x00,                 /* wPropertyNameLength: 190 */ \
    \
    /* Property Name: "DeviceRNDISInterfaceGUID" */ \
    'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, \
    'R', 0x00, 'N', 0x00, 'D', 0x00, 'I', 0x00, 'S', 0x00, \
    'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00, 'r', 0x00, \
    'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, \
    'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 0x00, 0x00, \
    \
    0x10, 0x00,                 /* wPropertyDataLength: 16 */ \
    \
    /* Property Data: GUID for RNDIS */ \
    0x00, 0x00, 0x00, 0x00,    /* {00000000-0000-0000-0000-000000000000} */ \
    0x00, 0x00, 0x00, 0x00,    /* (Placeholder — actual GUID assigned at runtime) */ \
    0x00, 0x00, 0x00, 0x00,    \
    0x00, 0x00, 0x00, 0x00

/* =========================================================================
 * String Descriptor Table
 * =========================================================================
 *
 * Array of string descriptor pointers for TinyUSB.
 * Index matches STRID_* enum values.
 */

static const char *usb_string_descriptors[] = {
    [STRID_LANGID]    = USB_LANGID_STRING,
    [STRID_MANUF]     = USB_MANUF_STRING,
    [STRID_PRODUCT]   = USB_PRODUCT_STRING,
    [STRID_SERIAL]    = USB_SERIAL_STRING,
    [STRID_CONFIG]    = USB_CONFIG_STRING,
    [STRID_RNDIS_IF]  = USB_RNDIS_IF_STRING,
};

/* =========================================================================
 * TinyUSB Configuration
 * ========================================================================= */

#define TUD_RNDIS_ENABLED       1
#define TUD_RNDIS_MTU           1500
#define TUD_RNDIS_VENDOR_ID     USB_VID_NOUS_RESEARCH
#define TUD_RNDIS_PRODUCT_ID    USB_PID_NEBULA_MATRIX

/* RNDIS MAC address */
#define TUD_RNDIS_MAC_ADDR      {0x02, 0x00, 0x00, 0x00, 0x00, 0x01}

/* RNDIS vendor description */
#define TUD_RNDIS_VENDOR_DESC   "Nous Research"
#define TUD_RNDIS_PRODUCT_DESC  "Nebula Matrix LED Engine"

/* =========================================================================
 * Endpoint Configuration
 * ========================================================================= */

/* CDC Notification endpoint (Interrupt IN) */
#define EPNUM_RNDIS_NOTIF       0x02
/* CDC Data endpoints (Bulk IN/OUT) */
#define EPNUM_RNDIS_IN          0x01
#define EPNUM_RNDIS_OUT         0x01

/* Endpoint sizes */
#define RNDIS_NOTIF_EPSIZE      8
#define RNDIS_DATA_EPSIZE       64

/* =========================================================================
 * USB Hardware Configuration (STM32H7 FS Device)
 * ========================================================================= */

/* STM32 FSDEV has 8 endpoints (0-7), each bidirectional.
 * Endpoint 0 is reserved for control transfers.
 * We use:
 *   EP1: RNDIS data (IN + OUT, Bulk, 64 bytes)
 *   EP2: RNDIS notification (IN, Interrupt, 8 bytes)
 */

#define USB_FSDEV_EP_COUNT      8
#define USB_FSDEV_EP0_SIZE      64

/* =========================================================================
 * RNDIS OID (Object Identifier) Handlers
 * =========================================================================
 *
 * RNDIS uses OIDs for configuration queries. Key OIDs we support:
 */

/* Mandatory OIDs */
#define OID_GEN_SUPPORTED_LIST          0x00010101
#define OID_GEN_HARDWARE_STATUS         0x00010102
#define OID_GEN_MEDIA_SUPPORTED         0x00010103
#define OID_GEN_MEDIA_IN_USE            0x00010104
#define OID_GEN_MAXIMUM_FRAME_SIZE      0x00010106
#define OID_GEN_LINK_SPEED              0x00010107
#define OID_GEN_TRANSMIT_BUFFER_SPACE   0x00010108
#define OID_GEN_RECEIVE_BUFFER_SPACE    0x00010109
#define OID_GEN_VENDOR_ID               0x0001010A
#define OID_GEN_VENDOR_DESCRIPTION      0x0001010B
#define OID_GEN_VENDOR_DRIVER_VERSION   0x0001010C
#define OID_GEN_CURRENT_PACKET_FILTER   0x0001010E
#define OID_GEN_MAXIMUM_TOTAL_SIZE      0x00010111
#define OID_GEN_MEDIA_CONNECT_STATUS    0x00010114
#define OID_GEN_PHYSICAL_MEDIUM         0x00010115

/* Optional OIDs */
#define OID_GEN_XMIT_OK                 0x00020101
#define OID_GEN_RCV_OK                  0x00020102
#define OID_GEN_XMIT_ERROR              0x00020103
#define OID_GEN_RCV_ERROR               0x00020104
#define OID_GEN_RCV_NO_BUFFER           0x00020105

/* 802.3 OIDs (Ethernet) */
#define OID_802_3_PERMANENT_ADDRESS     0x01010101
#define OID_802_3_CURRENT_ADDRESS       0x01010102
#define OID_802_3_MULTICAST_LIST        0x01010103
#define OID_802_3_MAXIMUM_LIST_SIZE     0x01010104
#define OID_802_3_MAC_OPTIONS           0x01010105
#define OID_802_3_RCV_ERROR_ALIGNMENT   0x01020101
#define OID_802_3_XMIT_ONE_COLLISION    0x01020102
#define OID_802_3_XMIT_MORE_COLLISIONS  0x01020103

/* =========================================================================
 * RNDIS Media Status
 * ========================================================================= */

#define RNDIS_MEDIA_STATE_CONNECTED     0x00000000
#define RNDIS_MEDIA_STATE_DISCONNECTED  0x00000001

/* =========================================================================
 * RNDIS Packet Filter Bits
 * ========================================================================= */

#define RNDIS_PACKET_FILTER_DIRECTED        0x00000001
#define RNDIS_PACKET_FILTER_MULTICAST       0x00000002
#define RNDIS_PACKET_FILTER_ALL_MULTICAST   0x00000004
#define RNDIS_PACKET_FILTER_BROADCAST       0x00000008
#define RNDIS_PACKET_FILTER_PROMISCUOUS     0x00000020

/* Default filter: directed + broadcast + multicast */
#define RNDIS_DEFAULT_FILTER \
    (RNDIS_PACKET_FILTER_DIRECTED | \
     RNDIS_PACKET_FILTER_BROADCAST | \
     RNDIS_PACKET_FILTER_ALL_MULTICAST)

/* =========================================================================
 * RNDIS Link Speed
 * ========================================================================= */

#define RNDIS_LINK_SPEED_BPS        100000000   /* 100 Mbps (USB FS limit) */

/* =========================================================================
 * RNDIS Buffer Sizes
 * ========================================================================= */

#define RNDIS_MAX_FRAME_SIZE        1500
#define RNDIS_MAX_TOTAL_SIZE        1558
#define RNDIS_TX_BUFFER_SPACE       8192
#define RNDIS_RX_BUFFER_SPACE       8192

#endif /* USB_DESCRIPTORS_H */
