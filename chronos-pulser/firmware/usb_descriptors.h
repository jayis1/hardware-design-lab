// usb_descriptors.h — USB Descriptors for Chronos Pulser
// FT601Q USB 3.0 bridge: device, configuration, and string descriptors
// USB 3.0 SuperSpeed + USB 2.0 High-Speed fallback

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

//=============================================================================
// USB Standard Descriptor Types
//=============================================================================

#define USB_DESC_TYPE_DEVICE                    1
#define USB_DESC_TYPE_CONFIGURATION             2
#define USB_DESC_TYPE_STRING                    3
#define USB_DESC_TYPE_INTERFACE                 4
#define USB_DESC_TYPE_ENDPOINT                  5
#define USB_DESC_TYPE_DEVICE_QUALIFIER          6
#define USB_DESC_TYPE_OTHER_SPEED_CONFIG        7
#define USB_DESC_TYPE_BOS                      15
#define USB_DESC_TYPE_DEVICE_CAPABILITY        16
#define USB_DESC_TYPE_SUPERSPEED_ENDPOINT_COMP 48

//=============================================================================
// USB Device Descriptor
//=============================================================================

typedef struct __attribute__((packed)) {
    uint8_t  bLength;            // 18 bytes
    uint8_t  bDescriptorType;    // DEVICE = 1
    uint16_t bcdUSB;             // USB 3.0 = 0x0300
    uint8_t  bDeviceClass;       // 0xFF = vendor-specific
    uint8_t  bDeviceSubClass;    // 0xFF
    uint8_t  bDeviceProtocol;    // 0xFF
    uint8_t  bMaxPacketSize0;    // 512 for USB 3.0 (9 for USB 2.0 EP0)
    uint16_t idVendor;           // 0x16D0 (Nous Research)
    uint16_t idProduct;          // 0x0C50 (Chronos Pulser)
    uint16_t bcdDevice;          // 0x0100 (rev 1.00)
    uint8_t  iManufacturer;      // String index 1
    uint8_t  iProduct;           // String index 2
    uint8_t  iSerialNumber;      // String index 3
    uint8_t  bNumConfigurations; // 1
} usb_device_descriptor_t;

// Chronos Pulser device descriptor
static const usb_device_descriptor_t device_descriptor = {
    .bLength            = sizeof(usb_device_descriptor_t),
    .bDescriptorType    = USB_DESC_TYPE_DEVICE,
    .bcdUSB             = 0x0300,   // USB 3.0
    .bDeviceClass       = 0xFF,     // Vendor-specific
    .bDeviceSubClass    = 0xFF,
    .bDeviceProtocol    = 0xFF,
    .bMaxPacketSize0    = 9,        // EP0 max packet (USB 2.0 portion)
    .idVendor           = 0x16D0,
    .idProduct          = 0x0C50,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1,
};

//=============================================================================
// USB Configuration Descriptor (USB 2.0 High-Speed portion)
//=============================================================================

typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;           // In 2 mA units (250 = 500 mA)
} usb_config_descriptor_t;

typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bInterfaceNumber;
    uint8_t  bAlternateSetting;
    uint8_t  bNumEndpoints;
    uint8_t  bInterfaceClass;     // 0xFF = vendor-specific
    uint8_t  bInterfaceSubClass;
    uint8_t  bInterfaceProtocol;
    uint8_t  iInterface;
} usb_interface_descriptor_t;

typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;    // 0x81 = EP1 IN, 0x01 = EP1 OUT
    uint8_t  bmAttributes;        // 0x02 = Bulk
    uint16_t wMaxPacketSize;      // 512 for HS bulk
    uint8_t  bInterval;           // 0 for bulk
} usb_endpoint_descriptor_t;

// Full configuration descriptor (config + interface + 2 endpoints)
typedef struct __attribute__((packed)) {
    usb_config_descriptor_t    config;
    usb_interface_descriptor_t interface;
    usb_endpoint_descriptor_t  ep_out;   // EP1 OUT — host to device (commands)
    usb_endpoint_descriptor_t  ep_in;    // EP1 IN — device to host (data/responses)
} usb_hs_config_full_t;

