/*
 * lora_mesh.h — TDMA LoRa mesh MAC for Glaciator-Motes
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef LORA_MESH_H
#define LORA_MESH_H

#include <stdint.h>
#include <stdbool.h>

/* Mesh roles */
typedef enum {
    MESH_ROLE_NODE = 0,
    MESH_ROLE_GATEWAY,
    MESH_ROLE_RELAY
} mesh_role_t;

/* Mesh packet types */
typedef enum {
    PKT_BEACON      = 0x01,
    PKT_EVENT       = 0x02,
    PKT_EVENT_FRAG  = 0x03,
    PKT_ACK         = 0x04,
    PKT_HEALTH      = 0x05,
    PKT_CONFIG      = 0x06,
    PKT_RELAY       = 0x07
} pkt_type_t;

/* Mesh header (8 bytes, prepended to every LoRa payload) */
typedef struct __attribute__((packed)) {
    uint8_t  sync;         /* 0xA5 */
    uint8_t  src_node;
    uint8_t  dst_node;     /* 0xFF = broadcast */
    uint8_t  pkt_type;
    uint16_t seq;
    uint16_t frag_offset;  /* byte offset for fragmented event packets */
} mesh_hdr_t;

#define MESH_HDR_LEN  8
#define MESH_PAYLOAD_FRAG (MESH_PAYLOAD_MAX - MESH_HDR_LEN)

void  mesh_init(mesh_role_t role, uint8_t node_id, uint8_t group);
void  mesh_set_role(mesh_role_t role);
void  mesh_set_freq(uint32_t hz);
void  mesh_set_sf(uint8_t sf);
void  mesh_tick(void);                            /* called from main loop */
bool  mesh_send_event(const uint8_t *buf, uint32_t len);
bool  mesh_send_health(uint16_t vbat_mv, int16_t temp_c,
                        uint8_t solar_pct, uint32_t uptime_s);
void  mesh_on_rx(void (*cb)(const mesh_hdr_t *, const uint8_t *, uint16_t));
void  mesh_irq_handler(void);                     /* radio DIO1 ISR */
uint32_t mesh_uptime_s(void);

/* Beacon (gateway-only) */
void  mesh_send_beacon(void);
bool  mesh_receive_beacon(uint32_t *global_ts, uint8_t *slot_assigned);

/* Statistics */
typedef struct {
    uint32_t tx_ok;
    uint32_t tx_fail;
    uint32_t rx_ok;
    uint32_t rx_crc_err;
    uint32_t events_tx;
    uint32_t events_rx;
} mesh_stats_t;
const mesh_stats_t *mesh_stats(void);

#endif /* LORA_MESH_H */