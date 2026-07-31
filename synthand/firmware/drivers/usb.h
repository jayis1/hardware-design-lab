/*
 * usb.h — USB-C MIDI and DFU interface for Synthand.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SYNTHAND_USB_H
#define SYNTHAND_USB_H

#include <stdint.h>
#include "board.h"
#include "drivers/ble_midi.h"

/* Initialize USB-C peripheral (USB MIDI + CDC for DFU).
 * Returns 0 on success. */
int usb_init(void);

/* Check if USB-C is connected (D+ pull-up detected). */
int usb_is_connected(void);

/* Send a MIDI event via USB-MIDI (CDC-ACM or USB-MIDI class).
 * Returns 0 on success. */
int usb_midi_send(const midi_event_t *event);

/* Process USB CDC data (for DFU firmware update commands).
 * Called from the main loop. */
void usb_cdc_poll(void);

/* Enter DFU (Device Firmware Update) mode.
 * Erases the DFU partition and waits for host to upload new firmware. */
void usb_enter_dfu(void);

/* USB-MIDI packet structure (4 bytes per USB-MIDI event) */
typedef struct {
    uint8_t cable_id;   /* bits 3:0 = cable number, bits 7:4 = reserved */
    uint8_t cin;        /* code index number (MIDI message type) */
    uint8_t midi0;      /* status byte */
    uint8_t midi1;      /* data byte 1 */
    uint8_t midi2;      /* data byte 2 */
} __attribute__((packed)) usb_midi_packet_t;

/* USB-MIDI code index numbers */
#define USB_MIDI_CIN_MISC        0x00
#define USB_MIDI_CIN_CABLE_EVENT 0x01
#define USB_MIDI_CIN_SYSCOM_2B   0x02
#define USB_MIDI_CIN_SYSCOM_3B   0x03
#define USB_MIDI_CIN_SYSEX_START 0x04
#define USB_MIDI_CIN_SYSEX_END_3B 0x05
#define USB_MIDI_CIN_SYSEX_END_2B 0x06
#define USB_MIDI_CIN_SYSEX_END_1B 0x07
#define USB_MIDI_CIN_NOTE_OFF    0x08
#define USB_MIDI_CIN_NOTE_ON     0x09
#define USB_MIDI_CIN_POLY_KEY    0x0A
#define USB_MIDI_CIN_CC          0x0B
#define USB_MIDI_CIN_PROGRAM_CHG 0x0C
#define USB_MIDI_CIN_CHAN_PRESS  0x0D
#define USB_MIDI_CIN_PITCH_BEND  0x0E
#define USB_MIDI_CIN_SYSCOM_1B   0x0F

#endif /* SYNTHAND_USB_H */