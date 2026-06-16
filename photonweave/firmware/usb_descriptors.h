/**
 * usb_descriptors.h — USB 3.0 Descriptors for PhotonWeave
 *
 * USB descriptors used by the FT601Q USB 3.0 FIFO bridge for
 * device enumeration. The FT601 presents as a vendor-specific
 * USB 3.0 SuperSpeed device with bulk IN/OUT endpoints.
 *
 * These descriptors are loaded into the FT601 configuration
 * EEPROM and also used by the STM32 for USB command processing.
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

//=============================================================================
// USB Vendor/Product IDs
//=============================================================================
#define USB_VID_NOUS            0x4E4F  // "NO" in ASCII
#define USB_PID_PHOTONWEAVE     0x5057  // "PW" in ASCII
#define USB_BCD_DEVICE          0x0100  // Version 1.0

//=============================================================================
// USB 3.0 Device Descriptor
//=============================================================================
static const uint8_t usb_device_descriptor[] = {
    0x12,                       // bLength (18 bytes)
    0x01,                       // bDescriptorType: DEVICE
    0x00, 0x03,                 // bcdUSB: 3.0
    0x00,                       // bDeviceClass: vendor-specific
    0x00,                       // bDeviceSubClass
    0x00,                       // bDeviceProtocol
    0x09,                       // bMaxPacketSize0: 512 (2^9) for SuperSpeed
    (uint8_t)(USB_VID_NOUS & 0xFF),
    (uint8_t)((USB_VID_NOUS >> 8) & 0xFF),
    (uint8_t)(USB_PID_PHOTONWEAVE & 0xFF),
    (uint8_t)((USB_PID_PHOTONWEAVE >> 8) & 0xFF),
    (uint8_t)(USB_BCD_DEVICE & 0xFF),
    (uint8_t)((USB_BCD_DEVICE >> 8) & 0xFF),
    0x01,                       // iManufacturer: string index 1
    0x02,                       // iProduct: string index 2
    0x03,                       // iSerialNumber: string index 3
    0x01                        // bNumConfigurations: 1
};

//=============================================================================
// USB 3.0 Configuration Descriptor
// Single configuration with one vendor-specific interface
// and two bulk endpoints (IN + OUT) with SuperSpeed companion descriptors
//=============================================================================
static const uint8_t usb_config_descriptor[] = {
    // ── Configuration Descriptor ──
    0x09,                       // bLength
    0x02,                       // bDescriptorType: CONFIGURATION
    0x2C, 0x00,                 // wTotalLength: 44 bytes
    0x01,                       // bNumInterfaces
    0x01,                       // bConfigurationValue
    0x00,                       // iConfiguration
    0x80,                       // bmAttributes: bus-powered, no remote wakeup
    0xFA,                       // bMaxPower: 500 mA × 2 = 1000 mA

    // ── Interface Descriptor (Interface 0, Alternate 0) ──
    0x09,                       // bLength
    0x04,                       // bDescriptorType: INTERFACE
    0x00,                       // bInterfaceNumber
    0x00,                       // bAlternateSetting
    0x02,                       // bNumEndpoints (2: Bulk IN + Bulk OUT)
    0xFF,                       // bInterfaceClass: vendor-specific
    0xFF,                       // bInterfaceSubClass: vendor-specific
    0xFF,                       // bInterfaceProtocol: vendor-specific
    0x00,                       // iInterface

    // ── Bulk OUT Endpoint Descriptor (Host → Device, EP1 OUT) ──
    0x07,                       // bLength
    0x05,                       // bDescriptorType: ENDPOINT
    0x01,                       // bEndpointAddress: EP1 OUT
    0x02,                       // bmAttributes: Bulk
    0x00, 0x04,                 // wMaxPacketSize: 1024 bytes
    0x00,                       // bInterval: ignored for bulk
    0x08,                       // bMaxBurst: 8 bursts per interval
    0x00,                       // bmAttributesMaxBurst: streams not used

    // ── SuperSpeed Endpoint Companion (Bulk OUT) ──
    0x06,                       // bLength
    0x30,                       // bDescriptorType: SS_ENDPOINT_COMPANION
    0x08,                       // bMaxBurst: 8 packets per burst
    0x00,                       // bmAttributes: bulk, no streams
    0x00, 0x00,                 // wBytesPerInterval: 0 (not periodic)

    // ── Bulk IN Endpoint Descriptor (Device → Host, EP2 IN) ──
    0x07,                       // bLength
    0x05,                       // bDescriptorType: ENDPOINT
    0x82,                       // bEndpointAddress: EP2 IN
    0x02,                       // bmAttributes: Bulk
    0x00, 0x04,                 // wMaxPacketSize: 1024 bytes
    0x00,                       // bInterval
    0x08,                       // bMaxBurst: 8 bursts
    0x00,                       // bmAttributesMaxBurst

    // ── SuperSpeed Endpoint Companion (Bulk IN) ──
    0x06,                       // bLength
    0x30,                       // bDescriptorType: SS_ENDPOINT_COMPANION
    0x08,                       // bMaxBurst
    0x00,                       // bmAttributes
    0x00, 0x00                  // wBytesPerInterval
};

//=============================================================================
// USB 3.0 BOS (Binary Object Store) Descriptor
// Required for SuperSpeed devices. Declares USB 2.0 Extension and
// SuperSpeed USB Device Capability descriptors.
//=============================================================================
static const uint8_t usb_bos_descriptor[] = {
    // ── BOS Descriptor ──
    0x05,                       // bLength
    0x0F,                       // bDescriptorType: BOS
    0x16, 0x00,                 // wTotalLength: 22 bytes
    0x02,                       // bNumDeviceCaps: 2

    // ── USB 2.0 Extension Device Capability ──
    0x07,                       // bLength
    0x10,                       // bDescriptorType: DEVICE CAPABILITY
    0x02,                       // bDevCapabilityType: USB 2.0 EXTENSION
    0x02, 0x00, 0x00, 0x00,    // bmAttributes: LPM supported

    // ── SuperSpeed USB Device Capability ──
    0x0A,                       // bLength
    0x10,                       // bDescriptorType: DEVICE CAPABILITY
    0x03,                       // bDevCapabilityType: SUPERSPEED USB
    0x00,                       // bmAttributes
    0x00, 0x0E,                 // wSpeedsSupported: Low, Full, High, SuperSpeed
    0x01,                       // bFunctionalitySupport: lowest speed = SuperSpeed
    0x0A,                       // bU1DevExitLat: 10 µs
    0x00, 0x00                  // wU2DevExitLat: 0 (U2 not supported)
};

//=============================================================================
// USB String Descriptors
//=============================================================================

// String Descriptor 0: Supported Language IDs
static const uint8_t usb_string_langid[] = {
    0x04,                       // bLength
    0x03,                       // bDescriptorType: STRING
    0x09, 0x04                  // wLANGID[0]: 0x0409 = English (United States)
};

// String Descriptor 1: Manufacturer — "Nous Research"
static const uint8_t usb_string_manufacturer[] = {
    0x1C,                       // bLength (28 bytes: 2 header + 13 chars × 2)
    0x03,                       // bDescriptorType: STRING
    'N', 0x00, 'o', 0x00, 'u', 0x00, 's', 0x00,
    ' ', 0x00, 'R', 0x00, 'e', 0x00, 's', 0x00,
    'e', 0x00, 'a', 0x00, 'r', 0x00, 'c', 0x00,
    'h', 0x00
};

// String Descriptor 2: Product — "PhotonWeave CGH Engine"
static const uint8_t usb_string_product[] = {
    0x2A,                       // bLength (42 bytes: 2 header + 20 chars × 2)
    0x03,                       // bDescriptorType: STRING
    'P', 0x00, 'h', 0x00, 'o', 0x00, 't', 0x00,
    'o', 0x00, 'n', 0x00, 'W', 0x00, 'e', 0x00,
    'a', 0x00, 'v', 0x00, 'e', 0x00, ' ', 0x00,
    'C', 0x00, 'G', 0x00, 'H', 0x00, ' ', 0x00,
    'E', 0x00, 'n', 0x00, 'g', 0x00, 'i', 0x00,
    'n', 0x00, 'e', 0x00
};

// String Descriptor 3: Serial Number — generated at runtime from EEPROM
// Format: "PW-" followed by 8 hex digits of serial number
// Maximum length: 22 bytes (2 header + 10 chars × 2)
#define USB_STRING_SERIAL_MAX_LEN  22
static uint8_t usb_string_serial[USB_STRING_SERIAL_MAX_LEN];

//=============================================================================
// USB Command Protocol (Vendor-Specific)
// Commands sent over Bulk OUT, responses over Bulk IN
//=============================================================================

// Command IDs
#define USB_CMD_GET_STATUS          0x01  // Get device status
#define USB_CMD_GET_TEMPERATURE      0x02  // Read all temperature sensors
#define USB_CMD_GET_PERFORMANCE      0x03  // Get CGH performance metrics
#define USB_CMD_SET_CGH_PARAMS       0x10  // Configure CGH parameters
#define USB_CMD_START_CGH            0x11  // Start CGH processing
#define USB_CMD_ABORT_CGH            0x12  // Abort current frame
#define USB_CMD_GET_HOLOGRAM_INFO    0x13  // Get hologram buffer info
#define USB_CMD_SET_HDMI_TIMING      0x20  // Configure HDMI output timing
#define USB_CMD_ENABLE_HDMI          0x21  // Enable/disable HDMI output
#define USB_CMD_GET_QSFP_STATUS      0x30  // Get QSFP+ module status
#define USB_CMD_FPGA_RESET           0xF0  // Soft reset FPGA
#define USB_CMD_FW_UPDATE_START      0xF1  // Begin firmware update
#define USB_CMD_FW_UPDATE_DATA       0xF2  // Firmware update data block
#define USB_CMD_FW_UPDATE_END        0xF3  // Complete firmware update
#define USB_CMD_READ_REGISTER        0xFA  // Read FPGA register
#define USB_CMD_WRITE_REGISTER       0xFB  // Write FPGA register
#define USB_CMD_ECHO                 0xFF  // Echo test command

// Response codes
#define USB_RESP_OK                  0x00
#define USB_RESP_ERROR               0x01
#define USB_RESP_BUSY                0x02
#define USB_RESP_INVALID_CMD         0x03
#define USB_RESP_INVALID_PARAM       0x04
#define USB_RESP_TIMEOUT             0x05
#define USB_RESP_NOT_READY           0x06

// Command packet structure (sent over Bulk OUT)
typedef struct __attribute__((packed)) {
    uint8_t  cmd_id;            // Command ID
    uint8_t  flags;             // Command flags
    uint16_t length;            // Payload length (bytes)
    uint32_t crc32;             // CRC32 of payload
    uint8_t  payload[1018];    // Payload data (max 1024 - 6 header)
} usb_cmd_packet_t;

// Response packet structure (sent over Bulk IN)
typedef struct __attribute__((packed)) {
    uint8_t  resp_code;         // Response code
    uint8_t  flags;             // Response flags
    uint16_t length;            // Payload length
    uint32_t crc32;             // CRC32 of payload
    uint8_t  payload[1018];    // Payload data
} usb_resp_packet_t;

// Status response payload
typedef struct __attribute__((packed)) {
    uint32_t system_state;      // System state enum
    uint32_t fpga_status;       // FPGA status register
    uint32_t cgh_status;        // CGH pipeline status
    uint32_t uptime_ms;         // System uptime
    int32_t  temperatures[4];   // Temperature sensors (millideg C)
    uint32_t power_rails[6];    // Voltage readings (mV)
    uint32_t fps_x1000;         // Current FPS
    uint32_t frame_count;       // Total frames processed
} usb_status_payload_t;

//=============================================================================
// Helper Functions
//=============================================================================

/**
 * Generate the serial number string descriptor from a 32-bit serial.
 * @param serial 32-bit serial number from board EEPROM
 */
static inline void usb_generate_serial_string(uint32_t serial)
{
    usb_string_serial[0] = USB_STRING_SERIAL_MAX_LEN;
    usb_string_serial[1] = 0x03; // STRING descriptor

    // Format: "PW-XXXXXXXX"
    const char hex_chars[] = "0123456789ABCDEF";
    usb_string_serial[2] = 'P'; usb_string_serial[3] = 0x00;
    usb_string_serial[4] = 'W'; usb_string_serial[5] = 0x00;
    usb_string_serial[6] = '-'; usb_string_serial[7] = 0x00;

    for (int i = 0; i < 8; i++) {
        uint8_t nibble = (serial >> (28 - i * 4)) & 0x0F;
        usb_string_serial[8 + i * 2] = hex_chars[nibble];
        usb_string_serial[9 + i * 2] = 0x00;
    }
}

/**
 * Compute CRC32 for USB packet payload verification.
 * Uses standard CRC32 polynomial (0xEDB88320 / 0x04C11DB7 reflected).
 * @param data Pointer to data buffer
 * @param len Length in bytes
 * @return CRC32 value
 */
static inline uint32_t usb_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

#endif // USB_DESCRIPTORS_H
