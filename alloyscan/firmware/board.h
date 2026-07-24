/*
 * board.h — Hardware pin assignments and peripheral configuration
 * for the AlloyScan multi-frequency eddy current alloy identifier.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Target MCU: STM32G474CEU6 (UFBGA-169 / QFN-68 compatible pinout)
 * System Clock: 170 MHz HSI + PLL
 */

#ifndef ALLOYSCAN_BOARD_H
#define ALLOYSCAN_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Clock Configuration ---- */
#define SYSCLK_HZ           170000000UL
#define HSI_HZ              16000000UL
#define APB1_HZ             (SYSCLK_HZ / 1)
#define APB2_HZ             (SYSCLK_HZ / 1)
#define ADC_CLK_HZ          (SYSCLK_HZ / 4)

/* ---- Excitation Frequencies ---- */
#define FREQ_COUNT          4
extern const uint32_t excitation_freqs[FREQ_COUNT];
#define FREQ_1K             1000UL
#define FREQ_10K            10000UL
#define FREQ_100K           100000UL
#define FREQ_500K           500000UL
#define DDS_SAMPLE_RATE_HZ  2000000UL
#define DDS_TABLE_SIZE      2048
#define DAC_FULL_SCALE      4095

/* ---- ADC ---- */
#define ADC_SAMPLE_RATE_HZ  1000000UL
#define ADC_SAMPLE_COUNT     2000    /* 2 ms capture = 2000 samples at 1 MSPS */
#define ADC_RESOLUTION_BITS 16

/* ---- Measurement ---- */
#define SCAN_TIMEOUT_MS     200
#define CONFIDENCE_HIGH     0.70f
#define CONFIDENCE_MEDIUM   0.40f
#define MAX_ALTERNATIVES    3
#define KNN_K               5

/* ---- Pin Assignments (STM32G474 QFN-68) ---- */
/* DAC1 Channel 1 output -> PA2 (pin 14) */
#define DAC1_OUT1_PIN       2   /* GPIOA, AF12 (DAC1_OUT1) */
#define DAC1_OUT1_AF        12

/* SPI1: ADC interface (ADS8866) — PB3=CLK, PB4=CS, PB5=MISO */
#define SPI1_SCK_PIN        3   /* GPIOB, AF5 (SPI1_SCK) */
#define SPI1_MISO_PIN       4   /* GPIOB, AF5 (SPI1_MISO) */
#define SPI1_CS_PIN         5   /* GPIOB, output (GPIO) */
#define SPI1_AF             5

/* SPI2: OLED display (SH1106) — PB12=CS, PB13=SCK, PB14=MISO, PB15=MOSI */
#define OLED_CS_PIN         12  /* GPIOB, output */
#define OLED_SCK_PIN        13  /* GPIOB, AF5 (SPI2_SCK) */
#define OLED_MISO_PIN       14  /* GPIOB, AF5 (SPI2_MISO) */
#define OLED_MOSI_PIN       15   /* GPIOB, AF5 (SPI2_MOSI) */
#define OLED_DC_PIN         8   /* GPIOA, output (data/command) */
#define OLED_RST_PIN        9   /* GPIOA, output (reset) */
#define OLED_AF             5

/* SPI3: Flash (W25Q128) — PC10=CLK, PC11=MISO, PC12=MOSI, PA4=CS */
#define FLASH_SCK_PIN       10  /* GPIOC, AF6 (SPI3_SCK) */
#define FLASH_MISO_PIN      11  /* GPIOC, AF6 (SPI3_MISO) */
#define FLASH_MOSI_PIN      12  /* GPIOC, AF6 (SPI3_MOSI) */
#define FLASH_CS_PIN        4   /* GPIOA, output (GPIO) */
#define FLASH_AF            6

/* USART2: BLE module (BMD-300) — PA2=TX, PA3=RX (remapped) */
/* NOTE: PA2 used by DAC — BLE USART uses PA14=TX, PA15=RX on G474 alt */
#define BLE_TX_PIN          14  /* GPIOA, AF7 (USART2_TX) */
#define BLE_RX_PIN          15  /* GPIOA, AF7 (USART2_RX) */
#define BLE_AF              7
#define BLE_BAUD            115200UL

/* I2C3: VL53L0X time-of-flight — PA8=SCL, PA9=SDA */
#define I2C3_SCL_PIN        8   /* GPIOA, AF4 (I2C3_SCL) */
#define I2C3_SDA_PIN        9   /* GPIOA, AF4 (I2C3_SDA) */
#define I2C3_AF             4
#define I2C3_CLK_HZ         400000UL   /* Fast mode */
#define VL53L0X_ADDR        0x29

/* GPIO: trigger and buttons */
#define TRIGGER_PIN         10  /* GPIOC, input with pull-up */
#define BUTTON_UP_PIN       11  /* GPIOC, input with pull-up */
#define BUTTON_DOWN_PIN     12  /* GPIOC, input with pull-up */
#define BUTTON_OK_PIN       0   /* GPIOB, input with pull-up */

/* GPIO: LED indicators */
#define LED_GREEN_PIN       1   /* GPIOC, output (high confidence) */
#define LED_YELLOW_PIN      2   /* GPIOC, output (medium confidence) */
#define LED_RED_PIN         3   /* GPIOC, output (low confidence) */

/* GPIO: piezo buzzer (via MAX98357A SD_MODE pin) */
#define BUZZER_EN_PIN       6   /* GPIOB, output */

/* GPIO: power control */
#define AMP_ENABLE_PIN      7   /* GPIOB, output (THS3091 shutdown) */
#define BATTERY_ADC_PIN     1   /* GPIOA, analog input (ADC2_IN1) */

/* ---- Battery thresholds ---- */
#define BATTERY_FULL_MV     4200
#define BATTERY_LOW_MV      3400
#define BATTERY_CRIT_MV      3100
#define BATTERY_DIVIDER     2.0f   /* resistor divider on board */

/* ---- Flash sector for calibration + alloy database ---- */
#define FLASH_ALLOY_DB_ADDR 0x080000UL  /* 512 KB offset (sector 128+) */
#define FLASH_CAL_ADDR      0x080100UL
#define FLASH_CAL_SIZE      4096
#define FLASH_SCAN_LOG_ADDR 0x080200UL
#define FLASH_SCAN_LOG_SIZE (1024 * 1024) /* 1 MB for scan log */
#define MAX_SCANS_LOGGED   100000

/* ---- OLED dimensions ---- */
#define OLED_WIDTH         128
#define OLED_HEIGHT        64
#define OLED_PAGES         (OLED_HEIGHT / 8)

/* ---- Utility macros ---- */
#define ARRAY_SIZE(a)      (sizeof(a) / sizeof((a)[0]))
#define CLAMP(x, lo, hi)   ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#define DEG_TO_RAD(d)      ((d) * 0.01745329252f)
#define RAD_TO_DEG(r)      ((r) * 57.2957795131f)

#endif /* ALLOYSCAN_BOARD_H */