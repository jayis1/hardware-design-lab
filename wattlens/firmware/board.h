/*
 * board.h — WattLens board configuration and pin map
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * STM32H743VIT6 pin assignments and peripheral configuration.
 */

#ifndef WATTLENS_BOARD_H
#define WATTLENS_BOARD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ========================================================================
 * Clock configuration
 * ======================================================================== */
#define HSE_FREQ_HZ         16000000UL   /* External 16 MHz crystal */
#define SYSCLK_HZ           480000000UL  /* 480 MHz max for H743 */
#define AHB_DIV             1
#define APB1_DIV            4            /* APB1 = 120 MHz */
#define APB2_DIV            2            /* APB2 = 240 MHz */
#define APB1_TIMER_HZ       120000000UL
#define APB2_TIMER_HZ       240000000UL

/* ========================================================================
 * Sampling configuration
 * ======================================================================== */
#define SAMPLE_RATE_HZ      2048         /* ADS131M08 sample rate */
#define WINDOW_SAMPLES      2048         /* 1-second window */
#define NUM_CHANNELS        8            /* 3V + 3I + neutral + leakage */
#define GRID_FREQ_HZ        50           /* 50 or 60, auto-detectable */
#define HARMONIC_MAX_ORDER  50           /* IEC 61000-4-7 */
#define FFT_SIZE            2048         /* radix-2 */
#define NILM_MAX_CLASSES    16

/* Channel indices in ADS131M08 frame */
#define CH_V1               0
#define CH_V2               1
#define CH_V3               2
#define CH_I1               3
#define CH_I2               4
#define CH_I3               5
#define CH_IN               6            /* neutral current */
#define CH_IL               7            /* leakage / auxiliary */

/* ========================================================================
 * STM32H743VIT6 (LQFP-100) Pin Map
 * ======================================================================== */

/* --- Port A --- */
#define PA0  ADC1_IN0        /* (not used — reserved) */
#define PA3  USART2_RX       /* BLE nRF52840 UART RX */
#define PA2  USART2_TX       /* BLE nRF52840 UART TX */
#define PA4  DAC1_OUT1       /* (not used) */
#define PA5  SPI1_NSS        /* (not used — ADS uses software CS) */
#define PA6  SPI1_MISO       /* (reserved) */
#define PA7  SPI1_MOSI       /* (reserved) */
#define PA9  USART1_TX       /* USB-C CDC virtual UART (remap) — actually USB */
#define PA11 USB_DM          /* USB-C data- */
#define PA12 USB_DP          /* USB-C data+ */
#define PA15 SPI3_NSS        /* Display CS (ILI9341) */

/* --- Port B --- */
#define PB0  ADC1_IN9        /* battery voltage divider (analog) */
#define PB1  ADC1_IN8        /* (reserved analog) */
#define PB2  BOOT1           /* (boot strap) */
#define PB3  SPI1_SCK        /* (reserved) */
#define PB4  SPI3_MISO       /* Display MISO */
#define PB5  SPI3_MOSI       /* Display MOSI */
#define PB6  I2C1_SCL        /* RTC + IMU + fuel gauge */
#define PB7  I2C1_SDA        /* RTC + IMU + fuel gauge */
#define PB10 USART3_TX       /* LoRa SX1262 UART TX */
#define PB11 USART3_RX       /* LoRa SX1262 UART RX */
#define PB12 SPI2_NSS        /* ADS131M08 CS (ADC) */
#define PB13 SPI2_SCK        /* ADS131M08 SCK */
#define PB14 SPI2_MISO       /* ADS131M08 MISO */
#define PB15 SPI2_MOSI       /* ADS131M08 MOSI */

