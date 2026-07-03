/*
 * board.h — RainForge board pin map & constants
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Pin assignments for the ESP32-C3FH4 on the RainForge board.
 * The ESP32-C3 has 22 GPIO (GPIO0-21, with strapping on 2/8/9).
 * We avoid the strapping pins for anything active during boot.
 */
#ifndef RAINFORGE_BOARD_H
#define RAINFORGE_BOARD_H

#include <stdint.h>

/* ---- MCU identity ---- */
#define MCU_NAME        "ESP32-C3FH4"
#define CPU_FREQ_HZ     160000000UL
#define SYSTICK_HZ      1000

/* ---- GPIO pin map ---- */
/* SPI0 (flash) is internal — never touch. We use SPI2 (GPSPI2) for
 * the FRAM and ADS131M04.  */
#define PIN_SPI2_SCK    4
#define PIN_SPI2_MISO   5
#define PIN_SPI2_MOSI   6
#define PIN_SPI2_CS_FRAM    7
#define PIN_SPI2_CS_ADC     10

/* ADS131M04 control */
#define PIN_ADC_DRDY    11      /* data-ready, falling-edge interrupt */
#define PIN_ADC_SYNC    12      /* sync/reset line */
#define PIN_ADC_PWREN   13      /* op-amp + ADC power enable (high=on) */

/* SX1262 LoRa radio (SPI3 = GSPI, single device) */
#define PIN_LORA_SCK    18
#define PIN_LORA_MISO   19
#define PIN_LORA_MOSI   20
#define PIN_LORA_CS     21
#define PIN_LORA_BUSY   17
#define PIN_LORA_DIO1   16      /* rising-edge interrupt */
#define PIN_LORA_NRST   15
#define PIN_LORA_ANT_SW 14      /* TX/RX antenna switch (high=TX) */

/* Energy harvester interface */
#define PIN_HARVEST_VOK 3       /* SE1010 V_STORE_OK — high when supercap > 3.3 V */
#define PIN_HARVEST_LOW 1       /* SE1010 V_STORE_LOW — low when supercap < 2.5 V (IRQ) */
#define PIN_PEAK_IRQ    0       /* LTC5500 peak comparator — falling edge = droplet */

/* Environmental sensors (I2C, shared) */
#define PIN_I2C_SCL     8       /* caution: strapping pin, pulled high on boot */
#define PIN_I2C_SDA     9       /* caution: strapping pin, pulled high on boot */

/* ---- I2C addresses ---- */
#define SHT45_ADDR      0x44
#define BMP390_ADDR     0x77
#define TSL2591_ADDR    0x29

/* ---- SPI clock rates ---- */
#define SPI2_SPEED_HZ   4000000     /* ADS131M04 + FRAM */
#define LORA_SPI_SPEED  8000000     /* SX1262 */

/* ---- ADC configuration ---- */
#define ADC_CHANNELS    4
#define ADC_SAMPLE_RATE 4000        /* 4 kSPS per channel */
#define ADC_WINDOW      128         /* samples captured per droplet event */
#define ADC_GAIN        1           /* PGA gain 1 (±2.5 V reference) */

/* ---- FRAM layout (4 Mbit = 512 KB) ---- */
#define FRAM_ADDR_CALIB     0x0000  /* 2 KB calibration table */
#define FRAM_ADDR_STATS     0x0800  /* 1 KB rolling statistics */
#define FRAM_ADDR_EVENTLOG  0x1000  /* 512 KB - 4 KB = event ring buffer */
#define FRAM_ADDR_CONFIG    0x7F000 /* 4 KB config block at end */
#define FRAM_SIZE           0x80000 /* 512 KB */
#define EVENT_SIZE          16      /* bytes per droplet event */
#define EVENT_RING_CAP      ((FRAM_SIZE - FRAM_ADDR_EVENTLOG - 0x1000) / EVENT_SIZE)

/* ---- DSD bins (Marshall-Palmer diameter scale, mm) ---- */
#define DSD_NUM_BINS    9
extern const float DSD_BIN_EDGES[DSD_NUM_BINS + 1];

/* ---- Super-capacitor thresholds ---- */
#define SCAP_V_OK       3.30f   /* minimum voltage for TX */
#define SCAP_V_LOW      2.50f   /* deep-sleep only below this */
#define SCAP_V_FULL     5.30f   /* stop charging above this */

/* ---- LoRaWAN ---- */
#define LORA_FREQ_HZ    868100000UL
#define LORA_BW         125000
#define LORA_SF          7
#define LORA_TX_POWER_DBM 20
#define LORA_PREAMBLE    8
#define LORA_PAYLOAD_MAX 51

/* ---- Wake sources ---- */
typedef enum {
    WAKE_PEAK = 0,      /* droplet impact (GPIO) */
    WAKE_RTC,           /* reporting timer expired */
    WAKE_VOK,           /* supercap reached operating voltage */
    WAKE_BLE,           /* BLE connection (calibration) */
    WAKE_RESET          /* power-on / external reset */
} wake_source_t;

/* ---- Operating state ---- */
typedef enum {
    STATE_COLDSTART = 0,    /* supercap too low; harvest only */
    STATE_HARVESTING,       /* rain falling, accumulating events */
    STATE_READY_TX,         /* enough energy + enough time → report */
    STATE_TX,               /* transmitting LoRa packet */
    STATE_SLEEP,            /* deep sleep, waiting for rain/RTC */
    STATE_CALIBRATION,      /* calibration mode (BLE-triggered) */
    STATE_FAULT
} op_state_t;

/* ---- Global board context shared by all drivers ---- */
typedef struct {
    op_state_t  state;
    wake_source_t wake_reason;
    uint32_t    boot_count;
    uint32_t    uptime_ms;
    float       scap_voltage;
    float       temperature;
    float       humidity;
    float       pressure;
    float       ambient_lux;
    uint16_t    events_this_interval;
    uint16_t    events_total;
} board_ctx_t;

extern board_ctx_t g_board;

/* ---- Convenience macros ---- */
#define ARRAY_LEN(a)    (sizeof(a)/sizeof((a)[0]))
#define CLAMP(x,lo,hi)  ((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))
#define MIN(a,b)        ((a)<(b)?(a):(b))
#define MAX(a,b)        ((a)>(b)?(a):(b))

#endif /* RAINFORGE_BOARD_H */