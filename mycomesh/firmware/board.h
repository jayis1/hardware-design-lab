/*
 * board.h — Pin assignments and hardware constants for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * This header defines all physical pin assignments, peripheral mappings,
 * timing constants, and hardware configuration for the MycoMesh sensor node
 * built around the STM32L4R9ZI microcontroller.
 */

#ifndef MYCOMESH_BOARD_H
#define MYCOMESH_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ===================================================================== */
/*  MCU identity                                                          */
/* ===================================================================== */

#define BOARD_NAME          "MycoMesh"
#define BOARD_VERSION       "1.0"
#define BOARD_AUTHOR        "jayis1"
#define BOARD_SERIAL_LEN    16

/* ===================================================================== */
/*  Clock configuration                                                   */
/* ===================================================================== */

#define HSI_FREQ_HZ         16000000UL   /* Internal HSI oscillator      */
#define HSE_FREQ_HZ         8000000UL    /* External 8 MHz crystal        */
#define LSE_FREQ_HZ         32768UL      /* External 32.768 kHz RTC       */
#define SYSCLK_FREQ_HZ      120000000UL  /* PLL: 8 MHz / 1 * 30 / 2 = 120 */
#define APB1_FREQ_HZ        120000000UL  /* APB1 = SYSCLK / 1             */
#define APB2_FREQ_HZ        120000000UL  /* APB2 = SYSCLK / 1             */
#define FLASH_LATENCY_WS    4            /* 4 wait states for 120 MHz     */

/* ===================================================================== */
/*  Pin assignments — STM32L4R9ZI (UFBGA-169 / LQFP-208)                 */
/* ===================================================================== */

/* --- SPI1: ADS1298 dual biomedical ADC --- */
#define SPI1_SCK_PIN        5        /* PA5  */
#define SPI1_MISO_PIN       6        /* PA6  */
#define SPI1_MOSI_PIN       7        /* PA7  */
#define ADS1298_CS0_PORT    GPIOB
#define ADS1298_CS0_PIN     0        /* PB0  — CS for ADS1298 #0       */
#define ADS1298_CS1_PORT    GPIOB
#define ADS1298_CS1_PIN     1        /* PB1  — CS for ADS1298 #1       */
#define ADS1298_DRDY_PORT   GPIOC
#define ADS1298_DRDY_PIN    4        /* PC4  — DRDY (both ADCs tied)   */
#define ADS1298_START_PORT  GPIOC
#define ADS1298_START_PIN   5        /* PC5  — START (both ADCs tied)  */
#define ADS1298_RESET_PORT  GPIOC
#define ADS1298_RESET_PIN   6        /* PC6  — RESET (both ADCs tied)  */
#define ADS1298_PWDN_PORT   GPIOC
#define ADS1298_PWDN_PIN    7        /* PC7  — PWDN (both ADCs tied)   */
#define ADS_CLK_PIN         8        /* PC8  — ADS1298 external clock  */

/* --- SPI2: microSD card --- */
#define SD_CS_PORT          GPIOB
#define SD_CS_PIN           12       /* PB12 */
#define SD_SCK_PIN          13       /* PB13 */
#define SD_MISO_PIN         14       /* PB14 */
#define SD_MOSI_PIN         15       /* PB15 */
#define SD_DETECT_PORT      GPIOC
#define SD_DETECT_PIN       9        /* PC9  — card detect (active low) */

/* --- SPI3: SX1262 LoRa transceiver --- */
#define LORA_CS_PORT        GPIOA
#define LORA_CS_PIN         4        /* PA4  */
#define LORA_SCK_PIN        10       /* PC10 */
#define LORA_MISO_PIN       11       /* PC11 */
#define LORA_MOSI_PIN       12       /* PC12 */
#define LORA_BUSY_PORT      GPIOA
#define LORA_BUSY_PIN       15       /* PA15 */
#define LORA_DIO1_PORT      GPIOB
#define LORA_DIO1_PIN       2        /* PB2  */
#define LORA_NRST_PORT      GPIOB
#define LORA_NRST_PIN       3        /* PB3  */
#define LORA_ANT_SW_PORT    GPIOC
#define LORA_ANT_SW_PIN     13       /* PC13 — RF switch control       */

/* --- I2C1: SCD41 CO₂ sensor --- */
#define I2C1_SCL_PIN        6        /* PB6  */
#define I2C1_SDA_PIN        7        /* PB7  */
#define SCD41_I2C_ADDR      0x62

/* --- UART3: SDI-12 soil moisture (SMT100) --- */
#define SDI12_TX_PIN        10       /* PC10 (shared via mux, alt func) */
#define SDI12_RX_PIN        11       /* PC11 */
#define SDI12_BAUD          1200

/* --- UART4: Debug CLI --- */
#define DEBUG_TX_PIN        0        /* PA0  */
#define DEBUG_RX_PIN        1        /* PA1  */
#define DEBUG_BAUD          115200

