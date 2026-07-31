/*
 * board.h — nRF5340 pin map, clock configuration, and Synthand constants.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SYNTHAND_BOARD_H
#define SYNTHAND_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* -------------------------------------------------------------------------
 * MCU identity
 * ------------------------------------------------------------------------- */
#define MCU_NAME        "nRF5340"
#define SYSCLK_HZ       128000000U     /* 128 MHz application core */
#define HFXO_FREQ_HZ    32000000U      /* 32 MHz HF crystal */
#define LFXO_FREQ_HZ    32768U         /* 32.768 kHz LF crystal */
#define HCLK_HZ         SYSCLK_HZ

#define FLASH_SIZE_BYTES  (1024 * 1024)
#define RAM_SIZE_BYTES    (512 * 1024)

/* -------------------------------------------------------------------------
 * GPIO pin assignments (nRF5340 QKAAAB0A, aQFN94)
 *
 *  P0.02  — SPIM0_SCK     (IMU SPI bus — 6× ICM-42688-P)
 *  P0.03  — SPIM0_MOSI
 *  P0.04  — SPIM0_MISO
 *  P0.05  — IMU_CS0        (GPIO, finger 0 / thumb IMU)
 *  P0.06  — IMU_CS1        (GPIO, finger 1 / index IMU)
 *  P0.07  — IMU_CS2        (GPIO, finger 2 / middle IMU)
 *  P0.08  — IMU_CS3        (GPIO, finger 3 / ring IMU)
 *  P0.09  — IMU_CS4        (GPIO, finger 4 / pinky IMU)
 *  P0.10  — IMU_CS5        (GPIO, wrist IMU)
 *  P0.11  — SPIM1_SCK      (EMG SPI bus — 3× ADS1292)
 *  P0.12  — SPIM1_MOSI
 *  P0.13  — SPIM1_MISO
 *  P0.14  — EMG_CS0        (GPIO, ADS1292 #1, channels 0-1)
 *  P0.15  — EMG_CS1        (GPIO, ADS1292 #2, channels 2-3)
 *  P0.16  — EMG_CS2        (GPIO, ADS1292 #3, channel 4)
 *  P0.17  — EMG_DRDY       (GPIO input, ADS1292 data ready IRQ)
 *
 *  P0.19  — TWIM0_SCL      (I²C bus — 5× DRV2605L + BQ27426)
 *  P0.20  — TWIM0_SDA
 *  P0.21  — HAPTIC_LRA_EN  (GPIO, global LRA enable / power gate)
 *
 *  P0.23  — USB_DP         (USB-C data+)
 *  P0.24  — USB_DM         (USB-C data-)
 *
 *  P0.25  — BAT_SENSE      (SAADC input — battery voltage divider)
 *  P0.26  — TEMP_SENSE     (SAADC input — NTC thermistor)
 *
 *  P0.28  — LED_STATUS     (GPIO output, RGB LED — status)
 *  P0.29  — BUTTON_MODE    (GPIO input, push button — mode/cycle)
 *
 *  P0.31  — NFC_ANT1       (NT3H2111 antenna — shared with GPIO)
 *  P1.00  — NFC_ANT2
 *
 *  P1.03  — CHG_STAT       (GPIO input, MCP73831 charge status)
 *  P1.04  — SHIP_MODE      (GPIO output, battery ship-mode FET gate)
 *
 *  P1.06  — ANTENNA_PSEL   (RF antenna switch, if external front-end)
 * ------------------------------------------------------------------------- */

/* SPI0 — IMU bus */
#define PIN_SPIM0_SCK       2
#define PIN_SPIM0_MOSI      3
#define PIN_SPIM0_MISO      4
#define PIN_IMU_CS0         5
#define PIN_IMU_CS1         6
#define PIN_IMU_CS2         7
#define PIN_IMU_CS3         8
#define PIN_IMU_CS4         9
#define PIN_IMU_CS5         10

/* SPI1 — EMG bus */
#define PIN_SPIM1_SCK       11
#define PIN_SPIM1_MOSI      12
#define PIN_SPIM1_MISO      13
#define PIN_EMG_CS0         14
#define PIN_EMG_CS1         15
#define PIN_EMG_CS2         16
#define PIN_EMG_DRDY        17

/* I²C — haptic + battery */
#define PIN_TWIM0_SCL       19
#define PIN_TWIM0_SDA       20
#define PIN_HAPTIC_EN       21

/* USB-C */
#define PIN_USB_DP          23
#define PIN_USB_DM          24

/* Analog */
#define PIN_BAT_SENSE       25  /* SAADC AIN5 */
#define PIN_TEMP_SENSE      26  /* SAADC AIN6 */

/* LED + button */
#define PIN_LED_STATUS      28
#define PIN_BUTTON_MODE     29

/* Power management */
#define PIN_CHG_STAT        3   /* P1.03 */
#define PIN_SHIP_MODE       4   /* P1.04 */

/* -------------------------------------------------------------------------
 * Sampling configuration
 * ------------------------------------------------------------------------- */
#define SAMPLE_RATE_HZ      500U
#define SAMPLE_PERIOD_MS    (1000U / SAMPLE_RATE_HZ)   /* 2 ms */
#define SAMPLE_PERIOD_TICKS (HFXO_FREQ_HZ / SAMPLE_RATE_HZ)

#define NUM_FINGERS         5
#define NUM_IMUS            6   /* 5 fingers + 1 wrist */
#define NUM_EMG_CHANNELS    5
#define NUM_HAPTIC          5

#define GESTURE_CLASSES     12
#define TCN_WINDOW          80   /* 160 ms at 500 Hz */
#define TCN_FEATURES        38
#define TCN_INFERENCE_DIV   10   /* run inference every 10th sample = 20 ms */