/* --- Port C --- */
#define PC0  ADC1_IN10       /* (reserved) */
#define PC4  GPIO_OUT        /* ADS131M08 RESET */
#define PC5  GPIO_IN_IRQ     /* ADS131M08 DRDY (EXTI5) */
#define PC6  GPIO_OUT        /* Display DC/RS */
#define PC7  GPIO_OUT        /* Display RESET */
#define PC8  GPIO_OUT        /* SD card CS (SDIO/SPI mode) */
#define PC9  GPIO_OUT        /* LoRa SX1262 RESET */
#define PC10 USART4_TX       /* (reserved — debug UART) */
#define PC11 USART4_RX       /* (reserved — debug UART) */
#define PC12 GPIO_OUT        /* LoRa SX1262 NSS (auxiliary SPI CS) */
#define PC13 GPIO_IN         /* SD card detect */
#define PC14 OSC32_IN        /* LSE 32.768 kHz */
#define PC15 OSC32_OUT       /* LSE 32.768 kHz */

/* --- Port D --- */
#define PD0  GPIO_OUT        /* status LED (red) */
#define PD1  GPIO_OUT        /* status LED (green) */
#define PD2  GPIO_OUT        /* TP4056 charge enable */
#define PD3  GPIO_IN_IRQ     /* nRF52840 BLE IRQ (optional) */
#define PD4  GPIO_IN_IRQ     /* SX1262 DIO1 (LoRa IRQ, EXTI4) */
#define PD5  GPIO_OUT        /* battery charge status LED */
#define PD8  GPIO_OUT        /* QSPI flash CS */
#define PD11 GPIO_OUT        /* QSPI CLK */
#define PD12 GPIO_OUT        /* QSPI IO0 */
#define PD13 GPIO_OUT        /* QSPI IO1 */

/* --- Port E --- */
#define PE0  GPIO_IN_IRQ     /* Display touch IRQ (EXTI0) */
#define PE1  GPIO_OUT        /* ADS131M08 SYNC/PWDN */
#define PE7  GPIO_OUT        /* USB-C VBUS enable */
#define PE8  GPIO_OUT        /* main power enable */

/* ========================================================================
 * Peripheral assignments
 * ======================================================================== */
#define ADC_SPI          SPI2             /* ADS131M08 on SPI2 */
#define DISP_SPI         SPI3             /* ILI9341 on SPI3 */
#define BLE_UART         USART2           /* nRF52840 */
#define LORA_UART        USART3           /* SX1262 (UART config mode) */
#define SENSORS_I2C      I2C1             /* RTC, IMU, fuel gauge */
#define USB_PERIPH       USB1_OTG         /* USB-C CDC */

/* EXTI lines */
#define EXTI_DRDY        5                /* PC5 — ADC data ready */
#define EXTI_LORA_IRQ    4                /* PD4 — SX1262 DIO1 */
#define EXTI_TOUCH       0                /* PE0 — display touch */

/* ========================================================================
 * Calibration constants (stored in EEPROM, defaults shown)
 * ======================================================================== */
typedef struct {
    float v_gain[3];       /* voltage channel gains (V/count) */
    float i_gain[4];       /* current channel gains (A/count) */
    float v_phase[3];      /* voltage phase correction (radians) */
    float i_phase[4];      /* current phase correction (radians) */
    float v_offset[3];     /* voltage DC offset (counts) */
    float i_offset[4];     /* current DC offset (counts) */
    float ct_ratio[4];     /* CT turns ratio (e.g., 100.0 for 100 A:0.333 A) */
    float ct_burden[4];    /* CT burden resistor (ohms) */
    uint8_t grid_freq;     /* 50 or 60 */
    uint8_t reserved[7];   /* alignment / future use */
    uint32_t crc;          /* CRC32 over preceding fields */
} cal_data_t;

#define DEFAULT_V_GAIN      0.0000358f   /* V/count for 230 V / 1000:1 divider */
#define DEFAULT_I_GAIN      0.0001220f   /* A/count for 100 A CT */
#define DEFAULT_CT_RATIO    100.0f
#define DEFAULT_CT_BURDEN   33.0f

/* ========================================================================
 * Measurement result structures
 * ======================================================================== */
