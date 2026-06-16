// usb_descriptors.h — USB Descriptors for Aether-Link Debug Interface
// FT232H USB 2.0 Hi-Speed to MPSSE/JTAG/UART bridge.
// These descriptors define the USB device presented to the host for
// debugging, firmware updates, and JTAG programming.
//
// The FT232H is a fixed-function USB device; these descriptors are
// provided for documentation and for any custom USB implementation
// that may replace the FT232H in future revisions.

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

// ============================================================================
// USB Device Descriptor
// ============================================================================

typedef struct __attribute__((packed)) {
    uint8_t  bLength;            // 18 bytes
    uint8_t  bDescriptorType;    // 0x01 = Device
    uint16_t bcdUSB;             // USB 2.0 = 0x0200
    uint8_t  bDeviceClass;       // 0x00 (defined at interface level)
    uint8_t  bDeviceSubClass;    // 0x00
    uint8_t  bDeviceProtocol;    // 0x00
    uint8_t  bMaxPacketSize0;    // 64 bytes (Hi-Speed)
    uint16_t idVendor;           // 0x0403 (FTDI)
    uint16_t idProduct;          // 0x6014 (FT232H)
    uint16_t bcdDevice;          // 0x0900 (9.00)
    uint8_t  iManufacturer;      // 1 = "jayis1"
    uint8_t  iProduct;           // 2 = "Aether-Link Debug Bridge"
    uint8_t  iSerialNumber;      // 3 = unique serial
    uint8_t  bNumConfigurations; // 1
} usb_device_descriptor_t;

// Aether-Link USB Device Descriptor values
#define USB_DESC_DEVICE {\
    .bLength            = 18,\
    .bDescriptorType    = 0x01,\
    .bcdUSB             = 0x0200,\
    .bDeviceClass       = 0x00,\
    .bDeviceSubClass    = 0x00,\
    .bDeviceProtocol    = 0x00,\
    .bMaxPacketSize0    = 64,\
    .idVendor           = 0x0403,\
    .idProduct          = 0x6014,\
    .bcdDevice          = 0x0900,\
    .iManufacturer      = 1,\
    .iProduct           = 2,\
    .iSerialNumber      = 3,\
    .bNumConfigurations = 1\
}

// ============================================================================
// USB Configuration Descriptor (FT232H with 2 interfaces)
// ============================================================================

typedef struct __attribute__((packed)) {
    uint8_t  bLength;            // 9 bytes
    uint8_t  bDescriptorType;    // 0x02 = Configuration
    uint16_t wTotalLength;       // Total length of config + interfaces + endpoints
    uint8_t  bNumInterfaces;     // 2 (MPSSE + UART)
    uint8_t  bConfigurationValue;// 1
    uint8_t  iConfiguration;     // 0
    uint8_t  bmAttributes;       // 0x80 (bus-powered)
    uint8_t  bMaxPower;          // 50 (100 mA)
} usb_config_descriptor_t;

// ============================================================================
// USB Interface Descriptor (Interface 0: MPSSE for JTAG)
// ============================================================================

typedef struct __attribute__((packed)) {
    uint8_t  bLength;            // 9 bytes
    uint8_t  bDescriptorType;    // 0x04 = Interface
    uint8_t  bInterfaceNumber;   // 0
    uint8_t  bAlternateSetting;  // 0
    uint8_t  bNumEndpoints;      // 2 (IN + OUT)
    uint8_t  bInterfaceClass;    // 0xFF (vendor-specific)
    uint8_t  bInterfaceSubClass; // 0xFF
    uint8_t  bInterfaceProtocol; // 0xFF
    uint8_t  iInterface;         // 4 = "MPSSE JTAG Interface"
} usb_interface_descriptor_t;

// ============================================================================
// USB Endpoint Descriptor (Bulk IN/OUT for MPSSE)
// ============================================================================

typedef struct __attribute__((packed)) {
    uint8_t  bLength;            // 7 bytes
    uint8_t  bDescriptorType;    // 0x05 = Endpoint
    uint8_t  bEndpointAddress;   // 0x82 (EP2 IN) or 0x02 (EP2 OUT)
    uint8_t  bmAttributes;       // 0x02 (Bulk)
    uint16_t wMaxPacketSize;     // 512 bytes (Hi-Speed bulk)
    uint8_t  bInterval;         // 0 (bulk endpoints ignore this)
} usb_endpoint_descriptor_t;