/* --- GPIO: DS18B20 1-Wire temperature --- */
#define DS18B20_PIN         0        /* PC0  */
#define DS18B20_PORT        GPIOC

/* --- GPIO: Power gating load switches --- */
#define LOADSW_ANA_PORT     GPIOC
#define LOADSW_ANA_PIN      1        /* PC1  — ADS1298 analog rail     */
#define LOADSW_ENV_PORT     GPIOC
#define LOADSW_ENV_PIN      2        /* PC2  — environmental sensors   */
#define LOADSW_SD_PORT      GPIOC
#define LOADSW_SD_PIN       3        /* PC3  — microSD power           */

/* --- GPIO: Status LEDs --- */
#define LED_STATUS_PORT     GPIOB
#define LED_STATUS_PIN      4        /* PB4  — green status            */
#define LED_ERROR_PORT      GPIOB
#define LED_ERROR_PIN       5        /* PB5  — red error               */

/* --- USB-C --- */
#define USB_DM_PIN          11       /* PA11 */
#define USB_DP_PIN          12       /* PA12 */

/* --- User button --- */
#define BUTTON_PORT         GPIOC
#define BUTTON_PIN          14       /* PC14 — active low, external PU */

/* ===================================================================== */
/*  ADS1298 configuration                                                 */
/* ===================================================================== */

#define ADS1298_NUM_DEVICES  2
#define ADS1298_CHANNELS_PER 8
#define ADS1298_TOTAL_CHANS  (ADS1298_NUM_DEVICES * ADS1298_CHANNELS_PER) /* 16 */
#define ADS1298_SPI_SPEED    4000000UL   /* 4 MHz max per datasheet       */
#define ADS1298_SAMPLE_RATE  1000        /* 1 kS/s per channel (configurable) */
#define ADS1298 PGA_GAIN     12          /* PGA gain ×12 (±2.5 V / 12 = ±208 mV) */
#define ADS1298_CLK_FREQ     2048000UL   /* 2.048 MHz external clock       */

/* ADS1298 register addresses (subset) */
#define ADS1298_REG_ID       0x00
#define ADS1298_REG_CONFIG1  0x01
#define ADS1298_REG_CONFIG2  0x02
#define ADS1298_REG_CONFIG3  0x03
#define ADS1298_REG_LOFF     0x04
#define ADS1298_REG_CH1SET   0x05
#define ADS1298_REG_CH2SET   0x06
#define ADS1298_REG_CH3SET   0x07
#define ADS1298_REG_CH4SET   0x08
#define ADS1298_REG_CH5SET   0x09
#define ADS1298_REG_CH6SET   0x0A
#define ADS1298_REG_CH7SET   0x0B
#define ADS1298_REG_CH8SET   0x0C
#define ADS1298_REG_BIASSENSP 0x0D
#define ADS1298_REG_BIASSENSN 0x0E
#define ADS1298_REG_LOFFSENSP 0x0F
#define ADS1298_REG_LOFFSENSN 0x10
#define ADS1298_REG_LOFFFLIP 0x11
#define ADS1298_REG_LOFFSTATP 0x12
#define ADS1298_REG_LOFFSTATN 0x13
#define ADS1298_REG_GPIO1    0x14
#define ADS1298_REG_GPIO2    0x15
#define ADS1298_REG_MISC1    0x15
#define ADS1298_REG_MISC2    0x16
#define ADS1298_REG_CONFIG4  0x17

/* ADS1298 commands */
#define ADS1298_CMD_WAKEUP   0x02
#define ADS1298_CMD_STANDBY  0x04
#define ADS1298_CMD_RESET    0x06
#define ADS1298_CMD_START    0x08
#define ADS1298_CMD_STOP     0x0A
#define ADS1298_CMD_OFFSETCAL 0x1A
#define ADS1298_CMD_CALIBRATE 0x1C
#define ADS1298_CMD_RDATA    0x10
#define ADS1298_CMD_RDATAC   0x20
#define ADS1298_CMD_SDATAC   0x11
#define ADS1298_CMD_RREG     0x20
#define ADS1298_CMD_WREG     0x40

/* ===================================================================== */
/*  DSP and spike detection constants                                     */
/* ===================================================================== */

#define DSP_BLOCK_SIZE       64       /* samples per processing block    */
#define SPIKE_WINDOW_PRE     12       /* samples before threshold crossing */
#define SPIKE_WINDOW_POST    36       /* samples after threshold crossing  */
#define SPIKE_WINDOW_TOTAL   (SPIKE_WINDOW_PRE + SPIKE_WINDOW_POST) /* 48 */
#define SPIKE_THRESHOLD_K    4.0f     /* multiplier on MAD for threshold  */
#define SPIKE_MAX_TEMPLATES  8        /* max k-means clusters per channel */
#define SPIKE_TEMPLATE_LEN   48       /* samples in a spike template      */

