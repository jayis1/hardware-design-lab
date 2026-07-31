/*
 * ble_midi.h — BLE-MIDI 1.0 GATT service for Synthand.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SYNTHAND_BLE_MIDI_H
#define SYNTHAND_BLE_MIDI_H

#include <stdint.h>
#include "board.h"

/* MIDI status byte masks */
#define MIDI_STATUS_NOTE_OFF          0x80
#define MIDI_STATUS_NOTE_ON           0x90
#define MIDI_STATUS_CC                0xB0
#define MIDI_STATUS_PROGRAM_CHANGE    0xC0
#define MIDI_STATUS_CHANNEL_PRESSURE  0xD0
#define MIDI_STATUS_PITCH_BEND        0xE0

/* MIDI event structure */
typedef struct {
    uint8_t status;     /* status byte (0x80-0xEF) */
    uint8_t channel;    /* MIDI channel 0-15 */
    uint8_t data1;      /* first data byte (note/CC/program) */
    uint8_t data2;      /* second data byte (velocity/value) — 0 for PC/CP */
    uint32_t timestamp; /* event timestamp in ms */
} midi_event_t;

/* Initialize BLE-MIDI service and SoftDevice (network core via IPC).
 * Returns 0 on success, nonzero on error. */
int ble_midi_init(void);

/* Start/stop BLE advertising.
 * When advertising, the glove is discoverable as "Synthand-XXXX". */
void ble_midi_advertise(int enable);

/* Disconnect from the current BLE central (phone/tablet). */
void ble_midi_disconnect(void);

/* Check if a BLE central is currently connected.
 * Returns 1 if connected, 0 otherwise. */
int ble_midi_is_connected(void);

/* Flush the MIDI ring buffer, packing events into BLE-MIDI packets.
 * Called after gesture events are pushed to the ring.
 * Uses BLE-MIDI running-status compression and 13-bit timestamps. */
void ble_midi_flush_ring(const midi_event_t *ring,
                         volatile uint16_t *head,
                         volatile uint16_t *tail,
                         uint16_t ring_size,
                         uint32_t current_time_ms);

/* Push a MIDI event into the ring buffer (called from main loop).
 * This is a utility function — the ring is defined in main.c. */
void midi_ring_push(midi_event_t *ring,
                    volatile uint16_t *head,
                    volatile uint16_t *tail,
                    const midi_event_t *event,
                    uint16_t ring_size);

/* Send a single MIDI message immediately (bypasses ring buffer).
 * Used for high-priority events like program change. */
int ble_midi_send_immediate(const midi_event_t *event);

/* Pack a BLE-MIDI packet (header + timestamp + MIDI messages).
 * Returns the number of bytes written to the output buffer. */
uint16_t ble_midi_pack_packet(uint8_t *out_buf,
                              uint16_t buf_size,
                              const midi_event_t *events,
                              uint8_t num_events,
                              uint32_t base_timestamp);

/* BLE-MIDI service UUIDs (standard MIDI Association specification) */
#define BLE_MIDI_SERVICE_UUID_LSB  0x03B80E5A
#define BLE_MIDI_SERVICE_UUID_MSB  0xA84B460D9E0F8C0D84E766E0
#define BLE_MIDI_CHAR_UUID_LSB     0x7772E5DB
#define BLE_MIDI_CHAR_UUID_MSB     0x38684112A1A9F2669D106BF3

/* Synthand OSC custom service UUIDs */
#define SYNTHAND_OSC_SERVICE_UUID_LSB  0x6E400001
#define SYNTHAND_OSC_SERVICE_UUID_MSB  0xB5A3F393E0A9E50E24DCCA9E
#define SYNTHAND_OSC_TX_UUID_LSB       0x6E400002
#define SYNTHAND_OSC_TX_UUID_MSB       0xB5A3F393E0A9E50E24DCCA9E

#endif /* SYNTHAND_BLE_MIDI_H */