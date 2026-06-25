/*
 * lorawan.h — LoRaWAN telemetry public interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */
#ifndef SOUNDLOOM_LORAWAN_H
#define SOUNDLOOM_LORAWAN_H

#include <stdint.h>
#include "board.h"

#define LORA_OK              0
#define LORA_ERR_NOT_JOINED  (-1)
#define LORA_ERR_TIMEOUT     (-2)
#define LORA_ERR_INVALID     (-3)

/* Downlink command opcodes */
#define LORA_DL_SET_CADENCE     0x01
#define LORA_DL_SET_THRESHOLD  0x02
#define LORA_DL_SET_SENSITIVITY 0x03
#define LORA_DL_REBOOT          0x04
#define LORA_DL_MODEL_UPDATE    0x05

typedef struct {
    uint8_t  svi;                      /* Soil Vitality Index 0–100 */
    uint16_t event_counts[CLS_NUM_CLASSES]; /* per-class event counts */
    float    moisture[2];              /* volumetric water content */
    float    ec[2];                   /* electrical conductivity µS/cm */
    float    depth_temp[4];           /* °C at 4 depths */
    uint16_t co2_ppm;                 /* soil CO2 */
    uint16_t battery_mv;             /* battery voltage */
    uint16_t flags;                   /* alert flags */
} lora_uplink_t;

typedef struct {
    uint8_t  joined;
    uint8_t  datarate;
    int8_t   tx_power;
    uint32_t frequency;
    uint32_t cadence_ms;
    uint8_t  cls_threshold;
    uint16_t sensitivity;
    uint8_t  ota_pending;
} lora_state_t;

int      lora_init(void);
int      lora_build_uplink(const lora_uplink_t *data, uint8_t *out, uint8_t *len);
int      lora_send_uplink(const lora_uplink_t *data);
int      lora_handle_downlink(const uint8_t *data, uint8_t len);
uint32_t lora_get_tx_count(void);
uint32_t lora_get_rx_count(void);
int8_t   lora_get_last_rssi(void);
int8_t   lora_get_last_snr(void);

#endif /* SOUNDLOOM_LORAWAN_H */