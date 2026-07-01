/*
 * proto.h — Soilchord LoRa wire protocol
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Compact binary protocol used over the SX1262 link. All multi-byte fields
 * are little-endian. Uplink frames are ≤ 51 bytes to be friendly to EU868
 * duty-cycle limits and TTN's 64-byte fair-use cap with margin.
 */
#ifndef SOILCHORD_PROTO_H
#define SOILCHORD_PROTO_H

#include <stdint.h>
#include "board.h"

/* Uplink frame (per-measurement) — 1 + 8 + 6*(4+1+1) = 45 bytes max */
#define UL_HDR_MAGIC            0x5C        /* 'S' for Soilchord */
#define UL_VER                  1

#define UL_FLAG_DEAD_PLUCK      (1U << 0)
#define UL_FLAG_SUSPECT         (1U << 1)
#define UL_FLAG_ALERT           (1U << 2)
#define UL_FLAG_URGENT          (1U << 3)

#pragma pack(push, 1)
typedef struct {
    uint8_t  magic;            /* UL_HDR_MAGIC */
    uint8_t  ver;              /* UL_VER */
    uint32_t seq;              /* sequence number */
    uint32_t unix_time;        /* epoch seconds */
    int16_t  battery_mv;
    int16_t  solar_mv;
    uint8_t  interval_s;
    uint8_t  reset_cause;
    uint8_t  urgent;
    uint8_t  nchords;          /* always NCHORDS in v1 */
    /* followed by nchords × chord_payload_t */
} ul_header_t;

typedef struct {
    uint16_t f0_q8;            /* f0 × 256  (Hz × 256) — 16..4000 Hz range */
    uint8_t  q_freq_u8;       /* q_freq × 4  (1..255 → 0.25..63.75)        */
    uint8_t  q_time_u8;       /* q_time × 4                                 */
    uint8_t  temp_c_u8;       /* temp + 64  (signed offset, °C)             */
    uint8_t  flags;            /* per-chord flags                            */
} chord_payload_t;
#pragma pack(pop)

/* Downlink frame */
#define DL_MAGIC                0xC5
#define DL_VER                  1
#pragma pack(push, 1)
typedef struct {
    uint8_t  magic;            /* DL_MAGIC */
    uint8_t  ver;
    uint8_t  cmd;              /* dl_cmd_t */
    uint32_t arg;
} dl_header_t;
#pragma pack(pop)

#endif /* SOILCHORD_PROTO_H */