/* ============================================
 * usb_descriptors.h — WaveForge USB Descriptors
 * USB 2.0 Full-Speed: Audio Class 2.0 + MIDI Class
 * ============================================ */

#ifndef WAVEFORGE_USB_DESCRIPTORS_H
#define WAVEFORGE_USB_DESCRIPTORS_H

#include <stdint.h>

/* ---- Device Descriptor ---- */
static const uint8_t usb_device_descriptor[] = {
    18,                         /* bLength */
    0x01,                       /* bDescriptorType: Device */
    0x00, 0x02,                 /* bcdUSB: 2.00 */
    0x00,                       /* bDeviceClass: Composite */
    0x00,                       /* bDeviceSubClass */
    0x00,                       /* bDeviceProtocol */
    64,                         /* bMaxPacketSize0: 64 bytes */
    0x83, 0x04,                 /* idVendor: 0x0483 (STMicroelectronics) */
    0x57, 0x50,                 /* idProduct: 0x5057 (WaveForge) */
    0x00, 0x01,                 /* bcdDevice: 1.00 */
    0x01,                       /* iManufacturer: 1 */
    0x02,                       /* iProduct: 2 */
    0x03,                       /* iSerialNumber: 3 */
    0x01,                       /* bNumConfigurations: 1 */
};

/* ---- Configuration Descriptor (Audio + MIDI) ---- */
static const uint8_t usb_config_descriptor[] = {
    /* Configuration Descriptor */
    9,                          /* bLength */
    0x02,                       /* bDescriptorType: Configuration */
    0x89, 0x00,                 /* wTotalLength: 137 bytes */
    0x03,                       /* bNumInterfaces: 3 */
    0x01,                       /* bConfigurationValue: 1 */
    0x00,                       /* iConfiguration: 0 */
    0x80,                       /* bmAttributes: Bus powered */
    250,                        /* bMaxPower: 500 mA */

    /* ---- Interface Association Descriptor: Audio Control ---- */
    8,                          /* bLength */
    0x0B,                       /* bDescriptorType: IAD */
    0x00,                       /* bFirstInterface: 0 */
    0x03,                       /* bInterfaceCount: 3 */
    0x01,                       /* bFunctionClass: Audio */
    0x00,                       /* bFunctionSubClass */
    0x20,                       /* bFunctionProtocol: AF_VERSION_02_00 */
    0x00,                       /* iFunction */

    /* ---- Interface 0: Audio Control ---- */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: Interface */
    0x00,                       /* bInterfaceNumber: 0 */
    0x00,                       /* bAlternateSetting: 0 */
    0x00,                       /* bNumEndpoints: 0 */
    0x01,                       /* bInterfaceClass: Audio */
    0x01,                       /* bInterfaceSubClass: Audio Control */
    0x20,                       /* bInterfaceProtocol: AF_VERSION_02_00 */
    0x00,                       /* iInterface */

    /* Audio Control Class-Specific AC Interface Descriptor */
    9,                          /* bLength */
    0x24,                       /* bDescriptorType: CS_INTERFACE */
    0x01,                       /* bDescriptorSubtype: HEADER */
    0x00, 0x02,                 /* bcdADC: 2.00 */
    0x09,                       /* bmControls: latency control */
    0x01, 0x00,                 /* wTotalLength: 9 bytes */
    0x01,                       /* bInCollection: 1 */
    0x01,                       /* baInterfaceNr: 1 (Audio Streaming) */

    /* ---- Interface 1: Audio Streaming (Output) ---- */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: Interface */
    0x01,                       /* bInterfaceNumber: 1 */
    0x00,                       /* bAlternateSetting: 0 */
    0x02,                       /* bNumEndpoints: 2 (ISO OUT + Synch) */
    0x01,                       /* bInterfaceClass: Audio */
    0x02,                       /* bInterfaceSubClass: Audio Streaming */
    0x20,                       /* bInterfaceProtocol: AF_VERSION_02_00 */
    0x00,                       /* iInterface */

    /* Audio Streaming Class-Specific AS Interface Descriptor */
    16,                         /* bLength */
    0x24,                       /* bDescriptorType: CS_INTERFACE */
    0x01,                       /* bDescriptorSubtype: AS_GENERAL */
    0x01,                       /* bTerminalLink: 1 (output) */
    0x00,                       /* bmControls: none */
    0x01,                       /* bFormatType: Type I (PCM) */
    0x01, 0x00,                 /* bmFormats: PCM */
    0x03,                       /* bNrChannels: 2 (stereo) */
    0x00, 0x00, 0x00,           /* bmChannelConfig: FL, FR */
    0x00,                       /* iChannelNames */

    /* Type I Format Type Descriptor (24-bit, 48 kHz) */
    6,                          /* bLength */
    0x24,                       /* bDescriptorType: CS_INTERFACE */
    0x02,                       /* bDescriptorSubtype: FORMAT_TYPE */
    0x01,                       /* bFormatType: Type I */
    0x03,                       /* bSubslotSize: 3 bytes (24-bit) */
    0x18,                       /* bBitResolution: 24 */
    0x01,                       /* bSamFreqType: 1 frequency */
    0x80, 0xBB, 0x00,           /* tSamFreq: 48000 Hz (0x00BB80) */

    /* ISO OUT Endpoint Descriptor */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: Endpoint */
    0x01,                       /* bEndpointAddress: OUT 1 */
    0x05,                       /* bmAttributes: ISOchronous, Async */
    0xC0, 0x00,                 /* wMaxPacketSize: 192 bytes (48 kHz × 2ch × 3bytes × 1packet) */
    0x01,                       /* bInterval: 1 ms */

    /* CS AS Endpoint Descriptor */
    8,                          /* bLength */
    0x25,                       /* bDescriptorType: CS_ENDPOINT */
    0x01,                       /* bDescriptorSubtype: EP_GENERAL */
    0x01,                       /* bmAttributes: sampling frequency control */
    0x00,                       /* bmControls: none */
    0x00,                       /* bLockDelay: 0 */
    0x00, 0x00,                 /* wLockDelay: 0 */

    /* ---- Interface 2: MIDI Streaming ---- */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: Interface */
    0x02,                       /* bInterfaceNumber: 2 */
    0x00,                       /* bAlternateSetting: 0 */
    0x02,                       /* bNumEndpoints: 2 (IN + OUT) */
    0x01,                       /* bInterfaceClass: Audio */
    0x03,                       /* bInterfaceSubClass: MIDI Streaming */
    0x00,                       /* bInterfaceProtocol */
    0x00,                       /* iInterface */

    /* MIDI Streaming Class-Specific MS Interface Descriptor */
    7,                          /* bLength */
    0x24,                       /* bDescriptorType: CS_INTERFACE */
    0x01,                       /* bDescriptorSubtype: MS_HEADER */
    0x00, 0x01,                 /* bcdMSC: 1.00 */
    0x01, 0x00,                 /* wTotalLength: 1 byte (placeholder) */

    /* MIDI IN Jack (Embedded) */
    6,                          /* bLength */
    0x24,                       /* bDescriptorType: CS_INTERFACE */
    0x02,                       /* bDescriptorSubtype: MIDI_IN_JACK */
    0x01,                       /* bJackType: Embedded */
    0x01,                       /* bJackID: 1 */
    0x00,                       /* iJack */

    /* MIDI OUT Jack (Embedded) */
    9,                          /* bLength */
    0x24,                       /* bDescriptorType: CS_INTERFACE */
    0x03,                       /* bDescriptorSubtype: MIDI_OUT_JACK */
    0x01,                       /* bJackType: Embedded */
    0x02,                       /* bJackID: 2 */
    0x01,                       /* bNrInputPins: 1 */
    0x01, 0x01,                 /* baSourceID, baSourcePin */
    0x00,                       /* iJack */

    /* MIDI Bulk IN Endpoint */
    9,                          /* bLength */
    0x05,                       /* bDescriptorType: Endpoint */
    0x82,                       /* bEndpointAddress: IN 2 */
    0x02,                       /* bmAttributes: Bulk */
    0x40, 0x00,                 /* wMaxPacketSize: 64 bytes */
    0x00,                       /* bInterval: N/A */

    /* CS MIDI Bulk IN Endpoint */
    5,                          /* bLength */
    0x25,                       /* bDescriptorType: CS_ENDPOINT */
    0x01,                       /* bDescriptorSubtype: MS_GENERAL */
    0x01,                       /* bNumEmbMIDIJack: 1 */
    0x01,                       /* baAssocJackID: 1 */

    /* MIDI Bulk OUT Endpoint */
    9,                          /* bLength */
    0x05,                       /* bDescriptorType: Endpoint */
    0x02,                       /* bEndpointAddress: OUT 2 */
    0x02,                       /* bmAttributes: Bulk */
    0x40, 0x00,                 /* wMaxPacketSize: 64 bytes */
    0x00,                       /* bInterval: N/A */

    /* CS MIDI Bulk OUT Endpoint */
    5,                          /* bLength */
    0x25,                       /* bDescriptorType: CS_ENDPOINT */
    0x01,                       /* bDescriptorSubtype: MS_GENERAL */
    0x01,                       /* bNumEmbMIDIJack: 1 */
    0x02,                       /* baAssocJackID: 2 */
};

