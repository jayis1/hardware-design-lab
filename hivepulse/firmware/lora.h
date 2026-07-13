/*
 * lora.h — LoRaWAN communication API for HivePulse
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LORA_H
#define LORA_H

#include <stdint.h>
#include <stdbool.h>
#include "ml_model.h"
#include "sensors.h"
#include "bee_counter.h"

/* LoRaWAN configuration */
#define LORA_FREQ_BAND      868000000  /* 868 MHz (EU) — also supports 915 MHz */
#define LORA_TX_POWER_DBM   20         /* +20 dBm (max for SX1262) */
#define LORA_SF             7          /* Spreading factor 7 (fast) */
#define LORA_BW             125000     /* 125 kHz bandwidth */
#define LORA_CR             4          /* Coding rate 4/5 */
#define LORA_PREAMBLE_LEN   8
#define LORA_SYNC_WORD      0x3441     /* LoRaWAN sync word */

/* OTAA join parameters */
#define LORA_JOIN_TRIES     3
#define LORA_JOIN_TIMEOUT_S 10

/* Uplink/downlink packet structures */

typedef struct {
    uint32_t timestamp;
    uint8_t  colony_state;     /* colony_state_t */
    float    confidence;
    uint16_t battery_mv;
    uint16_t solar_mv;
    float    weight_kg;
    float    ambient_temp;
    float    brood_temp;
    float    humidity;
    uint16_t co2_ppm;
    uint16_t bees_in;
    uint16_t bees_out;
    uint8_t  swarm_alert;
    uint8_t  queenless_alert;
    uint8_t  winter_mode;
} __attribute__((packed)) lora_uplink_t;

/* Downlink commands */
typedef enum {
    LORA_CMD_NONE = 0,
    LORA_CMD_INCREASE_SAMPLING = 1,
    LORA_CMD_REQUEST_AUDIO_SNAPSHOT = 2,
    LORA_CMD_SET_WINTER_MODE = 3,
    LORA_CMD_RESTART = 4,
    LORA_CMD_CALIBRATE_WEIGHT = 5,
} lora_cmd_t;

typedef struct {
    lora_cmd_t command;
    uint8_t    param;
    uint8_t    data[16];
    uint8_t    data_len;
} lora_downlink_t;

/* SX1262 radio states */
typedef enum {
    RADIO_STATE_IDLE = 0,
    RADIO_STATE_RX,
    RADIO_STATE_TX,
    RADIO_STATE_CAD,
} radio_state_t;

/* Initialize LoRa radio and LoRaWAN MAC layer */
int lora_init(void);

/* Join LoRaWAN network via OTAA */
bool lora_join(void);

/* Set LoRaWAN device class (A/B/C) */
void lora_set_class_b(void);
void lora_set_class_a(void);
void lora_set_class_c(void);

/* Send an uplink packet */
int lora_send_uplink(const lora_uplink_t *uplink);

/* Check for downlink messages (non-blocking) */
bool lora_check_downlink(lora_downlink_t *dl);

/* SX1262 low-level radio functions */
int sx1262_init(void);
int sx1262_reset(void);
int sx1262_read_reg(uint16_t addr, uint8_t *value);
int sx1262_write_reg(uint16_t addr, uint8_t value);
int sx1262_read_buffer(uint16_t addr, uint8_t *buf, int len);
int sx1262_write_buffer(uint16_t addr, const uint8_t *buf, int len);
int sx1262_set_tx(const uint8_t *data, int len, int timeout_ms);
int sx1262_set_rx(int timeout_ms);
int sx1262_get_rx(uint8_t *buf, int *len);
void sx1262_set_standby(void);
void sx1262_sleep(void);

/* SPI interface for SX1262 */
static int lora_spi_transfer(uint8_t *tx, uint8_t *rx, int len);

#endif /* LORA_H */