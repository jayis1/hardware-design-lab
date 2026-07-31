/*
 * usb.c — USB-C MIDI and DFU interface for Synthand.
 *
 * Implements USB-MIDI class device for direct DAW connection via USB-C,
 * and CDC-ACM for firmware updates (DFU mode).
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/usb.h"

/* -------------------------------------------------------------------------
 * USB state
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static int usb_connected = 0;
static int usb_midi_enabled = 0;

/* USB-MIDI TX buffer */
static uint8_t usb_midi_tx_buf[64];
static uint16_t usb_midi_tx_len = 0;

/* USB CDC RX buffer (for DFU commands) */
static uint8_t usb_cdc_rx_buf[64];
static volatile uint16_t usb_cdc_rx_len = 0;

/* -------------------------------------------------------------------------
 * USB device descriptors (simplified — MIDI + CDC composite)
 * Author: jayis1
 * ------------------------------------------------------------------------- */

/* USB device descriptor */
static const uint8_t usb_device_desc[] = {
    18,     /* bLength */
    0x01,   /* bDescriptorType: Device */
    0x10, 0x01,  /* USB 1.1 */
    0xEF,   /* bDeviceClass: Misc */
    0x02,   /* bDeviceSubClass: Common */
    0x01,   /* bDeviceProtocol: IAD */
    64,     /* bMaxPacketSize0 */
    0x19, 0x12,  /* idVendor: jayis1 (placeholder) */
    0x50, 0x13,  /* idProduct: Synthand (placeholder) */
    0x00, 0x01,  /* bcdDevice: 1.00 */
    0x01, 0x02, 0x03,  /* string indices */
    0x01    /* bNumConfigurations */
};

/* -------------------------------------------------------------------------
 * Initialize USB-C peripheral
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int usb_init(void)
{
    /* Enable USB peripheral */
    USBD->ENABLE = 0;
    for (volatile int i = 0; i < 1000; i++);
    USBD->ENABLE = 1;

    /* Configure USB pins (P0.23 = D+, P0.24 = D-) */
    /* On nRF5340, USB pins are fixed — no GPIO config needed */

    /* Enable pull-up on D+ to signal device presence */
    /* This would be done via the USB peripheral's USBD_PULLUP register */
    /* Simplified: just mark as initialized */
    usb_midi_enabled = 1;

    return 0;
}

/* -------------------------------------------------------------------------
 * Check if USB-C is connected
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int usb_is_connected(void)
{
    /* Check if D+ pull-up is active (USB host detected) */
    /* Simplified: return the cached state */
    return usb_connected;
}

/* -------------------------------------------------------------------------
 * Send a MIDI event via USB-MIDI
 * USB-MIDI uses 4-byte packets: [cable_id | CIN] [MIDI status] [data1] [data2]
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int usb_midi_send(const midi_event_t *event)
{
    if (!usb_connected || !usb_midi_enabled)
        return -1;

    if (usb_midi_tx_len + 4 > sizeof(usb_midi_tx_buf)) {
        /* Flush the buffer first */
        /* In a full implementation, this would trigger a USB endpoint write */
        usb_midi_tx_len = 0;
    }

    usb_midi_packet_t pkt;
    pkt.cable_id = 0;  /* cable 0 */
    pkt.midi0 = event->status | (event->channel & 0x0F);
    pkt.midi1 = event->data1;
    pkt.midi2 = event->data2;

    /* Determine CIN (code index number) from status byte */
    uint8_t status_hi = event->status & 0xF0;
    switch (status_hi) {
    case MIDI_STATUS_NOTE_OFF:   pkt.cin = USB_MIDI_CIN_NOTE_OFF; break;
    case MIDI_STATUS_NOTE_ON:    pkt.cin = USB_MIDI_CIN_NOTE_ON; break;
    case MIDI_STATUS_CC:         pkt.cin = USB_MIDI_CIN_CC; break;
    case MIDI_STATUS_PROGRAM_CHANGE: pkt.cin = USB_MIDI_CIN_PROGRAM_CHG; break;
    case MIDI_STATUS_CHANNEL_PRESSURE: pkt.cin = USB_MIDI_CIN_CHAN_PRESS; break;
    case MIDI_STATUS_PITCH_BEND: pkt.cin = USB_MIDI_CIN_PITCH_BEND; break;
    default:                     pkt.cin = USB_MIDI_CIN_MISC; break;
    }

    /* Pack into TX buffer */
    usb_midi_tx_buf[usb_midi_tx_len++] = (pkt.cable_id << 4) | pkt.cin;
    usb_midi_tx_buf[usb_midi_tx_len++] = pkt.midi0;
    usb_midi_tx_buf[usb_midi_tx_len++] = pkt.midi1;
    usb_midi_tx_buf[usb_midi_tx_len++] = pkt.midi2;

    /* If buffer is full or this is a high-priority event, flush */
    if (usb_midi_tx_len >= 64) {
        /* Trigger USB endpoint write — simplified */
        usb_midi_tx_len = 0;
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * Poll USB CDC for DFU commands
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void usb_cdc_poll(void)
{
    if (usb_cdc_rx_len == 0)
        return;

    /* Check for DFU commands */
    if (usb_cdc_rx_len >= 4) {
        if (memcmp(usb_cdc_rx_buf, "DFU\n", 4) == 0) {
            usb_enter_dfu();
        }
        if (memcmp(usb_cdc_rx_buf, "VER\n", 4) == 0) {
            /* Respond with firmware version */
            /* In a full implementation, send version string via CDC */
        }
    }

    usb_cdc_rx_len = 0;
}

/* -------------------------------------------------------------------------
 * Enter DFU mode
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void usb_enter_dfu(void)
{
    /* Disable BLE and sensors */
    /* Erase DFU partition (if separate) or mark for update */
    /* Reset into bootloader — on nRF5340, this involves writing
     * a magic value to UICR and resetting. */

    /* For this implementation, we signal the network core to
     * enter DFU mode via IPC, then reset. */

    /* Write DFU magic to UICR */
    NVMC->CONFIG = NVMC_CONFIG_WEN;
    while (NVMC->READY == 0);
    *(volatile uint32_t *)(UICR_CUSTOMER_BASE) = 0xB1B2B3B4;  /* DFU magic */
    while (NVMC->READY == 0);
    NVMC->CONFIG = NVMC_CONFIG_REN;

    /* Reset the system */
    SCB_AIRCR = (0x5FA << 16) | (1U << 2);  /* SYSRESETREQ */

    while (1);  /* should not reach here */
}

/* -------------------------------------------------------------------------
 * USB interrupt handler
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void USBD_IRQHandler(void)
{
    /* Handle USB events: RESET, EPDATA, SOF */
    /* In a full implementation, this would:
     * 1. Handle USB RESET (set device address to 0, configure endpoints)
     * 2. Handle SETUP requests (enumeration: GET_DESCRIPTOR, SET_ADDRESS, etc.)
     * 3. Handle EPDATA (MIDI IN/OUT, CDC RX/TX)
     *
     * This is a simplified stub showing the structure. */
}

/*
 * Author: jayis1
 * End of usb.c
 */