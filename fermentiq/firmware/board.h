/*
 * board.h — FermenTiq Hardware Board Definitions
 *
 * Pin assignments, I2C addresses, hardware constants, and configuration
 * for the ESP32-S3-based multi-modal fermentation monitor.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef FERMENTIQ_BOARD_H
#define FERMENTIQ_BOARD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ========================================================================
 * Author / Metadata
 * ======================================================================== */
#define FERMENTIQ_AUTHOR        "jayis1"
#define FERMENTIQ_VERSION       "1.0.0"
#define FERMENTIQ_BUILD_DATE    "2026-08-07"

/* ========================================================================
 * MCU / Platform
 * ======================================================================== */
#define PLATFORM_ESP32S3        1
#define CPU_FREQ_MHZ            240
#define MAIN_STACK_SIZE         (8 * 1024)

/* ========================================================================
 * GPIO Pin Assignments (ESP32-S3-WROOM-1)
 * ======================================================================== */

/* --- I2C Bus (shared: AD5933, LMP91200, SHT41, MAX17048) --- */
#define I2C_MASTER_SDA          8       /* GPIO8  — I2C data  */
#define I2C_MASTER_SCL          9       /* GPIO9  — I2C clock */
#define I2C_MASTER_FREQ         400000  /* 400 kHz fast-mode  */
#define I2C_PORT_NUM            I2C_NUM_0

/* --- SPI Bus (MAX31865 RTD) --- */
#define SPI_MOSI                11      /* GPIO11 — SPI MOSI  */
#define SPI_MISO                13      /* GPIO13 — SPI MISO  */
#define SPI_SCLK                12      /* GPIO12 — SPI CLK   */
#define RTD_CS                  10      /* GPIO10 — MAX31865 CS */
#define SPI_BUS_MAX_FREQ        4000000 /* 4 MHz               */

/* --- UART2 (Senseair S8 NDIR CO2) --- */
#define CO2_UART_TX             17      /* GPIO17 — UART2 TX  */
#define CO2_UART_RX             18      /* GPIO18 — UART2 RX  */
#define CO2_UART_BAUD           9600

/* --- I2S (MEMS Microphone SPH-0645LM4H-B) --- */
#define I2S_WS                  4       /* GPIO4  — I2S WS/LRCLK */
#define I2S_SCK                 5       /* GPIO5  — I2S BCLK     */
#define I2S_SD                  6       /* GPIO6  — I2S DATA IN  */
#define I2S_PORT                I2S_NUM_0
#define I2S_SAMPLE_RATE         16000
#define I2S_DMA_BUF_COUNT       4
#define I2S_DMA_BUF_LEN         1024

/* --- ADC (ISFET pH via LMP91200) --- */
#define PH_ADC_CHANNEL          ADC1_CHANNEL_0  /* GPIO1 */
#define PH_ADC_ATTEN            ADC_ATTEN_DB_11 /* 0-3.3V */
#define PH_ADC_WIDTH            ADC_WIDTH_BIT_12

/* --- Analog Switch (ADG715) for 4-wire impedance Kelvin --- */
#define ADG715_ADDR             0x4B    /* I2C addr (A0=A1=1) */

/* --- Power / Battery --- */
#define BATTERY_FUEL_GAUGE_ADDR 0x36    /* MAX17048 I2C address */
#define CHARGER_STAT_PIN        21      /* GPIO21 — TP4056 STAT  */
#define USB_VBUS_DETECT_PIN     20      /* GPIO20 — USB VBUS     */
#define BATTERY_LOW_MV          3300    /* 3.3V low threshold    */
#define BATTERY_CRIT_MV         3100    /* 3.1V critical          */

/* --- Status LED --- */
#define STATUS_LED_PIN          2       /* GPIO2 — WS2812 / simple LED */
#define STATUS_LED_ON           1

/* --- SD Card (optional, via SPI2, shares SPI with RTD) --- */
#define SD_CS                   14      /* GPIO14 — SD card CS    */
#define SD_MOUNT_POINT          "/sdcard"

/* ========================================================================
 * I2C Device Addresses
 * ======================================================================== */
