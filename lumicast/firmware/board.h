/*
 * board.h — LumiCast Hardware Board Configuration
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Pin assignments, peripheral mapping, sensor constants and timing
 * parameters for the LumiCast rev A PCB built around the STM32WB55CGU6.
 */

#ifndef LUMICAST_BOARD_H
#define LUMICAST_BOARD_H

#include <stdint.h>
#include <stddef.h>
#include "registers.h"

/* ===================================================================== */
/*  MCU / System identity                                                 */
/* ===================================================================== */

#define BOARD_NAME      "LumiCast rev A"
#define FW_VERSION_STR  "1.0.0"
#define FW_VERSION_BCD  0x0100
#define AUTHOR          "jayis1"

#define HSE_VALUE       32000000UL   /* 32 MHz HSE crystal */
#define HSI_VALUE       16000000UL   /* 16 MHz HSI */
#define LSE_VALUE       32768UL       /* 32.768 kHz LSE for RTC */
#define SYSTEM_CLK_HZ   64000000UL   /* 64 MHz SYSCLK (PLL) */

/* ===================================================================== */
/*  Pin map (STM32WB55CGU6 UFQFPN48)                                     */
/*  ----------------------------------------------------------------    */
/*  PA0  — ADC1_IN1   : Photodiode PD0 (TSL2572 alternate flicker pd)   */
/*  PA2  — LPUART1_TX : Debug console                                   */
/*  PA3  — LPUART1_RX : Debug console                                   */
/*  PA4  — GPIO OUT   : Sensor LED enable (CAL indicator)               */
/*  PA5  — SPI1_SCK   : microSD card                                    */
/*  PA6  — SPI1_MISO  : microSD card                                    */
/*  PA7  — SPI1_MOSI  : microSD card                                    */
/*  PA8  — I2C3_SCL   : AS7343 spectrometer (400 kHz)                   */
/*  PA9  — I2C3_SDA   : AS7343 spectrometer                              */
/*  PA10 — GPIO OUT   : microSD CS (active low)                          */
/*  PA11 — USB DM     : USB CDC (debug)                                  */
/*  PA12 — USB DP     : USB CDC (debug)                                  */
/*  PB0  — GPIO OUT   : OLED reset                                      */
/*  PB1  — GPIO OUT   : AS7343 LED enable (CAL source)                  */
/*  PB5  — TIM17_CH1  : OLED backlight PWM                              */
/*  PB6  — I2C1_SCL   : SSD1306 OLED display                            */
/*  PB7  — I2C1_SDA   : SSD1306 OLED display                            */
/*  PB8  — GPIO OUT   : VBUS detect pullup enable                       */
/*  PB9  — TIM16_CH1  : Sensor integration gate (not used in rev A)     */
/*  PC0  — ADC1_IN2   : Photodiode PD1 (secondary flicker pd)           */
/*  PC1  — ADC1_IN3   : Thermistor for sensor temperature               */
/*  PC13 — GPIO IN    : User button (active low)                        */
/*  PH3  — GPIO OUT   : BOOT0 strap (software override)                  */
/* ===================================================================== */

#define PIN_PD0_ADC_CH        1U
#define PIN_PD1_ADC_CH        3U
#define PIN_THERM_ADC_CH      11U

/* AS7343 I2C address: 0x39 (7-bit, SCL pulled low by default) */
#define AS7343_I2C_ADDR       0x39U
#define AS7343_I2C_ADDR_8BIT  (AS7343_I2C_ADDR << 1)

/* SSD1306 OLED I2C address: 0x3C */
#define SSD1306_I2C_ADDR      0x3CU
#define SSD1306_I2C_ADDR_8BIT (SSD1306_I2C_ADDR << 1)

/* microSD CS pin */
#define SD_CS_PORT            GPIOA
#define SD_CS_PIN             10U
#define SD_CS_LOW()           (SD_CS_PORT->BSRR = GPIO_PIN_RST(SD_CS_PIN))
#define SD_CS_HIGH()          (SD_CS_PORT->BSRR = GPIO_PIN_SET(SD_CS_PIN))

/* Sensor LED enable (PB1) */
#define SENSOR_LED_PORT       GPIOB
#define SENSOR_LED_PIN        1U
#define SENSOR_LED_ON()       (SENSOR_LED_PORT->BSRR = GPIO_PIN_SET(SENSOR_LED_PIN))
#define SENSOR_LED_OFF()      (SENSOR_LED_PORT->BSRR = GPIO_PIN_RST(SENSOR_LED_PIN))

/* OLED reset (PB0) */
#define OLED_RST_PORT         GPIOB
#define OLED_RST_PIN          0U
#define OLED_RST_LOW()        (OLED_RST_PORT->BSRR = GPIO_PIN_RST(OLED_RST_PIN))
#define OLED_RST_HIGH()       (OLED_RST_PORT->BSRR = GPIO_PIN_SET(OLED_RST_PIN))

/* User button (PC13) */
#define BTN_PORT              GPIOC
#define BTN_PIN               13U
#define BTN_PRESSED()         ((BTN_PORT->IDR & GPIO_PIN_SET(BTN_PIN)) == 0U)