// ============================================================================
// USB Interface Descriptor (Interface 1: UART for debug console)
// ============================================================================

typedef struct __attribute__((packed)) {
    uint8_t  bLength;            // 9 bytes
    uint8_t  bDescriptorType;    // 0x04 = Interface
    uint8_t  bInterfaceNumber;   // 1
    uint8_t  bAlternateSetting;  // 0
    uint8_t  bNumEndpoints;      // 2 (IN + OUT)
    uint8_t  bInterfaceClass;    // 0xFF (vendor-specific)
    uint8_t  bInterfaceSubClass; // 0xFF
    uint8_t  bInterfaceProtocol; // 0xFF
    uint8_t  iInterface;         // 5 = "Debug UART Interface"
} usb_interface_uart_descriptor_t;

// ============================================================================
// USB String Descriptors
// ============================================================================

// String descriptor header
typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;    // 0x03 = String
} usb_string_descriptor_header_t;

// String index definitions
#define USB_STRING_IDX_MANUFACTURER  1
#define USB_STRING_IDX_PRODUCT      2
#define USB_STRING_IDX_SERIAL       3
#define USB_STRING_IDX_MPSSE        4
#define USB_STRING_IDX_UART         5

// Manufacturer string: "jayis1"
#define USB_STRING_MANUFACTURER \
    { 26, 0x03, 'N',0, 'o',0, 'u',0, 's',0, ' ',0, 'R',0, 'e',0, 's',0, 'e',0, 'a',0, 'r',0, 'c',0, 'h',0 }

// Product string: "Aether-Link Debug Bridge"
#define USB_STRING_PRODUCT \
    { 50, 0x03, 'A',0, 'e',0, 't',0, 'h',0, 'e',0, 'r',0, '-',0, 'L',0, 'i',0, 'n',0, 'k',0, ' ',0, \
      'D',0, 'e',0, 'b',0, 'u',0, 'g',0, ' ',0, 'B',0, 'r',0, 'i',0, 'd',0, 'g',0, 'e',0 }

// Serial string: unique per device (e.g., "AETH-0001")
#define USB_STRING_SERIAL \
    { 20, 0x03, 'A',0, 'E',0, 'T',0, 'H',0, '-',0, '0',0, '0',0, '0',0, '1',0 }

// ============================================================================
// Complete Configuration Descriptor Assembly
// ============================================================================

// Total configuration descriptor (config + 2 interfaces + 4 endpoints)
// 9 (config) + 9 (iface0) + 7 (ep2in) + 7 (ep2out) + 9 (iface1) + 7 (ep3in) + 7 (ep3out) = 55 bytes

typedef struct __attribute__((packed)) {
    usb_config_descriptor_t        config;
    usb_interface_descriptor_t     iface_mpsee;
    usb_endpoint_descriptor_t      ep_mpsee_in;
    usb_endpoint_descriptor_t      ep_mpsee_out;
    usb_interface_uart_descriptor_t iface_uart;
    usb_endpoint_descriptor_t      ep_uart_in;
    usb_endpoint_descriptor_t      ep_uart_out;
} usb_full_config_t;

// ============================================================================
// USB Device Qualifier Descriptor (for Hi-Speed capable devices)
// ============================================================================

typedef struct __attribute__((packed)) {
    uint8_t  bLength;            // 10 bytes
    uint8_t  bDescriptorType;    // 0x06 = Device Qualifier
    uint16_t bcdUSB;             // 0x0200
    uint8_t  bDeviceClass;       // 0x00
    uint8_t  bDeviceSubClass;    // 0x00
    uint8_t  bDeviceProtocol;    // 0x00
    uint8_t  bMaxPacketSize0;    // 8 (Full-Speed alternate)
    uint8_t  bNumConfigurations; // 1
    uint8_t  bReserved;          // 0
} usb_device_qualifier_descriptor_t;

// ============================================================================
// USB Binary Device Object Store (BOS) Descriptor
// ============================================================================

typedef struct __attribute__((packed)) {
    uint8_t  bLength;            // 5 bytes
    uint8_t  bDescriptorType;    // 0x0F = BOS
    uint16_t wTotalLength;       // Total length
    uint8_t  bNumDeviceCaps;     // Number of device capability descriptors
} usb_bos_descriptor_t;

