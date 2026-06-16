/*
 * usb_descriptors.h - USB Device Descriptors for CyberGuard
 * CTAP2/HID FIDO2 Authenticator + CDC Debug
 *
 * USB VID: 0x1EA8  (jayis1)
 * USB PID: 0xCY01  (CyberGuard Token)
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* ============================================================
 * USB Device Descriptor
 * ============================================================ */
#define USB_VID                 0x1EA8
#define USB_PID                 0xCY01
#define USB_LANG_ID             0x0409   /* English (US) */
#define USB_MANUFACTURER        "jayis1"
#define USB_PRODUCT             "CyberGuard Security Token"
#define USB_SERIAL              "CG-00000001"
#define USB_VERSION             0x0100  /* v1.0 */

/* Device descriptor (18 bytes) */
static const uint8_t usb_device_descriptor[] = {
    0x12,       /* bLength = 18 */
    0x01,       /* bDescriptorType = DEVICE */
    0x00, 0x02, /* bcdUSB = 2.00 */
    0x00,       /* bDeviceClass = 0 (defined at interface level) */
    0x00,       /* bDeviceSubClass = 0 */
    0x00,       /* bDeviceProtocol = 0 */
    0x40,       /* bMaxPacketSize0 = 64 */
    0xA8, 0x1E, /* idVendor = 0x1EA8 */
    0x01, 0xCY, /* idProduct = 0xCY01 */
    0x00, 0x01, /* bcdDevice = 1.00 */
    0x01,       /* iManufacturer = 1 */
    0x02,       /* iProduct = 2 */
    0x03,       /* iSerialNumber = 3 */
    0x01        /* bNumConfigurations = 1 */
};

/* ============================================================
 * USB Configuration Descriptor (Composite: HID + CDC)
 * ============================================================ */

