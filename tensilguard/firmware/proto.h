/*
 * proto.h — TensilGuard uplink protocol definitions
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Wire format for LoRa uplink and mesh-relay packets. All multi-byte fields
 * are little-endian on the wire. The gateway decodes these structs and
 * forwards to the TensilGuard Field app over its REST/WebSocket API.
 */
#ifndef TENSILGUARD_PROTO_H
#define TENSILGUARD_PROTO_H

#include <stdint.h>
#include "tensilguard.h"

#define UL_HDR_MAGIC            0x54   /* 'T' */
#define UL_VER                    1

/* Header — fixed portion, 16 bytes on the wire */
typedef struct {
    uint8_t  magic;            /* UL_HDR_MAGIC */
    uint8_t  ver;               /* UL_VER */
    uint8_t  node_id[6];       /* EUI-48 lower half, mesh source */
    uint8_t  hops;              /* mesh hop count */
    uint8_t  rssi_db;           /* signed, offset +100 */
    uint16_t crc16;             /* over the full payload */
    uint32_t seq;
    uint32_t unix_time;         /* UTC seconds at measurement */
} ul_header_t;                  /* 18 bytes (packed) */

/* AE event sub-record — appended when ae_count > 0 */
typedef struct {
    uint32_t unix_time;
    uint16_t peak_uv_q1;        /* µV × 10 */
    uint16_t rise_us;
    uint16_t dur_us;
    uint16_t centroid_khz;      /* kHz */
    uint8_t  hi_lo_ratio_q4;   /* ×16 */
    uint8_t  score;
    uint8_t  classification;    /* AE_NOISE..AE_BREAK */
    uint8_t  waveform[24];     /* 8:1 downsampled summary */
} ae_record_t;                   /* 38 bytes */

/* Mesh relay header — prepended by each relayer */
typedef struct {
    uint8_t  relay_id[6];
    uint8_t  hop_seq;          /* bitmap of relayers so far */
    int8_t   snr_db;
} relay_header_t;               /* 8 bytes */

#define UL_MAX_PAYLOAD         (TG_UPLINK_MAX_BYTES - sizeof(ul_header_t))

/* Flag helpers */
static inline uint8_t ul_set_urgent(uint8_t flags, bool urgent) {
    return urgent ? (flags | TG_FLAG_URGENT) : (flags & ~TG_FLAG_URGENT);
}

static inline uint8_t ul_set_dt_alarm(uint8_t flags, bool dt) {
    return dt ? (flags | TG_FLAG_DT_ALARM) : (flags & ~TG_FLAG_DT_ALARM);
}

static inline uint8_t ul_set_ae_alarm(uint8_t flags, bool ae) {
    return ae ? (flags | TG_FLAG_AE_ALARM) : (flags & ~TG_FLAG_AE_ALARM);
}

/* Downlink commands (gateway -> node) */
#define DL_CMD_SET_INTERVAL     0x01
#define DL_CMD_REQUEST_SWEEP    0x02
#define DL_CMD_CAL_WRITE        0x03
#define DL_CMD_CAL_READ         0x04
#define DL_CMD_REBOOT            0x05
#define DL_CMD_AE_DUMP           0x06
#define DL_CMD_SET_AE_THRESH     0x07

typedef struct {
    uint8_t  cmd;
    uint8_t  arg[7];
} dl_command_t;

/* CRC16-CCITT — implementation in lora.c */
uint16_t crc16_ccitt(const uint8_t *data, size_t len);

#endif /* TENSILGUARD_PROTO_H */