#define AD5933_I2C_ADDR         0x0D    /* AD5933 default addr    */
#define LMP91200_I2C_ADDR       0x09    /* LMP91200 config addr   */
#define SHT41_I2C_ADDR          0x44    /* SHT41 temp/RH          */
#define MAX17048_I2C_ADDR       0x36    /* Fuel gauge             */

/* ========================================================================
 * AD5933 Impedance Analyzer Configuration
 * ======================================================================== */
#define AD5933_START_FREQ_HZ    1000UL      /* Sweep start 1 kHz   */
#define AD5933_DELTA_FREQ_HZ    2000UL      /* 2 kHz step          */
#define AD5933_NUM_INCR         50          /* 50 points (1-100kHz) */
#define AD5933_SETTLE_CYCLES    15          /* Settling time        */
#define AD5933_PGA_GAIN         1           /* PGA gain x1          */
#define AD5933_EXT_SYSTEM_CLK   0           /* Internal 16.776 MHz  */
#define AD5933_RANGE_4WIRE      1           /* 4-wire Kelvin mode   */

/* ========================================================================
 * Sensor Sampling Intervals (milliseconds)
 * ======================================================================== */
#define SAMPLE_INTERVAL_IMPEDANCE   60000   /* 60 s  */
#define SAMPLE_INTERVAL_CO2         15000   /* 15 s  */
#define SAMPLE_INTERVAL_PH          30000   /* 30 s  */
#define SAMPLE_INTERVAL_TEMP        10000   /* 10 s  */
#define SAMPLE_INTERVAL_AMBIENT     60000   /* 60 s  */
#define FUSION_INTERVAL             60000   /* 60 s  */
#define MQTT_PUBLISH_INTERVAL       30000   /* 30 s  */
#define LOG_FLUSH_INTERVAL          5000    /* 5 s   */

/* ========================================================================
 * Fermentation Phase Definitions
 * ======================================================================== */
typedef enum {
    PHASE_IDLE = 0,
    PHASE_LAG,
    PHASE_EXPONENTIAL,
    PHASE_STATIONARY,
    PHASE_DECLINE,
    PHASE_STUCK,
    PHASE_SPOILED,
    PHASE_UNKNOWN
} fermentation_phase_t;

static const char *phase_names[] = {
    "idle", "lag", "exponential", "stationary",
    "decline", "stuck", "spoiled", "unknown"
};

/* ========================================================================
 * Fermentation Type (for recipe-specific model parameters)
 * ======================================================================== */
typedef enum {
    FERM_BEER = 0,
    FERM_WINE,
    FERM_CIDER,
    FERM_KOMBUCHA,
    FERM_YOGURT,
    FERM_KEFIR,
    FERM_KIMCHI,
    FERM_SAUERKRAUT,
    FERM_SOURDOUGH,
    FERM_CUSTOM
} fermentation_type_t;

static const char *ferm_type_names[] = {
    "beer", "wine", "cider", "kombucha", "yogurt", "kefir",
    "kimchi", "sauerkraut", "sourdough", "custom"
};

/* ========================================================================
 * Default Alarm Thresholds
 * ======================================================================== */
#define DEFAULT_TEMP_MIN_C      15.0f
#define DEFAULT_TEMP_MAX_C      35.0f
#define DEFAULT_PH_MIN          3.0f
#define DEFAULT_PH_MAX          7.5f
#define DEFAULT_CO2_MAX_PPM     8000
#define DEFAULT_SPOILAGE_THRESH 60      /* 0-100 risk score */
#define DEFAULT_VESSEL_VOL_L    19.0f   /* 5 gal carboy */

/* ========================================================================
 * Data Structures — Shared Sensor State
 * ======================================================================== */