/* Configuration descriptor (9 bytes) */
static const uint8_t usb_config_descriptor[] = {
    /* Configuration descriptor */
    0x09,       /* bLength = 9 */
    0x02,       /* bDescriptorType = CONFIGURATION */
    0x4B, 0x00, /* wTotalLength = 75 bytes */
    0x02,       /* bNumInterfaces = 2 (HID + CDC) */
    0x01,       /* bConfigurationValue = 1 */
    0x00,       /* iConfiguration = 0 */
    0x80,       /* bmAttributes = Bus powered */
    0x32,       /* MaxPower = 100mA (50 * 2) */

    /* ============ Interface 0: FIDO2 HID ============ */
    /* Interface descriptor */
    0x09,       /* bLength = 9 */
    0x04,       /* bDescriptorType = INTERFACE */
    0x00,       /* bInterfaceNumber = 0 */
    0x00,       /* bAlternateSetting = 0 */
    0x02,       /* bNumEndpoints = 2 (IN + OUT) */
    0x03,       /* bInterfaceClass = HID */
    0x01,       /* bInterfaceSubClass = 1 (Boot) */
    0x00,       /* bInterfaceProtocol = 0 */
    0x04,       /* iInterface = 4 */

    /* HID descriptor */
    0x09,       /* bLength = 9 */
    0x21,       /* bDescriptorType = HID */
    0x11, 0x01, /* bcdHID = 1.11 */
    0x00,       /* bCountryCode = 0 */
    0x01,       /* bNumDescriptors = 1 */
    0x22,       /* bDescriptorType = Report */
    0x22, 0x00, /* wDescriptorLength = 34 bytes */

    /* Endpoint 1 IN descriptor (HID IN) */
    0x07,       /* bLength = 7 */
    0x05,       /* bDescriptorType = ENDPOINT */
    0x81,       /* bEndpointAddress = EP1 IN */
    0x03,       /* bmAttributes = Interrupt */
    0x40, 0x00, /* wMaxPacketSize = 64 */
    0x05,       /* bInterval = 5ms */

    /* Endpoint 2 OUT descriptor (HID OUT) */
    0x07,       /* bLength = 7 */
    0x05,       /* bDescriptorType = ENDPOINT */
    0x02,       /* bEndpointAddress = EP2 OUT */
    0x03,       /* bmAttributes = Interrupt */
    0x40, 0x00, /* wMaxPacketSize = 64 */
    0x05,       /* bInterval = 5ms */

    /* ============ Interface 1: CDC (Debug) ============ */
    /* Interface Association Descriptor */
    0x08,       /* bLength = 8 */
    0x0B,       /* bDescriptorType = IAD */
    0x01,       /* bFirstInterface = 1 */
    0x02,       /* bInterfaceCount = 2 */
    0x02,       /* bFunctionClass = CDC */
    0x02,       /* bFunctionSubClass = ACM */
    0x01,       /* bFunctionProtocol = 0x01 */
    0x05,       /* iFunction = 5 */

    /* CDC Interface descriptor (Control) */
    0x09,       /* bLength = 9 */
    0x04,       /* bDescriptorType = INTERFACE */
    0x01,       /* bInterfaceNumber = 1 */
    0x00,       /* bAlternateSetting = 0 */
    0x01,       /* bNumEndpoints = 1 (Notification) */
    0x02,       /* bInterfaceClass = CDC */
    0x02,       /* bInterfaceSubClass = ACM */
    0x01,       /* bInterfaceProtocol = 0x01 */
    0x05,       /* iInterface = 5 */

    /* CDC Header Functional descriptor */
    0x05, 0x24, 0x00, 0x10, 0x01,

    /* CDC Call Management Functional descriptor */
    0x05, 0x24, 0x01, 0x00, 0x03,

    /* CDC ACM Functional descriptor */
    0x04, 0x24, 0x02, 0x02,

    /* CDC Union Functional descriptor */
    0x05, 0x24, 0x06, 0x01, 0x02,

    /* Endpoint 3 IN descriptor (CDC Notification) */
    0x07,       /* bLength = 7 */
    0x05,       /* bDescriptorType = ENDPOINT */
    0x83,       /* bEndpointAddress = EP3 IN */
    0x02,       /* bmAttributes = Bulk */
    0x08, 0x00, /* wMaxPacketSize = 8 */
    0xFF,       /* bInterval = 255ms */

    /* CDC Data Interface descriptor */
    0x09,       /* bLength = 9 */
    0x04,       /* bDescriptorType = INTERFACE */
    0x02,       /* bInterfaceNumber = 2 */
    0x00,       /* bAlternateSetting = 0 */
    0x02,       /* bNumEndpoints = 2 (IN + OUT) */
    0x0A,       /* bInterfaceClass = CDC Data */
    0x00,       /* bInterfaceSubClass = 0 */
    0x00,       /* bInterfaceProtocol = 0 */
    0x06,       /* iInterface = 6 */

    /* Endpoint 4 OUT descriptor (CDC Data OUT) */
    0x07,       /* bLength = 7 */
    0x05,       /* bDescriptorType = ENDPOINT */
    0x04,       /* bEndpointAddress = EP4 OUT */
    0x02,       /* bmAttributes = Bulk */
    0x40, 0x00, /* wMaxPacketSize = 64 */
    0x00,       /* bInterval = 0 */

    /* Endpoint 5 IN descriptor (CDC Data IN) */
    0x07,       /* bLength = 7 */
    0x05,       /* bDescriptorType = ENDPOINT */
    0x85,       /* bEndpointAddress = EP5 IN */
    0x02,       /* bmAttributes = Bulk */
    0x40, 0x00, /* wMaxPacketSize = 64 */
    0x00        /* bInterval = 0 */
};

/* ============================================================
 * HID Report Descriptor (FIDO2 CTAP2)
 * Usage Page: FIDO Alliance (0xF1D0)
 * Usage: CTAP2 Authenticator (0x01)
 * Report size: 64 bytes IN, 64 bytes OUT
 * ============================================================ */
static const uint8_t usb_hid_report_descriptor[] = {
    0x06, 0xD0, 0xF1,   /* Usage Page (FIDO Alliance) */
    0x09, 0x01,          /* Usage (CTAP2 Authenticator) */
    0xA1, 0x01,          /* Collection (Application) */
    0x09, 0x20,          /* Usage (Data IN) */
    0x15, 0x00,          /* Logical Minimum (0) */
    0x26, 0xFF, 0x00,    /* Logical Maximum (255) */
    0x75, 0x08,          /* Report Size (8 bits) */
    0x95, 0x40,          /* Report Count (64 bytes) */
    0x81, 0x02,          /* Input (Data, Variable, Absolute) */
    0x09, 0x21,          /* Usage (Data OUT) */
    0x15, 0x00,          /* Logical Minimum (0) */
    0x26, 0xFF, 0x00,    /* Logical Maximum (255) */
    0x75, 0x08,          /* Report Size (8 bits) */
    0x95, 0x40,          /* Report Count (64 bytes) */
    0x91, 0x02,          /* Output (Data, Variable, Absolute) */
    0xC0                  /* End Collection */
};

