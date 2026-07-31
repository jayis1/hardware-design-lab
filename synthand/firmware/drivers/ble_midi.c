/*
 * ble_midi.c — BLE-MIDI 1.0 GATT service implementation for Synthand.
 *
 * Implements the standard BLE-MIDI service for communication with iOS CoreMIDI,
 * Android MIDI API, and desktop DAWs. Uses the nRF5340 network core (SoftDevice
 * S140) via IPC for BLE protocol processing.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/ble_midi.h"

/* -------------------------------------------------------------------------
 * BLE state
 * Author: jayis1
 * ------------------------------------------------------------------------- */

static int ble_connected = 0;
static int ble_advertising = 0;

/* BLE-MIDI TX packet buffer */
static uint8_t midi_tx_packet[BLE_MIDI_MTU];
static uint16_t midi_tx_len = 0;

/* BLE connection handle (received from network core via IPC) */
static uint16_t ble_conn_handle = 0xFFFF;

/* BLE-MIDI timestamp epoch (set at connection) */
static uint32_t midi_timestamp_epoch = 0;

/* -------------------------------------------------------------------------
 * IPC communication with network core (SoftDevice)
 * The network core runs the BLE stack and signals events via IPC.
 * Author: jayis1
 * ------------------------------------------------------------------------- */

/* IPC message types */
#define IPC_MSG_ADV_START       0x01
#define IPC_MSG_ADV_STOP        0x02
#define IPC_MSG_CONN_ESTABLISHED 0x03
#define IPC_MSG_CONN_DISCONNECTED 0x04
#define IPC_MSG_MIDI_TX         0x05
#define IPC_MSG_MIDI_RX         0x06
#define IPC_MSG_OSC_TX          0x07

/* IPC shared memory (mailbox at fixed RAM address) */
typedef struct {
    uint32_t msg_type;
    uint32_t length;
    uint8_t  data[180];
} ipc_message_t;

#define IPC_MAILBOX ((volatile ipc_message_t *)0x20080000)

/* Send a message to the network core via IPC */
static void ipc_send(uint32_t msg_type, const uint8_t *data, uint32_t len)
{
    IPC_MAILBOX->msg_type = msg_type;
    IPC_MAILBOX->length = len;
    if (data && len > 0 && len <= sizeof(IPC_MAILBOX->data)) {
        memcpy((void *)IPC_MAILBOX->data, data, len);
    }
    /* Trigger IPC interrupt on network core */
    IPC->TASKS_SEND[0] = 1;
}

/* -------------------------------------------------------------------------
 * IPC interrupt handler (receiving from network core)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void IPC_IRQHandler(void)
{
    if (IPC->EVENTS_RECEIVE[0]) {
        IPC->EVENTS_RECEIVE[0] = 0;

        uint32_t msg = IPC_MAILBOX->msg_type;
        switch (msg) {
        case IPC_MSG_CONN_ESTABLISHED:
            ble_connected = 1;
            ble_advertising = 0;
            ble_conn_handle = 0x0001;  /* simplified */
            midi_timestamp_epoch = 0;  /* reset epoch */
            break;

        case IPC_MSG_CONN_DISCONNECTED:
            ble_connected = 0;
            ble_conn_handle = 0xFFFF;
            /* Re-advertise after disconnection */
            if (ble_advertising) {
                ipc_send(IPC_MSG_ADV_START, NULL, 0);
            }
            break;

        case IPC_MSG_MIDI_RX:
            /* MIDI data received from central (e.g., clock, CC from app) */
            /* Process incoming MIDI if needed (e.g., sync, patch changes) */
            break;

        default:
            break;
        }
    }
}

/* -------------------------------------------------------------------------
 * Initialize BLE-MIDI service
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int ble_midi_init(void)
{
    /* Enable IPC interrupt */
    IPC->INTENSET = (1U << 0);
    NVIC_ICPR0 = (1U << IRQ_IPC);
    NVIC_ISER0 = (1U << IRQ_IPC);

    /* The network core boots the SoftDevice automatically.
     * We just need to signal that the MIDI service is ready. */
    ble_connected = 0;
    ble_advertising = 0;

    return 0;
}

/* -------------------------------------------------------------------------
 * Start/stop BLE advertising
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void ble_midi_advertise(int enable)
{
    if (enable && !ble_advertising) {
        ipc_send(IPC_MSG_ADV_START, NULL, 0);
        ble_advertising = 1;
    } else if (!enable && ble_advertising) {
        ipc_send(IPC_MSG_ADV_STOP, NULL, 0);
        ble_advertising = 0;
    }
}

/* -------------------------------------------------------------------------
 * Disconnect from current BLE central
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void ble_midi_disconnect(void)
{
    if (ble_connected) {
        /* Signal network core to disconnect */
        ipc_send(IPC_MSG_CONN_DISCONNECTED, NULL, 0);
        ble_connected = 0;
    }
}

