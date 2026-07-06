/*
 * board.h — TactiScript hardware pin map and board configuration
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Target: Nordic nRF5340 (QFAA-AB0), custom TactiScript ring PCB.
 * The nRF5340 has two cores: APP core (network + app) and NET core.
 * Pin assignments below are for the APP core. GPIO port 0 = P0.x,
 * port 1 = P1.x.
 */

#ifndef TACTISCRIPT_BOARD_H
#define TACTISCRIPT_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ----------------------------------------------------------------
 * Clock configuration
 * ---------------------------------------------------------------- */
#define BOARD_HFCLK_HZ       32768   /* LFXO (32.768 kHz LF crystal) */
#define BOARD_HFCO_HZ        32000000 /* HFXO 32 MHz crystal */
#define BOARD_CPU_HZ          128000000 /* APP core max clock */

/* ----------------------------------------------------------------
 * GPIO pin assignments (APP core)
 * ---------------------------------------------------------------- */

/* --- SPI bus 0: actuator driver chain (DRV2700 x6) --- */
#define PIN_SPI0_SCK          0, 6    /* P0.06 */
#define PIN_SPI0_MOSI         0, 7    /* P0.07 */
#define PIN_SPI0_MISO         0, 8    /* P0.08 (unused but routed) */
#define PIN_SPI0_CS_DRV       0, 11   /* DRV2700 chain CS (active low) */

/* --- SPI bus 1: IMU (ICM-42688-P) --- */
#define PIN_SPI1_SCK          0, 14   /* P0.14 */
#define PIN_SPI1_MOSI         0, 15   /* P0.15 */
#define PIN_SPI1_MISO         0, 16   /* P0.16 */
#define PIN_SPI1_CS_IMU       0, 17   /* P0.17 (active low) */
#define PIN_IMU_INT1          0, 19   /* P0.19 — IMU interrupt 1 (data ready) */
#define PIN_IMU_INT2          0, 20   /* P0.20 — IMU interrupt 2 (gesture) */

/* --- I2C bus: OLED + NTC + ambient light --- */
#define PIN_I2C_SCL           0, 24  /* P0.24 */
#define PIN_I2C_SDA           0, 25   /* P0.25 */

/* --- I2C addresses --- */
#define I2C_ADDR_OLED         0x3C   /* SSD1316 */
#define I2C_ADDR_NTC_MUX      0x48   /* ADC mux for NTC */
#define I2C_ADDR_ALS          0x44   /* ambient light sensor */

/* --- HV boost / actuator control --- */
#define PIN_HV_BOOST_EN       1, 0   /* P1.00 — boost converter enable */
#define PIN_HV_GATE           1, 1    /* P1.01 — per-refresh HV gate (PWM) */
#define PIN_DEMUX_A0          1, 2   /* P1.02 — demux select bit 0 */
#define PIN_DEMUX_A1          1, 3   /* P1.03 — demux select bit 1 */
#define PIN_DEMUX_A2          1, 4   /* P1.04 — demux select bit 2 */
#define PIN_DRV_LATCH         1, 5   /* P1.05 — DRV2700 output latch */

/* --- Skin contact detect (capacitive) --- */
#define PIN_SKIN_SENSE        0, 26  /* P0.26 — analog (SAADC) */
#define PIN_SKIN_DRIVE        0, 27  /* P0.27 — capacitive drive */

/* --- LRA whole-ring vibration motor --- */
#define PIN_LRA_EN            1, 6   /* P1.06 — LRA enable */
#define PIN_LRA_PWM           1, 7   /* P1.07 — LRA PWM drive */

/* --- NFC (passive, uses antenna pins) --- */
#define PIN_NFC_ANT1          0, 9   /* P0.09 — NFC antenna 1 */
#define PIN_NFC_ANT2          0, 10  /* P0.10 — NFC antenna 2 */

/* --- Power / charging --- */
#define PIN_QI_DET            0, 28  /* P0.28 — Qi charge detect */
#define PIN_USB_DET           0, 29  /* P0.29 — USB-C VBUS detect */
#define PIN_BAT_MON           0, 30  /* P0.30 — battery voltage monitor (SAADC) */
#define PIN_CHG_STAT           0, 31 /* P0.31 — nPM1300 charge status */

/* --- Capacitive touch zone (ring exterior) --- */
#define PIN_TOUCH_SENSE        1, 8  /* P1.08 — capacitive touch */
#define PIN_TOUCH_DRIVE        1, 9  /* P1.09 — touch drive */

/* --- Status LED --- */
#define PIN_LED_R             1, 10  /* P1.10 — red LED (charge/error) */
#define PIN_LED_G             1, 11  /* P1.11 — green LED (connected) */

/* ----------------------------------------------------------------
 * Actuator array geometry
 * ---------------------------------------------------------------- */