/* ---- String Descriptors ---- */
static const uint8_t usb_string_lang_id[] = {
    4, 0x03, 0x09, 0x04          /* LangID: English (0x0409) */
};

static const uint8_t usb_string_manufacturer[] = {
    26, 0x03,                     /* bLength, bDescriptorType */
    'W', 0x00, 'a', 0x00, 'v', 0x00, 'e', 0x00,
    'F', 0x00, 'o', 0x00, 'r', 0x00, 'g', 0x00,
    'e', 0x00, ' ', 0x00, 'L', 0x00, 'a', 0x00,
    'b', 0x00, 's', 0x00
};

static const uint8_t usb_string_product[] = {
    30, 0x03,                     /* bLength, bDescriptorType */
    'W', 0x00, 'a', 0x00, 'v', 0x00, 'e', 0x00,
    'F', 0x00, 'o', 0x00, 'r', 0x00, 'g', 0x00,
    'e', 0x00, ' ', 0x00, 'S', 0x00, 'y', 0x00,
    'n', 0x00, 't', 0x00, 'h', 0x00
};

static const uint8_t usb_string_serial[] = {
    18, 0x03,                     /* bLength, bDescriptorType */
    'W', 0x00, 'F', 0x00, '0', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '1', 0x00, 'A', 0x00
};

#endif /* WAVEFORGE_USB_DESCRIPTORS_H */