static const usb_hs_config_full_t hs_config = {
    .config = {
        .bLength            = sizeof(usb_config_descriptor_t),
        .bDescriptorType    = USB_DESC_TYPE_CONFIGURATION,
        .wTotalLength       = sizeof(usb_hs_config_full_t),
        .bNumInterfaces     = 1,
        .bConfigurationValue = 1,
        .iConfiguration     = 0,
        .bmAttributes       = 0x80,   // Bus-powered, no remote wakeup
        .bMaxPower          = 250,    // 500 mA
    },
    .interface = {
        .bLength            = sizeof(usb_interface_descriptor_t),
        .bDescriptorType    = USB_DESC_TYPE_INTERFACE,
        .bInterfaceNumber   = 0,
        .bAlternateSetting  = 0,
        .bNumEndpoints      = 2,
        .bInterfaceClass    = 0xFF,   // Vendor-specific
        .bInterfaceSubClass = 0xFF,
        .bInterfaceProtocol = 0xFF,
        .iInterface         = 0,
    },
    .ep_out = {
        .bLength            = sizeof(usb_endpoint_descriptor_t),
        .bDescriptorType    = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress   = 0x01,   // EP1 OUT
        .bmAttributes       = 0x02,   // Bulk
        .wMaxPacketSize     = 512,    // HS bulk max packet
        .bInterval          = 0,
    },
    .ep_in = {
        .bLength            = sizeof(usb_endpoint_descriptor_t),
        .bDescriptorType    = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress   = 0x81,   // EP1 IN
        .bmAttributes       = 0x02,   // Bulk
        .wMaxPacketSize     = 512,    // HS bulk max packet
        .bInterval          = 0,
    },
};

//=============================================================================
// USB 3.0 SuperSpeed Endpoint Companion Descriptor
//=============================================================================

typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;    // SUPERSPEED_ENDPOINT_COMP = 48
    uint8_t  bMaxBurst;          // Max packets per burst (0–15)
    uint8_t  bmAttributes;       // [4:0]=MaxStreams, [7:5]=Reserved
    uint16_t wBytesPerInterval;  // Bytes per service interval
} usb_ss_endpoint_comp_descriptor_t;

// SuperSpeed bulk endpoint: up to 16 bursts × 1024 bytes = 16 KB per interval
static const usb_ss_endpoint_comp_descriptor_t ss_ep_comp_out = {
    .bLength            = sizeof(usb_ss_endpoint_comp_descriptor_t),
    .bDescriptorType    = USB_DESC_TYPE_SUPERSPEED_ENDPOINT_COMP,
    .bMaxBurst          = 15,    // 16 packets per burst
    .bmAttributes       = 0,     // No streams
    .wBytesPerInterval  = 0,     // Bulk: ignored
};

static const usb_ss_endpoint_comp_descriptor_t ss_ep_comp_in = {
    .bLength            = sizeof(usb_ss_endpoint_comp_descriptor_t),
    .bDescriptorType    = USB_DESC_TYPE_SUPERSPEED_ENDPOINT_COMP,
    .bMaxBurst          = 15,
    .bmAttributes       = 0,
    .wBytesPerInterval  = 0,
};

//=============================================================================
// USB BOS Descriptor (Binary Object Store — required for USB 3.0)
//=============================================================================

typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;    // BOS = 15
    uint16_t wTotalLength;
    uint8_t  bNumDeviceCaps;
} usb_bos_descriptor_t;

typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;    // DEVICE_CAPABILITY = 16
    uint8_t  bDevCapabilityType; // 1 = USB 2.0 Extension
    uint32_t bmAttributes;       // LPM support bitmask
} usb_dev_cap_usb2ext_t;

typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bDevCapabilityType; // 2 = SuperSpeed USB
    uint8_t  bmAttributes;       // Speed attributes
    uint16_t wSpeedsSupported;   // Supported speed bitmask
    uint8_t  bFunctionalitySupport;
    uint8_t  bU1DevExitLat;      // U1 exit latency in µs
    uint16_t wU2DevExitLat;      // U2 exit latency in µs
} usb_dev_cap_ss_t;

typedef struct __attribute__((packed)) {
    usb_bos_descriptor_t       bos;
    usb_dev_cap_usb2ext_t      usb2ext;
    usb_dev_cap_ss_t           ss;
} usb_bos_full_t;

