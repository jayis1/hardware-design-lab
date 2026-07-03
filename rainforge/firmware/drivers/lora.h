/*
 * lora.h — SX1262 LoRa transceiver driver & LoRaWAN uplink
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef RAINFORGE_LORA_H
#define RAINFORGE_LORA_H

#include <stdint.h>
#include "disdrometer.h"

/* ---- LoRaWAN join state ---- */
typedef enum {
    LORA_IDLE = 0,
    LORA_JOINING,
    LORA_JOINED,
    LORA_TX_PENDING,
    LORA_TX_DONE,
    LORA_RX_DOWNLINK,
    LORA_ERROR
} lora_state_t;

/* ---- Uplink payload (27 bytes) ----
 * The payload is packed for LoRaWAN's max 51-byte limit:
 *
 *  Byte 0:     device status byte (state + flags)
 *  Bytes 1-2:  rainfall rate × 100 (mm/hr, uint16)
 *  Bytes 3-6:  reflectivity Z (uint32, mm^6/m^3)
 *  Bytes 7-8:  LWC × 100 (uint16, g/m^3)
 *  Bytes 9-10: mean diameter × 256 (uint16, mm)
 *  Bytes 11-12: median diameter × 256 (uint16, mm)
 *  Bytes 13-14: Z-R coefficient a × 10 (uint16)
 *  Byte 15:    Z-R coefficient b × 10 (uint8)
 *  Bytes 16-17: total droplets (uint16, capped)
 *  Bytes 18-19: positive charge count (uint16)
 *  Bytes 20-21: negative charge count (uint16)
 *  Bytes 22-23: average charge × 16 (int16, pC)
 *  Bytes 24-25: supercap voltage × 1000 (uint16, V)
 *  Byte 26:    temperature + 100 (int8, °C)
 */
#define LORA_PAYLOAD_LEN 27

/* ---- API ---- */
void  lora_init(void);
void  lora_reset(void);
int   lora_join(const uint8_t *app_eui, const uint8_t *app_key,
                const uint8_t *dev_eui);
int   lora_tx(const uint8_t *payload, uint8_t len, uint8_t port);
int   lora_build_payload(const dsd_t *dsd, float scap_v, float temp_c,
                         uint8_t *out);
lora_state_t lora_get_state(void);

/* SX1262 low-level (exposed for diagnostics) */
int   lora_read_status(uint8_t *status);
int   lora_check_irq(uint16_t *irq_flags);
void  lora_clear_irq(uint16_t mask);
void  lora_sleep(void);
void  lora_wake(void);

/* Downlink callback (set by main) */
typedef void (*lora_downlink_cb)(uint8_t port, const uint8_t *data, uint8_t len);
void  lora_set_downlink_cb(lora_downlink_cb cb);

#endif /* RAINFORGE_LORA_H */