/* HID report descriptor length */
#define USB_HID_REPORT_DESC_LEN  34

/* ============================================================
 * String Descriptors
 * ============================================================ */

/* String descriptor 0: Language ID */
static const uint8_t usb_string_lang_id[] = {
    0x04, 0x03, 0x09, 0x04   /* Length=4, Type=3, LangID=0x0409 */
};

/* String descriptor 1: Manufacturer */
static const uint8_t usb_string_manufacturer[] = {
    0x1A, 0x03,   /* Length=26, Type=3 */
    'N', 0x00, 'o', 0x00, 'u', 0x00, 's', 0x00,
    ' ', 0x00, 'R', 0x00, 'e', 0x00, 's', 0x00,
    'e', 0x00, 'a', 0x00, 'r', 0x00, 'c', 0x00,
    'h', 0x00
};

/* String descriptor 2: Product */
static const uint8_t usb_string_product[] = {
    0x30, 0x03,   /* Length=48, Type=3 */
    'C', 0x00, 'y', 0x00, 'b', 0x00, 'e', 0x00,
    'r', 0x00, 'G', 0x00, 'u', 0x00, 'a', 0x00,
    'r', 0x00, 'd', 0x00, ' ', 0x00, 'S', 0x00,
    'e', 0x00, 'c', 0x00, 'u', 0x00, 'r', 0x00,
    'i', 0x00, 't', 0x00, 'y', 0x00, ' ', 0x00,
    'T', 0x00, 'o', 0x00, 'k', 0x00, 'e', 0x00,
    'n', 0x00
};

/* String descriptor 3: Serial Number */
static const uint8_t usb_string_serial[] = {
    0x14, 0x03,   /* Length=20, Type=3 */
    'C', 0x00, 'G', 0x00, '-', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00,
    '1', 0x00
};

/* String descriptor 4: HID Interface */
static const uint8_t usb_string_hid_interface[] = {
    0x16, 0x03,   /* Length=22, Type=3 */
    'F', 0x00, 'I', 0x00, 'D', 0x00, 'O', 0x00,
    '2', 0x00, ' ', 0x00, 'T', 0x00, 'o', 0x00,
    'k', 0x00, 'e', 0x00, 'n', 0x00
};

/* String descriptor 5: CDC Interface */
static const uint8_t usb_string_cdc_interface[] = {
    0x14, 0x03,   /* Length=20, Type=3 */
    'D', 0x00, 'e', 0x00, 'b', 0x00, 'u', 0x00,
    'g', 0x00, ' ', 0x00, 'P', 0x00, 'o', 0x00,
    'r', 0x00, 't', 0x00
};

/* ============================================================
 * CTAP2 HID Command IDs
 * ============================================================ */
#define CTAPHID_MSG             0x03    /* Encapsulated CTAP1 message */
#define CTAPHID_INIT            0x06    /* Channel initialization */
#define CTAPHID_WINK            0x08    /* Wink (blink LED) */
#define CTAPHID_LOCK            0x04    /* Lock channel */
#define CTAPHID_CBOR            0x10    /* Encapsulated CTAP2 CBOR */
#define CTAPHID_CANCEL          0x11    /* Cancel pending request */
#define CTAPHID_KEEPALIVE       0x3B    /* Keep-alive response */
#define CTAPHID_ERROR           0x3F    /* Error response */

/* CTAP2 HID Error Codes */
#define CTAPHID_ERR_INVALID_CMD     0x01
#define CTAPHID_ERR_INVALID_PAR     0x02
#define CTAPHID_ERR_INVALID_LEN     0x03
#define CTAPHID_ERR_INVALID_SEQ     0x04
#define CTAPHID_ERR_MSG_TIMEOUT     0x05
#define CTAPHID_ERR_CHANNEL_BUSY    0x06
#define CTAPHID_ERR_LOCK_REQUIRED   0x0A
#define CTAPHID_ERR_INVALID_CID     0x0B
#define CTAPHID_ERR_OTHER           0x7F

/* USB Endpoint Addresses */
#define USB_EP1_IN          0x81    /* HID IN */
#define USB_EP2_OUT         0x02    /* HID OUT */
#define USB_EP3_IN          0x83    /* CDC Notification IN */
#define USB_EP4_OUT         0x04    /* CDC Data OUT */
#define USB_EP5_IN          0x85    /* CDC Data IN */

#endif /* USB_DESCRIPTORS_H */