static const usb_bos_full_t bos_descriptor = {
    .bos = {
        .bLength         = sizeof(usb_bos_descriptor_t),
        .bDescriptorType = USB_DESC_TYPE_BOS,
        .wTotalLength    = sizeof(usb_bos_full_t),
        .bNumDeviceCaps  = 2,
    },
    .usb2ext = {
        .bLength            = sizeof(usb_dev_cap_usb2ext_t),
        .bDescriptorType    = USB_DESC_TYPE_DEVICE_CAPABILITY,
        .bDevCapabilityType = 1,   // USB 2.0 Extension
        .bmAttributes       = 0x00000002,  // LPM capable
    },
    .ss = {
        .bLength            = sizeof(usb_dev_cap_ss_t),
        .bDescriptorType    = USB_DESC_TYPE_DEVICE_CAPABILITY,
        .bDevCapabilityType = 2,   // SuperSpeed
        .bmAttributes       = 0,   // No LTM
        .wSpeedsSupported   = 0x0E, // HS, FS, SS
        .bFunctionalitySupport = 1, // Lowest speed = FS
        .bU1DevExitLat      = 10,  // 10 µs
        .wU2DevExitLat      = 100, // 100 µs
    },
};

//=============================================================================
// USB String Descriptors
//=============================================================================

// String descriptor header
typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;  // STRING = 3
} usb_string_header_t;

// Language ID string (index 0)
static const struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wLANGID;          // 0x0409 = English (US)
} string_langid = {
    .bLength         = 4,
    .bDescriptorType = USB_DESC_TYPE_STRING,
    .wLANGID         = 0x0409,
};

// Manufacturer string: "Nous Research"
static const struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bString[13];      // UTF-16LE
} string_manufacturer = {
    .bLength         = 2 + 26,
    .bDescriptorType = USB_DESC_TYPE_STRING,
    .bString         = { 'N','o','u','s',' ','R','e','s','e','a','r','c','h' },
};

// Product string: "Chronos Pulser"
static const struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bString[14];
} string_product = {
    .bLength         = 2 + 28,
    .bDescriptorType = USB_DESC_TYPE_STRING,
    .bString         = { 'C','h','r','o','n','o','s',' ','P','u','l','s','e','r' },
};

// Serial number string: "CP000001"
static const struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bString[8];
} string_serial = {
    .bLength         = 2 + 16,
    .bDescriptorType = USB_DESC_TYPE_STRING,
    .bString         = { 'C','P','0','0','0','0','0','1' },
};

//=============================================================================
// Descriptor Access Functions
//=============================================================================

// Get descriptor by type and index (for USB control endpoint handler)
// Returns pointer to descriptor and its length
typedef struct {
    const uint8_t *data;
    uint16_t       length;
} descriptor_result_t;

static inline descriptor_result_t usb_get_descriptor(uint8_t type, uint8_t index) {
    descriptor_result_t result = { .data = NULL, .length = 0 };

    switch (type) {
    case USB_DESC_TYPE_DEVICE:
        result.data = (const uint8_t *)&device_descriptor;
        result.length = sizeof(device_descriptor);
        break;

    case USB_DESC_TYPE_CONFIGURATION:
        result.data = (const uint8_t *)&hs_config;
        result.length = sizeof(hs_config);
        break;

    case USB_DESC_TYPE_STRING:
        switch (index) {
        case 0:
            result.data = (const uint8_t *)&string_langid;
            result.length = string_langid.bLength;
            break;
        case 1:
            result.data = (const uint8_t *)&string_manufacturer;
            result.length = string_manufacturer.bLength;
            break;
        case 2:
            result.data = (const uint8_t *)&string_product;
            result.length = string_product.bLength;
            break;
        case 3:
            result.data = (const uint8_t *)&string_serial;
            result.length = string_serial.bLength;
            break;
        default:
            break;
        }
        break;

    case USB_DESC_TYPE_BOS:
        result.data = (const uint8_t *)&bos_descriptor;
        result.length = sizeof(bos_descriptor);
        break;

    default:
        break;
    }

    return result;
}

#endif // USB_DESCRIPTORS_H
