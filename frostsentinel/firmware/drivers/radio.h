/*
 * drivers/radio.h — SX1262 LoRa mesh radio driver header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef FROSTSENTINEL_RADIO_H
#define FROSTSENTINEL_RADIO_H

#include <stdint.h>
#include "../board.h"

/* Mesh message types */
#define MESH_MSG_DATA       0x01
#define MESH_MSG_ALERT      0x02
#define MESH_MSG_BEACON     0x03
#define MESH_MSG_JOIN       0x04
#define MESH_MSG_ACK        0x05

/* The mesh packet is exactly 19 bytes payload + 4 byte AES-GCM tag = 23.
 * We use a union so the driver can access both the raw bytes (for TX/RX)
 * and the structured fields. */
typedef union {
    struct {
        uint8_t  node_id;
        uint8_t  msg_type;
        uint8_t  rfri_q8;
        uint8_t  rfri_q8_hi;
        uint8_t  twet_lo;
        uint8_t  twet_hi;
        uint8_t  sky_lo;
        uint8_t  sky_hi;
        uint8_t  delta_lo;
        uint8_t  delta_hi;
        uint8_t  leaf_wet;
        uint8_t  ae_status;
        uint8_t  flags;
        uint8_t  hops;
        uint8_t  crc_lo;
        uint8_t  crc_hi;
        uint8_t  tag[4];
    };
    uint8_t raw[23];
} mesh_packet_t;

/* Initialize the radio and mesh MAC. */
void radio_init(const uint8_t *network_key, uint8_t node_id, uint8_t slot);

/* Transmit a mesh packet (data or alert). Returns 0 on success. */
int  radio_tx(uint8_t msg_type);

/* Transmit a frost alert (high priority). */
int  radio_tx_alert(void);

/* Receive with timeout. Returns 0 on success, -1 on timeout. */
int  radio_rx(uint32_t timeout_ms, mesh_packet_t *out);

/* Low-power RX duty cycle for sleep state. */
void radio_sleep_duty(void);

/* Alert management */
void   radio_set_alert_pending(void);
uint8_t radio_get_alert_pending(void);
void   radio_clear_alert_pending(void);

/* Slot management */
uint8_t radio_get_tx_slot(void);

/* Verify a received packet's CRC and AES-GCM tag. Returns 1 if valid. */
int radio_verify_packet(const mesh_packet_t *pkt);

#endif /* FROSTSENTINEL_RADIO_H */