typedef struct {
    /* Impedance sweep (8 derived features for TinyML) */
    float z_mag_1k;         /* |Z| at 1 kHz (ohms)           */
    float z_mag_10k;        /* |Z| at 10 kHz                 */
    float z_mag_100k;       /* |Z| at 100 kHz                */
    float z_phase_10k;      /* Phase at 10 kHz (degrees)     */
    float z_phase_100k;     /* Phase at 100 kHz              */
    float cole_alpha;       /* Cole-Cole alpha               */
    float cole_r0;          /* Extracellular resistance       */
    float cole_rinf;        /* Infinite-freq resistance       */
    float cell_density;     /* Model output: cells/mL        */
    float cell_density_log; /* log10(cells/mL)               */
    uint64_t timestamp_ms;  /* Sample timestamp              */
    bool valid;
} impedance_data_t;

typedef struct {
    uint16_t co2_ppm;       /* CO2 concentration (ppm)       */
    float cer_mmol_lh;      /* CO2 evolution rate (mmol/L/h) */
    float co2_dissolved;    /* Estimated dissolved CO2 (g/L) */
    uint64_t timestamp_ms;
    bool valid;
} co2_data_t;

typedef struct {
    float ph;               /* pH value                      */
    float ph_raw_mv;        /* Raw pH voltage (mV)           */
    float ph_rate;          /* pH change rate (pH/hour)      */
    uint64_t timestamp_ms;
    bool valid;
} ph_data_t;

typedef struct {
    float temp_c;           /* Liquid temperature (°C)       */
    float ambient_temp_c;   /* Air temperature (°C)          */
    float ambient_rh;       /* Relative humidity (%)         */
    float dew_point_c;      /* Dew point (°C)                */
    uint64_t timestamp_ms;
    bool valid;
} temp_data_t;

typedef struct {
    float bubble_rate;      /* Bubbles per minute            */
    float spectral_centroid;/* Audio spectral centroid (Hz)  */
    float rms_level;        /* RMS audio level               */
    uint64_t timestamp_ms;
    bool valid;
} acoustic_data_t;

typedef struct {
    fermentation_phase_t phase;
    float abv_estimate;     /* Estimated ABV (%)             */
    float attenuation;      /* Apparent attenuation (%)      */
    int spoilage_risk;      /* 0-100 risk score              */
    float health_score;     /* 0-100 fermentation health     */
    uint64_t batch_start_ms;
    uint32_t batch_age_hours;
    uint64_t timestamp_ms;
} fusion_data_t;

typedef struct {
    /* Batch configuration */
    fermentation_type_t type;
    char batch_name[32];
    float vessel_volume_l;
    float temp_min_c;
    float temp_max_c;
    float ph_min;
    float ph_max;
    uint16_t co2_max_ppm;
    uint8_t spoilage_threshold;
    bool active;

    /* Live sensor data (written by sensor tasks, read by fusion/BLE) */
    impedance_data_t impedance;
    co2_data_t co2;
    ph_data_t ph;
    temp_data_t temp;
    acoustic_data_t acoustic;
    fusion_data_t fusion;

    /* Battery */
    float battery_soc;      /* State of charge (%)           */
    float battery_mv;       /* Battery voltage (mV)          */
    bool usb_connected;
} fermentiq_state_t;

/* Global state (defined in main.c) */
extern fermentiq_state_t g_state;

/* Mutex for thread-safe state access */
extern void *g_state_mutex;  /* SemaphoreHandle_t, cast to void* */

/* ========================================================================
 * Utility Macros
 * ======================================================================== */
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#define DEG_TO_RAD(x)   ((x) * 0.017453292519943295f)
#define RAD_TO_DEG(x)   ((x) * 57.295779513082320876f)

#define FERMENTIQ_TAG    "fermentiq"

/* pH conversion: Nernst slope at 25°C ≈ 59.16 mV/pH */
#define PH_NERNST_MV     59.16f
#define PH_ADC_VREF_MV   3300.0f
#define PH_ADC_MAX       4095.0f

/* Henry's law constant for CO2 in water at 25°C (mol/L/atm) */
#define HENRY_CO2_25C    0.034f

/* Stoichiometric: 1 mol glucose → 2 mol CO2 + 2 mol ethanol */
#define GLUCOSE_TO_ETHANOL_MOL_RATIO 2.0f
#define ETHANOL_MW_GMOL  46.07f
#define ETHANOL_DENSITY   0.789f  /* g/mL */

#endif /* FERMENTIQ_BOARD_H */