/* -------------------------------------------------------------------------
 * Check if BLE central is connected
 * ------------------------------------------------------------------------- */
int ble_midi_is_connected(void)
{
    return ble_connected;
}

/* -------------------------------------------------------------------------
 * Push a MIDI event into the ring buffer
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void midi_ring_push(midi_event_t *ring,
                     volatile uint16_t *head,
                     volatile uint16_t *tail,
                     const midi_event_t *event,
                     uint16_t ring_size)
{
    uint16_t next = (*head + 1) % ring_size;
    if (next == *tail) {
        /* Buffer full — drop oldest event */
        *tail = (*tail + 1) % ring_size;
    }
    ring[*head] = *event;
    *head = next;
}

/* -------------------------------------------------------------------------
 * Pack a BLE-MIDI packet
 * BLE-MIDI format: [header byte] [timestamp hi+lo] [MIDI msg] ...
 * Author: jayis1
 * ------------------------------------------------------------------------- */
uint16_t ble_midi_pack_packet(uint8_t *out_buf,
                               uint16_t buf_size,
                               const midi_event_t *events,
                               uint8_t num_events,
                               uint32_t base_timestamp)
{
    if (buf_size < 3 || num_events == 0)
        return 0;

    uint16_t pos = 0;

    /* BLE-MIDI header byte: top 7 bits = timestamp hi (bits 6:0 of msb)
     * bit 0 = MSB high bit (always 1 for header) */
    uint16_t ts = (uint16_t)(base_timestamp & 0x1FFF);  /* 13-bit timestamp */
    uint8_t ts_hi = (uint8_t)((ts >> 7) & 0x3F);
    uint8_t header = 0x80 | ts_hi;  /* bit 7 = 1 (header marker) */
    out_buf[pos++] = header;

    /* Running status for compression */
    uint8_t running_status = 0;

    for (int i = 0; i < num_events && pos < buf_size - 4; i++) {
        const midi_event_t *evt = &events[i];

        /* Timestamp low byte (7 bits + bit 7 = 1) */
        uint8_t ts_lo = (uint8_t)(ts & 0x7F);
        out_buf[pos++] = 0x80 | ts_lo;

        /* Status byte (with channel) */
        uint8_t status = evt->status | (evt->channel & 0x0F);

        if (status != running_status) {
            out_buf[pos++] = status;
            running_status = status;
        }

        /* Data bytes */
        out_buf[pos++] = evt->data1 & 0x7F;
        if (status != MIDI_STATUS_PROGRAM_CHANGE &&
            status != MIDI_STATUS_CHANNEL_PRESSURE) {
            out_buf[pos++] = evt->data2 & 0x7F;
        }
    }

    return pos;
}

/* -------------------------------------------------------------------------
 * Flush the MIDI ring buffer — pack events and send via BLE
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void ble_midi_flush_ring(const midi_event_t *ring,
                          volatile uint16_t *head,
                          volatile uint16_t *tail,
                          uint16_t ring_size,
                          uint32_t current_time_ms)
{
    if (!ble_connected)
        return;

    /* Count events in the ring */
    uint16_t count = 0;
    uint16_t t = *tail;
    while (t != *head) {
        count++;
        t = (t + 1) % ring_size;
    }

    if (count == 0)
        return;

    /* Collect events into a local array (max 12 per packet) */
    midi_event_t events[12];
    uint8_t num_to_pack = (count > 12) ? 12 : (uint8_t)count;

    for (uint8_t i = 0; i < num_to_pack; i++) {
        events[i] = ring[*tail];
        *tail = (*tail + 1) % ring_size;
    }

    /* Pack into BLE-MIDI packet */
    uint16_t pkt_len = ble_midi_pack_packet(midi_tx_packet,
                                             BLE_MIDI_MTU,
                                             events, num_to_pack,
                                             current_time_ms);

    /* Send via IPC to network core for BLE transmission */
    if (pkt_len > 0) {
        ipc_send(IPC_MSG_MIDI_TX, midi_tx_packet, pkt_len);
    }

    /* If more events remain, they'll be sent on the next flush */
}

/* -------------------------------------------------------------------------
 * Send a single MIDI message immediately
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int ble_midi_send_immediate(const midi_event_t *event)
{
    if (!ble_connected)
        return -1;

    uint16_t pkt_len = ble_midi_pack_packet(midi_tx_packet,
                                             BLE_MIDI_MTU,
                                             event, 1,
                                             0);
    if (pkt_len > 0) {
        ipc_send(IPC_MSG_MIDI_TX, midi_tx_packet, pkt_len);
        return 0;
    }
    return -2;
}

/*
 * Author: jayis1
 * End of ble_midi.c
 */