typedef struct {
    float v_rms[3];          /* phase voltages (V) */
    float i_rms[4];          /* phase + neutral currents (A) */
    float p_real[3];         /* per-phase real power (W) */
    float p_reactive[3];     /* per-phase reactive power (var) */
    float p_apparent[3];     /* per-phase apparent power (VA) */
    float pf[3];             /* per-phase power factor */
    float p_total_real;      /* total 3-phase real power (W) */
    float p_total_reactive;  /* total 3-phase reactive (var) */
    float p_total_apparent;  /* total 3-phase apparent (VA) */
    float pf_total;          /* total power factor */
    float freq;              /* grid frequency (Hz) */
    float thd_v[3];          /* voltage THD (%) */
    float thd_i[3];          /* current THD (%) */
} power_metrics_t;

typedef struct {
    float v_mag[3][HARMONIC_MAX_ORDER + 1]; /* voltage harmonic magnitude (V) */
    float i_mag[3][HARMONIC_MAX_ORDER + 1]; /* current harmonic magnitude (A) */
    float v_ph[3][HARMONIC_MAX_ORDER + 1]; /* voltage harmonic phase (rad) */
    float i_ph[3][HARMONIC_MAX_ORDER + 1]; /* current harmonic phase (rad) */
} harmonic_data_t;

typedef struct {
    float nilm_probs[NILM_MAX_CLASSES];     /* appliance probabilities */
    uint8_t nilm_state[NILM_MAX_CLASSES];   /* on/off per class */
    float nilm_power[NILM_MAX_CLASSES];     /* estimated power per appliance (W) */
} nilm_result_t;

typedef enum {
    PQ_EVENT_NONE = 0,
    PQ_EVENT_SAG,
    PQ_EVENT_SWELL,
    PQ_EVENT_INTERRUPTION,
    PQ_EVENT_FLICKER,
    PQ_EVENT_HARMONIC_EXCEED,
    PQ_EVENT_NILM_ON,
    PQ_EVENT_NILM_OFF,
} pq_event_type_t;

typedef struct {
    uint32_t timestamp;       /* RTC epoch */
    pq_event_type_t type;
    uint8_t phase;            /* 0=L1, 1=L2, 2=L3, 3=all */
    float severity;           /* event-specific (V deviation, Pst, etc.) */
    float duration_ms;
} pq_event_t;

/* ========================================================================
 * Device state
 * ======================================================================== */
typedef enum {
    STATE_BOOT = 0,
    STATE_IDLE,
    STATE_MEASURE,
    STATE_CALIBRATE,
    STATE_CAPTURE,
    STATE_ERROR
} device_state_t;

typedef struct {
    device_state_t state;
    cal_data_t cal;
    power_metrics_t metrics;
    harmonic_data_t harmonics;
    nilm_result_t nilm;
    uint32_t uptime_s;
    uint32_t sample_count;
    uint16_t error_flags;
    uint8_t battery_pct;
    bool sd_present;
    bool ble_connected;
    bool lora_joined;
} device_ctx_t;

/* Error flags */
#define ERR_ADC_SPI      (1 << 0)
#define ERR_ADC_TIMEOUT  (1 << 1)
#define ERR_SD_MOUNT     (1 << 2)
#define ERR_SD_WRITE     (1 << 3)
#define ERR_BLE_INIT     (1 << 4)
#define ERR_LORA_INIT    (1 << 5)
#define ERR_CAL_INVALID  (1 << 6)
#define ERR_NILM_MODEL   (1 << 7)
#define ERR_BATTERY_LOW  (1 << 8)
#define ERR_OVERTEMP     (1 << 9)

/* Timing constants */
#define SAMPLE_PERIOD_MS     (1000 / SAMPLE_RATE_HZ)
#define WINDOW_PERIOD_MS     (WINDOW_SAMPLES * 1000 / SAMPLE_RATE_HZ)
#define BLE_TX_INTERVAL_MS   1000
#define LORA_TX_INTERVAL_MS  30000
#define DISPLAY_UPDATE_MS    500
#define EVENT_DEBOUNCE_S     3

#endif /* WATTLENS_BOARD_H */