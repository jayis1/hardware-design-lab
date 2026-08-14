/*
 * board.h — MusselWatch board pinout, constants & data model
 *
 * Pin assignments for the MusselWatch valvometric biosensor node, an
 * STM32L432KC-based, solar-powered, LoRaWAN-capable device that monitors
 * the shell-gape activity of up to 8 freshwater bivalves using a
 * Hall-effect sensor array (magnets bonded to mussel shells; DRV5053
 * ratiometric linear Hall sensors read through a TMUX1108 analog
 * multiplexer).
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#ifndef MUSSelWATCH_BOARD_H
#define MUSSelWATCH_BOARD_H

#include <stdint.h>
#include <stdbool.h>

/* ---- Device identity ----------------------------------------------- */
#define BOARD_NAME       "MusselWatch-1.0"
#define BOARD_AUTHOR     "jayis1"
#define HW_REV           0x10u   /* v1.0 */
#define FW_VERSION       0x0100u /* 1.00 */

/* ---- Sampling & duty cycle ---------------------------------------- */
#define NUM_CHANNELS     8u      /* up to 8 mussels per node */
#define SAMPLE_HZ        4u      /* 4 Hz gape sampling per channel */
#define SAMPLE_PERIOD_MS (1000u / SAMPLE_HZ)
#define BASELINE_WINDOW_S 900u    /* 15-min rolling baseline window */
#define BASELINE_SAMPLES (BASELINE_WINDOW_S * SAMPLE_HZ)
#define UPLINK_INTERVAL_S 600u    /* normal 10-min LoRa uplink */
#define ALERT_UPLINK_HOLD_S 30u   /* minimum gap between alert uplinks */

/* ---- Battery / solar ---------------------------------------------- */
#define BATT_MV_FULL      4200u
#define BATT_MV_EMPTY     3200u
#define BATT_MV_LOW       3500u
#define SOLAR_MV_MIN      50u      /* dark threshold for solar input */
#define BUCKET_SIZE       16u      /* gape histogram buckets */

/* ---- Pin map (STM32L432KC UFQFN/UFQFPN) --------------------------- */
/* PA0  - ADC_IN5  - HALL_SIGNAL (mux output -> ADC)                 */
/* PA4  - GPIO OUT - MUX_A0                                          */
/* PA5  - GPIO OUT - MUX_A1                                          */
/* PA6  - GPIO OUT - MUX_A2                                          */
/* PA7  - GPIO OUT - MUX_EN  (active-low enable of TMUX1108)         */
/* PB0  - ADC_IN9  - VBAT_DIV (1:3 divider to ADC)                   */
/* PB1  - ADC_IN5_ - SOLAR_DIV (1:3 divider to ADC) (alt)            */
/* PA8  - GPIO OUT - SX1262 NSS (SPI CS, active-low)                 */
/* PA11 - GPIO IN  - SX1262 DIO1 (radio IRQ)                        */
/* PA12 - GPIO OUT - SX1262 RESET                                    */
/* PB3  - SPI1_SCK  (AF5)                                            */
/* PB4  - SPI1_MISO  (AF5)                                           */
/* PB5  - SPI1_MOSI  (AF5)                                           */
/* PA9  - USART1_TX (AF7) - debug console                            */
/* PA10 - USART1_RX (AF7) - debug console                            */
/* PA13 - SWDIO  (debug)                                             */
/* PA14 - SWCLK  (debug)                                             */
/* PB6  - GPIO OUT - DS18B20 1-Wire bus (open-drain)                 */
/* PB7  - GPIO IN  - CHARGER nPG (power-good indicator)              */
/* PB8  - I2C1_SCL (AF4) - PMIC + FRAM                               */
/* PB9  - I2C1_SDA (AF4) - PMIC + FRAM                               */
/* PC14 - GPIO OUT - STATUS_LED (active-low)                         */
/* PC15 - GPIO OUT - HEATER_EN (anti-condensation heater)           */

#define HALL_ADC_CH      5u
#define VBAT_ADC_CH      9u
#define SOLAR_ADC_CH     15u

#define MUX_A0_PIN       4u   /* PA4 */
#define MUX_A1_PIN       5u   /* PA5 */
#define MUX_A2_PIN       6u   /* PA6 */
#define MUX_EN_PIN       7u   /* PA7 */

#define SX_NSS_PIN       8u   /* PA8 */
#define SX_DIO1_PIN      11u  /* PA11 */
#define SX_RESET_PIN     12u  /* PA12 */

#define ONEWIRE_PIN      6u   /* PB6 */
#define CHARGER_PG_PIN   7u   /* PB7 */
#define LED_PIN          14u  /* PC14 */
#define HEATER_PIN       15u  /* PC15 */

/* ---- I2C device addresses (7-bit) --------------------------------- */
#define PMIC_I2C_ADDR    0x6Bu   /* bq25870 battery charger */
#define FRAM_I2C_ADDR    0x50u   /* FM24C64 64 Kbit FRAM event log */

/* ---- LoRa modem parameters ---------------------------------------- */
#define LORA_FREQ_HZ     868100000u  /* EU 868.1 MHz (swap for US915) */
#define LORA_SF          7u           /* spreading factor 7 */
#define LORA_BW          125000u      /* 125 kHz */
#define LORA_CR          4u           /* coding rate 4/5 */
#define LORA_TX_POWER_DBM 14u
#define LORA_PREAMBLE    8u
#define LORA_SYNC_WORD  0x1424u       /* private LoRaWAN sync */

/* ---- Per-mussel gape record --------------------------------------- */
typedef struct {
    uint8_t  channel;        /* 0..7, 0xFF = unused */
    uint16_t raw_hall;       /* raw 12-bit ADC count */
    uint16_t raw_baseline;   /* ADC reading at calibration (shell shut) */
    int16_t  gape_um;        /* inferred shell opening in micrometres */
    uint8_t  activity_score; /* 0..100 instantaneous variability metric */
    uint8_t  anomaly_flag;   /* 0 = nominal, bit0=clamp, bit1=gape-stall */
    uint32_t last_event_s;   /* seconds since last flagged event */
} channel_state_t;

/* ---- Aggregate node telemetry ------------------------------------- */
typedef struct {
    uint16_t battery_mv;
    uint16_t solar_mv;
    int16_t  water_temp_c10;   /* water temperature x10 C */
    uint8_t  charger_state;    /* 0=idle 1=charging 2=full 3=fault */
    uint8_t  flags;            /* bit0=heater_on bit1=low_batt bit2=alert */
    uint8_t  active_channels;  /* bitmask of populated channels */
    uint8_t  max_anomaly;      /* highest anomaly score this epoch */
    uint32_t uptime_s;
} telemetry_t;

/* ---- Uplink packet (binary, 24 bytes) ----------------------------- */
#define PKT_TYPE_TELEMETRY  0x01u
#define PKT_TYPE_ALERT      0x02u
#define PKT_TYPE_BASELINE   0x03u

typedef struct __attribute__((packed)) {
    uint8_t  type;
    uint8_t  node_id;          /* low byte of UID64 */
    uint8_t  seq;
    uint8_t  flags;
    uint16_t battery_mv;
    uint16_t solar_mv;
    int16_t  water_temp_c10;
    uint8_t  active_channels;
    uint8_t  max_anomaly;
    uint8_t  channel;          /* for alert packets */
    uint8_t  anomaly_flag;
    uint16_t gape_um;
    uint8_t  activity_score;
    uint8_t  reserved;
    uint32_t uptime_s;
    uint32_t crc32;
} uplink_pkt_t;

#endif /* MUSSelWATCH_BOARD_H */