/* Activity classification */
#define CLASS_IDLE           0
#define CLASS_FORAGE         1
#define CLASS_TRANSPORT      2
#define CLASS_STRESS         3
#define CLASS_COMPETE        4
#define CLASS_COUNT          5

#define EPOCH_DURATION_SEC   10       /* seconds per classification epoch */
#define EPOCH_SAMPLES        (EPOCH_DURATION_SEC * ADS1298_SAMPLE_RATE)

/* Cross-channel propagation */
#define PROP_WINDOW_MS       50       /* max propagation window in ms     */
#define PROP_CORR_THRESHOLD  0.7f     /* min cross-correlation for propagation */
#define PROP_MAX_VELOCITY    10.0f    /* m/s, upper bound for sanity      */

/* ===================================================================== */
/*  LoRaWAN configuration                                                 */
/* ===================================================================== */

#define LORA_FREQ_EU868      868100000UL  /* EU868 default channel       */
#define LORA_FREQ_US915      902300000UL  /* US915 default channel       */
#define LORA_SF              9            /* Spreading factor 9          */
#define LORA_BW              125000       /* 125 kHz bandwidth           */
#define LORA_CR              4            /* Coding rate 4/5             */
#define LORA_TX_POWER_DBM    20           /* +20 dBm (SX1262 max with PA boost) */
#define LORA_PREAMBLE_LEN    8
#define LORA_MAX_PAYLOAD     51           /* EU868 max for SF9/125kHz     */

#define LORAWAN_PORT_STATUS   10
#define LORAWAN_PORT_SPIKES   11
#define LORAWAN_PORT_ALARM    12
#define LORAWAN_PORT_HEALTH   20

/* ===================================================================== */
/*  Power management                                                      */
/* ===================================================================== */

#define DUTY_CYCLE_DEFAULT_PCT   5      /* 5% active, 95% sleep           */
#define DUTY_CYCLE_CONTINUOUS    100    /* 100% = always on               */
#define ACTIVE_WINDOW_SEC        10     /* default active window           */
#define SLEEP_WINDOW_SEC         190    /* default sleep (10s + 190s = 5%) */
#define SUPERCAP_THRESH_MV       4500   /* auto-increase duty cycle above  */

/* Sleep mode currents (for estimation) */
#define CURRENT_ACTIVE_MA        10.5
#define CURRENT_SLEEP_UA         5
#define CURRENT_TX_MA            120

/* ===================================================================== */
/*  SD logging                                                            */
/* ===================================================================== */

#define SD_LOG_MAGIC            0x4D434D53U  /* "MCMS"                    */
#define SD_LOG_VERSION          1
#define SD_LOG_BLOCK_HEADER     16
#define SD_LOG_SAMPLE_BYTES     3           /* 24-bit signed per sample   */
#define SD_LOG_MAX_FILES        999

/* ===================================================================== */
/*  USB/BLE binary protocol                                               */
/* ===================================================================== */

#define PROTO_CMD_GET_STATUS    0x01
#define PROTO_CMD_START_ACQ     0x02
#define PROTO_CMD_STOP_ACQ      0x03
#define PROTO_CMD_GET_SPIKES    0x04
#define PROTO_CMD_SET_CONFIG    0x05
#define PROTO_CMD_GET_ENV       0x06
#define PROTO_CMD_CALIBRATE     0x07
#define PROTO_CMD_DFU_ENTER     0x08
#define PROTO_CMD_GET_LOG_LIST  0x09
#define PROTO_CMD_DOWNLOAD_LOG  0x0A

#define PROTO_ACK               0xA5
#define PROTO_NACK              0x5A

/* ===================================================================== */
/*  Type definitions                                                      */
/* ===================================================================== */

typedef struct {
    uint8_t  class_label;
    uint8_t  spike_rate;        /* spikes/sec across all channels       */
    uint8_t  propagation_count; /* propagating events in epoch          */
    uint8_t  reserved;
    uint16_t mean_amplitude_uv; /* mean spike amplitude in µV           */
    uint16_t isi_cv_x100;       /* inter-spike-interval CV × 100        */
    int16_t  soil_moisture;     /* % VWC × 10                           */
    int16_t  soil_temp_cx10;    /* °C × 10                              */
    uint16_t co2_ppm;           /* CO₂ in ppm                           */
    uint16_t battery_mv;        /* battery voltage in mV                */
    uint32_t timestamp;         /* epoch timestamp (Unix time)          */
} __attribute__((packed)) epoch_summary_t;

typedef struct {
    uint8_t  channel;           /* 0–15                                 */
    uint8_t  template_id;       /* which spike template (0–7)           */
    int16_t  amplitude_uv;      /* peak amplitude in µV                 */
    uint32_t timestamp_ms;      /* millisecond timestamp                */
} __attribute__((packed)) spike_event_t;

typedef struct {
    int16_t  samples[ADS1298_TOTAL_CHANS]; /* one sample per channel    */
} channel_sample_t;

#endif /* MYCOMESH_BOARD_H */