/* ===================================================================== */
/*  AS7343 spectral channel mapping                                      */
/*  ----------------------------------------------------------------    */
/*  The AMS AS7343 has 11 visible/NIR channels plus a clear + flicker    */
/*  detector.  Channel centroid wavelengths (nm):                        */
/*    F1 415nm  F2 445nm  F3 480nm  F4 515nm  F5 555nm                  */
/*    F6 590nm  F7 630nm  F8 680nm  NIR 910nm  CLR (broadband)           */
/*    FLK (flicker)                                                      */
/* ===================================================================== */

#define AS7343_NUM_CHANNELS   11U

#define AS7343_REG_WHOAMI    0x60U
#define AS7343_REG_ASTATUS   0x60U
#define AS7343_REG_CFG0      0xAAU
#define AS7343_REG_CFG1      0xABU
#define AS7343_REG_CFG6      0xAFU
#define AS7343_REG_CFG9      0xB2U
#define AS7343_REG_CFG10     0xB3U
#define AS7343_REG_CFG12     0xB5U
#define AS7343_REG_ENABLE    0x80U
#define AS7343_REG_ATIME     0x81U
#define AS7343_REG_WTIME     0x83U
#define AS7343_REG_AUXID     0x84U
#define AS7343_REG_ID        0x92U
#define AS7343_REG_STATUS    0x93U
#define AS7343_REG_CH0_DATAH 0x95U
#define AS7343_REG_CH0_DATAL 0x96U

#define AS7343_WHOAMI_VAL    0x19U   /* AS7343 ID */
#define AS7343_CHIPID        0x09U

#define AS7343_ENABLE_SM_EN  (1U << 1)  /* spectral measurement enable */
#define AS7343_ENABLE_SP_EN  (1U << 4)  /* spectral power */
#define AS7343_ENABLE_FD_EN (1U << 5)  /* flicker detection */
#define AS7343_ENABLE_PON    (1U << 0)  /* power on */

#define AS7343_CFG0_ASLP_SHUT (1U << 3)

#define AS7343_CFG1_SMUX_RD   (1U << 3)
#define AS7343_CFG1_SMUX_EN   (1U << 2)

#define AS7343_CFG10_FD_D1   0x20U  /* flicker threshold 1 */
#define AS7343_CFG10_FD_D2   0x10U  /* flicker threshold 2 */
#define AS7343_CFG10_FD_GAIN  0x0CU  /* flicker gain mask */

/* Channel centroid wavelengths (nm) — used for spectral interpolation */
extern const float as7343_wl_nm[AS7343_NUM_CHANNELS];

/* Normalized spectral responsivity table for melanopic correction.      */
/* These are pre-computed sm(λ)·λ responsivity weighting factors used    */
/* to compute melanopic EDI per CIE S 026. Index aligns to channels F1..NIR */
extern const float as7343_melanopic_w[AS7343_NUM_CHANNELS];

/* Same weighting for photopic (V(λ)) response — gives illuminance.     */
extern const float as7343_photopic_w[AS7343_NUM_CHANNELS];

/* Same weighting for cyanopic (S-cone melanopsin-adjacent) response.    */
extern const float as7343_cyanopic_w[AS7343_NUM_CHANNELS];

/* CIE illuminant D65 SPD normalization (for Duv calculation).           */
#define CIE_D65_REF_X  95.047f
#define CIE_D65_REF_Y  100.000f
#define CIE_D65_REF_Z  108.883f

/* ===================================================================== */
/*  Timing & sampling                                                     */
/* ===================================================================== */

/* Spectral sampling interval — once per 2 seconds default. */
#define SAMPLE_INTERVAL_MS    2000UL

/* Flicker sampling: photodiode sampled at 4 kHz to cover up to 2 kHz   */
/* (well past the 120 Hz LED PWM / 100 Hz mains fundamental).            */
#define FLICKER_SAMPLE_HZ     4000UL
#define FLICKER_BUF_LEN       512U   /* 128 ms window @ 4 kHz */
#define FLICKER_FFT_LEN       512U   /* power of 2 for radix-2 FFT */

/* System tick — 1 ms. */
#define SYSTICK_HZ            1000UL
#define SYSTICK_RELOAD        (SYSTEM_CLK_HZ / SYSTICK_HZ - 1UL)

/* ===================================================================== */
/*  Logging                                                              */
/* ===================================================================== */

#define LOG_MAGIC            0x4C434C31UL  /* "LCL1" */
#define LOG_VERSION          1U
#define LOG_RECORD_SIZE      128U   /* fixed-size records on SD */
#define LOG_MAX_FILE_BYTES   (4000000UL)   /* rotate at 4 MB */

/* On-device ring buffer for short-term trend (last 256 samples).       */
#define TREND_BUF_LEN        256U

/* ===================================================================== */
/*  BLE                                                                   */
/* ===================================================================== */

#define BLE_MTU              247U
#define BLE_MAX_PAYLOAD       180U
#define BLE_SERVICE_UUID      0xFC01U  /* custom service UUID */
#define BLE_ADV_INTERVAL_MS  200UL

/* ===================================================================== */
/*  Power                                                                */
/* ===================================================================== */

#define BATTERY_ADC_CH       0U    /* VBAT via divider on PA0 alt */
#define BATTERY_FULL_MV      4200
#define BATTERY_EMPTY_MV     3200
#define USB_VBUS_GPIO         GPIOB
#define USB_VBUS_PIN          8U

#endif /* LUMICAST_BOARD_H */