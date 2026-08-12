/*
 * loramesh.h — LoRa mesh networking (header)
 * Author: jayis1  Copyright (C) 2026 jayis1  License: GPL-2.0
 */
#ifndef GRAINGUARD_LORAMESH_H
#define GRAINGUARD_LORAMESH_H

#include <stdint.h>
#include <stdbool.h>

#define MESH_PACKET_SIZE  18    /* payload (excl. AES-CMAC) */
#define MESH_TX_RETRIES   3

/* Packet structure (18 bytes + 2-byte CMAC truncated) */
typedef struct __attribute__((packed)) {
    uint8_t  version;        /* 0x01 */
    uint8_t  serial;         /* probe serial (0-254) */
    uint16_t timestamp_min;  /* UTC minutes since epoch (mod 2^16) */
    uint16_t co2_ppm_x10;    /* CO2 ÷ 10 */
    uint8_t  sri;             /* 0-100 */
    uint8_t  tmax_offset;    /* max temp °C + 128 */
    uint8_t  tmin_offset;    /* min temp °C + 128 */
    uint8_t  delta_t;         /* max - min °C */
    uint8_t  rh_pct;          /* 0-100 */
    uint8_t  emc_x10;         /* EMC × 10 (e.g. 135 = 13.5%) */
    uint8_t  ae_events_per_min;
    uint8_t  insect_id;       /* 0 = none */
    uint16_t battery_mv;
    /* 16 bytes so far */
    uint8_t  hop_count;       /* incremented on each relay */
    uint8_t  reserved;        /* padding to 18 */
} mesh_packet_t;

typedef enum {
    MESH_OK = 0,
    MESH_ERR_TX,
    MESH_ERR_RX_TIMEOUT,
    MESH_ERR_CRC,
} mesh_status_t;

/* Initialize the SX1262 radio (LoRa modulation). */
int  mesh_init(uint32_t freq_hz);

/* Transmit a packet (with AES-128 CMAC appended by radio). */
int  mesh_send(const mesh_packet_t *pkt, uint8_t *aes_key);

/* Receive a packet with timeout (ms). 0 on success. */
int  mesh_recv(mesh_packet_t *out, uint8_t *aes_key, uint32_t timeout_ms);

/* Set the probe's own serial number (from EEPROM). */
void mesh_set_serial(uint8_t serial);
uint8_t mesh_get_serial(void);

/* ---- AES-128 CMAC (truncated to 2 bytes for header, full used for auth) ---- */
void aes_cmac_compute(const uint8_t *key, const uint8_t *msg, uint8_t len,
                       uint8_t mac[16]);

#endif /* GRAINGUARD_LORAMESH_H */