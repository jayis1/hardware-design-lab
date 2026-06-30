/*
 * drivers/loramesh.h — LoRa mesh networking for SpectraPest
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef LORAMESH_H
#define LORAMESH_H

#include <stdint.h>
#include "../board.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MESH_MAX_MSG_LEN    64
#define MESH_BROADCAST_ADDR  0xFFFF
#define MESH_GATEWAY_ADDR   0x0000
#define MESH_MSG_PEST       0x01
#define MESH_MSG_BEACON     0x02
#define MESH_MSG_ROUTE      0x03
#define MESH_MSG_ACK        0x04
#define MESH_MSG_CONFIG     0x05
#define MESH_MSG_OTA        0x06

typedef struct {
    uint8_t  msg_type;
    uint8_t  source_addr;
    uint8_t  dest_addr;
    uint8_t  ttl;
    uint16_t msg_id;
    uint16_t payload_len;
    uint8_t  payload[MESH_MAX_MSG_LEN - 8];
} mesh_message_t;

typedef struct {
    uint8_t  species_id;
    uint8_t  severity;
    uint8_t  confidence_pct;  /* 0-100 */
    uint8_t  node_id;
    int32_t  lat_e7;            /* Latitude × 10^7 */
    int32_t  lon_e7;            /* Longitude × 10^7 */
    uint32_t timestamp;
} pest_alert_t;

int  lora_init(uint8_t node_addr, uint32_t freq_hz);
int  lora_send(mesh_message_t *msg);
int  lora_receive(mesh_message_t *msg, uint32_t timeout_ms);
int  lora_send_pest_alert(const pest_alert_t *alert, uint8_t dest);
void lora_sleep(void);
void lora_wakeup(void);
int  lora_get_rssi(void);

/* Mesh routing */
int  mesh_init(uint8_t self_addr);
int  mesh_broadcast(mesh_message_t *msg);
int  mesh_send(uint8_t dest, mesh_message_t *msg);
int  mesh_process_rx(mesh_message_t *msg);
int  mesh_discover_route(uint8_t dest);
void mesh_tick(void);

/* Duplicate detection */
int  mesh_is_duplicate(uint16_t msg_id, uint8_t source);
void mesh_record_msg(uint16_t msg_id, uint8_t source);

#ifdef __cplusplus
}
#endif

#endif /* LORAMESH_H */