// USB 2.0 Extension Device Capability
typedef struct __attribute__((packed)) {
    uint8_t  bLength;            // 7 bytes
    uint8_t  bDescriptorType;    // 0x10 = Device Capability
    uint8_t  bDevCapabilityType; // 0x02 = USB 2.0 Extension
    uint32_t bmAttributes;       // LPM support bitmask
} usb_dev_cap_usb20ext_t;

// ============================================================================
// FT232H-Specific: MPSSE Command Protocol
// ============================================================================

// MPSSE commands used for JTAG communication
#define MPSSE_CMD_SET_BITS_LOW      0x80
#define MPSSE_CMD_SET_BITS_HIGH     0x82
#define MPSSE_CMD_GET_BITS_LOW      0x81
#define MPSSE_CMD_GET_BITS_HIGH     0x83
#define MPSSE_CMD_LOOPBACK_ENABLE   0x84
#define MPSSE_CMD_LOOPBACK_DISABLE  0x85
#define MPSSE_CMD_SET_TCK_DIVISOR   0x86
#define MPSSE_CMD_SEND_IMMEDIATE    0x87
#define MPSSE_CMD_WAIT_ON_HIGH      0x88
#define MPSSE_CMD_WAIT_ON_LOW       0x89
#define MPSSE_CMD_ENABLE_CLK_DIV5   0x8A
#define MPSSE_CMD_DISABLE_CLK_DIV5  0x8B
#define MPSSE_CMD_ENABLE_3PH_CLK    0x8C
#define MPSSE_CMD_DISABLE_3PH_CLK   0x8D
#define MPSSE_CMD_CLK_N_CYCLES      0x8E
#define MPSSE_CMD_CLK_N8_CYCLES     0x8F
#define MPSSE_CMD_CLK_N8_CYCLES_MSB 0x90
#define MPSSE_CMD_CLK_TO_HIGH       0x94
#define MPSSE_CMD_CLK_TO_LOW        0x95
#define MPSSE_CMD_ENABLE_ADAPTIVE   0x96
#define MPSSE_CMD_DISABLE_ADAPTIVE  0x97
#define MPSSE_CMD_CLK_N8_CYCLES_MSB_TDI 0x9E

// JTAG pin mapping on FT232H MPSSE
#define MPSSE_PIN_TCK   0x01  // ADBUS0
#define MPSSE_PIN_TDI   0x02  // ADBUS1
#define MPSSE_PIN_TDO   0x04  // ADBUS2
#define MPSSE_PIN_TMS   0x08  // ADBUS3
#define MPSSE_PIN_GPIOL0 0x10 // ADBUS4 (optional)
#define MPSSE_PIN_GPIOL1 0x20 // ADBUS5 (optional)
#define MPSSE_PIN_GPIOL2 0x40 // ADBUS6 (optional)
#define MPSSE_PIN_GPIOL3 0x80 // ADBUS7 (optional)

// ============================================================================
// USB Power Management
// ============================================================================

// FT232H power consumption
#define FT232H_VCCIO_CURRENT_MA   25   // 3.3V I/O supply
#define FT232H_VCORE_CURRENT_MA   15   // 1.8V core supply
#define FT232H_TOTAL_CURRENT_MA   40   // Total from USB VBUS

// USB suspend current limit: 2.5 mA (bus-powered, Hi-Speed)
// FT232H supports USB suspend with remote wakeup

// ============================================================================
// Descriptor Table for Quick Reference
// ============================================================================

// Complete USB descriptor table (all descriptors in order)
typedef struct {
    const usb_device_descriptor_t           *device;
    const usb_config_descriptor_t           *config;
    const usb_interface_descriptor_t        *iface_mpsee;
    const usb_endpoint_descriptor_t         *ep_mpsee_in;
    const usb_endpoint_descriptor_t         *ep_mpsee_out;
    const usb_interface_uart_descriptor_t   *iface_uart;
    const usb_endpoint_descriptor_t         *ep_uart_in;
    const usb_endpoint_descriptor_t         *ep_uart_out;
    const usb_device_qualifier_descriptor_t *qualifier;
    const uint8_t                           *string_manufacturer;
    const uint8_t                           *string_product;
    const uint8_t                           *string_serial;
    const uint8_t                           *string_mpsee;
    const uint8_t                           *string_uart;
} usb_descriptor_table_t;

#endif // USB_DESCRIPTORS_H