#define ACT_ROWS              4       /* 4 rows across fingertip width */
#define ACT_COLS              6       /* 6 rows along fingertip length */
#define ACT_COUNT             (ACT_ROWS * ACT_COLS) /* 24 elements */
#define ACT_REFRESH_HZ        200     /* tactile frame refresh rate */
#define ACT_REFRESH_US        (1000000 / ACT_REFRESH_HZ) /* 5000 µs */
#define ACT_DRIVE_WINDOW_US   500   /* HV gate on per sub-frame */
#define ACT_SUBFRAMES         4       /* 4 groups of 6 elements */

/* DRV2700 has 2 channels per IC; 12 channels via 6 ICs */
#define DRV2700_COUNT         6
#define DRV2700_CHANNELS      2

/* HV supply */
#define HV_TARGET_V           120     /* peak drive voltage */
#define HV_BOOST_WARMUP_US    100     /* boost converter warm-up time */

/* ----------------------------------------------------------------
 * IMU configuration (ICM-42688-P)
 * ---------------------------------------------------------------- */
#define IMU_ODR_HZ            1000    /* output data rate */
#define IMU_ACCEL_RANGE_G     8       /* ±8 g */
#define IMU_GYRO_RANGE_DPS    1000    /* ±1000 dps */
#define IMU_FIFO_WATERMARK    32      /* samples before interrupt */

/* Gesture thresholds */
#define TAP_THRESHOLD_MG     1500    /* 1.5 g tap detection */
#define TAP_DURATION_MS      80      /* max tap duration */
#define DOUBLE_TAP_GAP_MS    400     /* max gap between taps */
#define SWIPE_THRESHOLD_DPS  300     /* deg/s for swipe */
#define SWIPE_MIN_SAMPLES    20      /* min samples for valid swipe */

/* ----------------------------------------------------------------
 * OLED configuration (SSD1316, 64x32)
 * ---------------------------------------------------------------- */
#define OLED_WIDTH           64
#define OLED_HEIGHT          32
#define OLED_PAGES           (OLED_HEIGHT / 8)  /* 4 pages */

/* ----------------------------------------------------------------
 * Battery / power
 * ---------------------------------------------------------------- */
#define BAT_MAH              120      /* 120 mAh LiPo */
#define BAT_FULL_MV          4200
#define BAT_EMPTY_MV         3200
#define BAT_WARN_MV          3500     /* low battery warning */
#define BAT_CRIT_MV          3300     /* critical, force shutdown */
#define SAADC_BAT_DIV        3        /* VBAT/3 via resistor divider */

/* ----------------------------------------------------------------
 * BLE configuration
 * ---------------------------------------------------------------- */
#define BLE_DEVICE_NAME      "TactiScript"
#define BLE_CONN_INTERVAL_MIN_US  7500   /* 7.5 ms min interval */
#define BLE_CONN_INTERVAL_MAX_US  15000  /* 15 ms max interval */
#define BLE_SLAVE_LATENCY          4
#define BLE_CONN_TIMEOUT_MS        4000

/* GATT service UUIDs (custom, 128-bit) */
#define BLE_SVC_UUID_BASE { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11, \
                            0x42, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
/* Short UUIDs (replaces last two bytes) */
#define BLE_SVC_TEXT         0x0001   /* write: UTF-8 text stream */
#define BLE_SVC_CMD          0x0002   /* write: command byte */
#define BLE_SVC_STATUS       0x0003   /* notify: status */
#define BLE_SVC_TEXTURE      0x0004   /* write: raw 4x6 frame */
#define BLE_SVC_GESTURE      0x0005   /* notify: gesture event */
#define BLE_SVC_NAV          0x0006   /* write: nav direction byte */

/* Command bytes for BLE_SVC_CMD */
#define CMD_MODE_READER      0x01
#define CMD_MODE_NAV         0x02
#define CMD_MODE_MUSIC       0x03
#define CMD_MODE_TUTOR       0x04
#define CMD_MODE_TEXTURE     0x05
#define CMD_MODE_SLEEP       0x06
#define CMD_SPEED_UP         0x10
#define CMD_SPEED_DOWN       0x11
#define CMD_INTENSITY_UP     0x12
#define CMD_INTENSITY_DOWN   0x13
#define CMD_BRAILLE_GRADE1   0x20
#define CMD_BRAILLE_GRADE2   0x21
#define CMD_PAUSE            0x30
#define CMD_RESUME           0x31
#define CMD_CLEAR            0x40

/* ----------------------------------------------------------------
 * System tick
 * ---------------------------------------------------------------- */
#define SYSTICK_HZ           1000     /* 1 ms tick */

/* ----------------------------------------------------------------
 * Helper macros
 * ---------------------------------------------------------------- */
#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))
#define MIN(a,b)       ((a) < (b) ? (a) : (b))
#define MAX(a,b)       ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) (MAX((lo), MIN((hi), (x))))

#endif /* TACTISCRIPT_BOARD_H */