/* -------------------------------------------------------------------------
 * BLE-MIDI configuration
 * ------------------------------------------------------------------------- */
#define BLE_MIDI_MTU        128
#define BLE_CONN_INTERVAL_MS 6   /* 7.5 ms min × 0.8 ≈ 6 ms target */
#define MIDI_RING_SIZE      64

/* -------------------------------------------------------------------------
 * I²C addresses
 * ------------------------------------------------------------------------- */
#define DRV2605L_BASE_ADDR  0x5A  /* A0=A1=0: 0x5A, A0=1,A1=0: 0x5B, etc. */
#define DRV2605L_ADDR(ch)   (DRV2605L_BASE_ADDR + (ch))
#define BQ27426_ADDR        0x55

/* -------------------------------------------------------------------------
 * Power thresholds
 * ------------------------------------------------------------------------- */
#define BAT_LOW_MV          3400   /* alert below 3.4V */
#define BAT_CRIT_MV         3200   /* shutdown below 3.2V */
#define BAT_FULL_MV         4200
#define TEMP_ALERT_MC       42000  /* 42 °C in milli-celsius */
#define TEMP_SHUTDOWN_MC    45000  /* 45 °C */

/* -------------------------------------------------------------------------
 * Haptic waveform IDs (DRV2605L built-in library)
 * ------------------------------------------------------------------------- */
#define HAPTIC_WAVEFORM_NONE     0
#define HAPTIC_WAVEFORM_CLICK    17   /* sharp click — drum tap */
#define HAPTIC_WAVEFORM_SOFT_BUZZ 47  /* string pluck */
#define HAPTIC_WAVEFORM_BUMP     22   /* fret press */
#define HAPTIC_WAVEFORM_RAMP     72   /* sustain rise */
#define HAPTIC_WAVEFORM_DOUBLE   65   /* error/alert */
#define HAPTIC_WAVEFORM_STRONG   56   /* strong click */

/* -------------------------------------------------------------------------
 * System states
 * ------------------------------------------------------------------------- */
typedef enum {
    STATE_BOOT = 0,
    STATE_CALIBRATING,
    STATE_IDLE,          /* BLE connected, sensors on, not playing */
    STATE_PLAYING,       /* active gesture capture + MIDI output */
    STATE_CHARGING,
    STATE_SHUTDOWN,
    STATE_SHIP,
} system_state_t;

/* -------------------------------------------------------------------------
 * MIDI mapping defaults
 * ------------------------------------------------------------------------- */
#define DEFAULT_MIDI_CHANNEL    0   /* channel 1 (0-indexed) */
#define DEFAULT_CC_EMG_BASE     20  /* CC 20-24 = EMG 0-4 */
#define DEFAULT_CC_CURL_BASE    30  /* CC 30-34 = finger curl 0-4 */
#define DEFAULT_CC_VIBRATO      35  /* CC 35 = vibrato depth */
#define DEFAULT_CC_MOD          1   /* CC 1 = modulation (wrist rotation) */
#define DEFAULT_PB_CHANNEL      0   /* pitch bend on channel 1 */

/* Default per-finger note assignments (drum kit) */
#define DEFAULT_NOTE_FINGER0    36  /* C1 — kick drum */
#define DEFAULT_NOTE_FINGER1    38  /* D1 — snare */
#define DEFAULT_NOTE_FINGER2    42  /* F#1 — closed hi-hat */
#define DEFAULT_NOTE_FINGER3    46  /* A#1 — open hi-hat */
#define DEFAULT_NOTE_FINGER4    49  /* C#2 — crash */

/* -------------------------------------------------------------------------
 * Calibration data structure (stored in flash)
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t    magic;              /* 0x534E4844 = "SNHD" */
    uint16_t    emg_baseline[NUM_EMG_CHANNELS];   /* resting ADC counts */
    uint16_t    emg_mvc[NUM_EMG_CHANNELS];        /* max voluntary contraction */
    int16_t     gyro_bias[NUM_IMUS][3];           /* gyroscope bias (dps) */
    int16_t     accel_bias[NUM_IMUS][3];          /* accelerometer bias */
    int16_t     mag_soft_iron[3][3];              /* magnetometer correction */
    int16_t     mag_hard_iron[3];                 /* magnetometer offset */
    uint8_t     handedness;                       /* 0=right, 1=left */
    uint8_t     reserved[3];
    uint32_t    crc32;
} calibration_t;

#define CALIBRATION_MAGIC 0x534E4844U
#define CALIBRATION_FLASH_PAGE 254  /* last page before UICR */

/* -------------------------------------------------------------------------
 * MIDI mapping configuration (stored in flash)
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t    magic;
    uint8_t     midi_channel;
    uint8_t     notes[NUM_FINGERS];       /* per-finger note numbers */
    uint8_t     cc_emg[NUM_EMG_CHANNELS]; /* CC numbers for EMG envelopes */
    uint8_t     cc_curl[NUM_FINGERS];     /* CC numbers for finger curl */
    uint8_t     cc_vibrato;
    uint8_t     cc_mod;
    uint8_t     haptic_waveform[GESTURE_CLASSES];
    uint16_t    emg_threshold;            /* Q15 threshold for press trigger */
    uint16_t    tap_accel_threshold;      /* m/s² (Q8) for tap detection */
    uint8_t     vibrato_sensitivity;      /* 1-10 */
    uint8_t     reserved[7];
    uint32_t    crc32;
} mapping_t;

#define MAPPING_MAGIC 0x4D41504DU  /* "MAPM" */
#define MAPPING_FLASH_PAGE 253

#endif /* SYNTHAND_BOARD_H */