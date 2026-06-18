/*
 * board.h - Pin map and peripheral configuration for Pollen Scout
 * Hardware platform: STM32H743VIT6 custom board
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef POLLEN_SCOUT_BOARD_H
#define POLLEN_SCOUT_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Clocking ---- */
#define HSE_VALUE           16000000U   /* 16 MHz external xtal on X3 */
#define LSE_VALUE           32768U      /* 32.768 kHz RTC xtal */
#define SYSTEM_CLOCK        480000000U  /* PLL1 -> 480 MHz core */
#define APB1_CLOCK          120000000U
#define APB2_CLOCK          240000000U
#define APB4_CLOCK          120000000U

/* ---- LED / status indicators ---- */
#define LED_STATUS_PORT     GPIOB
#define LED_STATUS_PIN      0           /* green */
#define LED_ERROR_PORT      GPIOB
#define LED_ERROR_PIN       1           /* red */

/* ---- OV5640 camera (DCMI peripheral) ---- */
#define OV5640_PWDN_PORT    GPIOC
#define OV5640_PWDN_PIN     4
#define OV5640_RESET_PORT   GPIOC
#define OV5640_RESET_PIN    5
/* SCCB (I2C-like) on I2C4 */
#define OV5640_SCCB_I2C     I2C4
#define OV5640_I2C_ADDR     (0x60 << 1)
/* DCMI data bus: PC6..PC13 (8-bit) */
#define DCMI_VSYNC_PIN      GPIO_PIN_7   /* PA7 */
#define DCMI_HSYNC_PIN      GPIO_PIN_4   /* PA4 */
#define DCMI_PCLK_PIN       GPIO_PIN_6   /* PA6 */

/* ---- Strobe LEDs ---- */
#define STROBE_WHITE_PORT   GPIOD
#define STROBE_WHITE_PIN    12           /* TIM4_CH1 PWM */
#define STROBE_UV_PORT      GPIOD
#define STROBE_UV_PIN       13           /* TIM4_CH2 PWM */
#define STROBE_TIM          TIM4
#define STROBE_PULSE_US     20U

/* ---- BLDC micro-blower PWM ---- */
#define BLOWER_PWM_PORT     GPIOE
#define BLOWER_PWM_PIN      5            /* TIM1_CH2 */
#define BLOWER_PWM_TIM      TIM1
#define BLOWER_RPM_PORT     GPIOE
#define BLOWER_RPM_PIN      6            /* TIM3_CH1 tach */

/* ---- SDP810 differential pressure (flow feedback) ---- */
#define SDP810_I2C          I2C1
#define SDP810_I2C_ADDR     (0x25 << 1)

/* ---- BME280 T/RH/P ---- */
#define BME280_I2C          I2C1
#define BME280_I2C_ADDR     (0x76 << 1)

/* ---- SI1145 UV index ---- */
#define SI1145_I2C          I2C1
#define SI1145_I2C_ADDR     (0x60 << 1)

/* ---- PMS5003 particulate (UART) ---- */
#define PMS5003_UART        USART3
#define PMS5003_BAUD        9600U
#define PMS5003_TX_PIN      GPIO_PIN_8   /* PD8 */
#define PMS5003_RX_PIN      GPIO_PIN_9   /* PD9 */

/* ---- Wind sensor RS-485 (UART + DE/RE) ---- */
#define WIND_UART           UART4
#define WIND_BAUD           19200U
#define WIND_DE_PORT        GPIOD
#define WIND_DE_PIN         10

/* ---- SX1262 LoRa radio (SPI1) ---- */
#define SX1262_SPI          SPI1
#define SX1262_CS_PORT      GPIOA
#define SX1262_CS_PIN       4
#define SX1262_RST_PORT     GPIOB
#define SX1262_RST_PIN      3
#define SX1262_BUSY_PORT    GPIOB
#define SX1262_BUSY_PIN     4
#define SX1262_DIO1_PORT    GPIOC
#define SX1262_DIO1_PIN     0
#define SX1262_ANT_SWITCH   GPIOC, 1     /* RFI / RFO switch */

/* ---- ESP32-C3 co-MCU (UART6 + reset/boot) ---- */
#define ESP32_UART          USART6
#define ESP32_BAUD          115200U
#define ESP32_RST_PORT      GPIOC
#define ESP32_RST_PIN       2
#define ESP32_BOOT_PORT     GPIOC
#define ESP32_BOOT_PIN      3

/* ---- microSD (SDMMC1, 4-bit) ---- */
#define SD_SDMMC            SDMMC1
#define SD_DETECT_PORT      GPIOC
#define SD_DETECT_PIN       13           /* active low */

/* ---- BQ25895 charger (I2C3) ---- */
#define BQ25895_I2C         I2C3
#define BQ25895_I2C_ADDR    (0x6B << 1)
#define BQ_INT_PORT         GPIOF
#define BQ_INT_PIN          2
#define BAT_NTC_ADC         ADC1
#define BAT_NTC_CHANNEL     5
#define SOLAR_V_ADC         ADC1
#define SOLAR_V_CHANNEL     6
#define BAT_V_ADC           ADC1
#define BAT_V_CHANNEL       7

/* ---- RTC ---- */
#define RTC_INSTANCE        RTC

/* ---- Timings (ms) ---- */
#define CAPTURE_PERIOD_MS       60000U
#define WEATHER_PERIOD_MS       2000U
#define FORECAST_PERIOD_MS      600000U
#define FLOW_PERIOD_MS          100U
#define POWER_PERIOD_MS         5000U
#define TELEMETRY_PERIOD_MS     600000U   /* 10 min uplink */

/* ---- LoRaWAN region ---- */
typedef enum {
    LORAWAN_REGION_EU868 = 0,
    LORAWAN_REGION_US915,
    LORAWAN_REGION_AS923,
} lorawan_region_t;

#define LORAWAN_REGION_DEFAULT  LORAWAN_REGION_EU868
#define LORAWAN_DEV_EUI_LEN     8
#define LORAWAN_JOIN_EUI_LEN    8
#define LORAWAN_APP_KEY_LEN     16

/* ---- Particle ROI / image ---- */
#define IMG_WIDTH       1280
#define IMG_HEIGHT      960
#define IMG_BPP_BYTES   2       /* RGB565 binned */
#define ROI_MAX         64
#define ROI_MAX_DIM     96      /* pixels */
#define ROI_MIN_DIM     8
#define TAXA_CLASSES    40

/* ---- Battery thresholds (mV, 2S LiFePO4) ---- */
#define BAT_FULL_MV         6600U
#define BAT_LOW_MV          5800U
#define BAT_CRITICAL_MV     5400U

/* ---- Board-specific helpers ---- */
#define BIT_SET(reg, bit)     ((reg) |= (1U << (bit)))
#define BIT_CLR(reg, bit)     ((reg) &= ~(1U << (bit)))
#define BIT_READ(reg, bit)    (((reg) >> (bit)) & 1U)

#endif /* POLLEN_SCOUT_BOARD_H */