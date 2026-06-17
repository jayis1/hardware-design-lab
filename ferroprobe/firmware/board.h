/*
 * board.h — FerroProbe Board Configuration & Pin Assignments
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Pin assignments, clock configuration, and physical constants for the
 * FerroProbe PCB (120mm × 72mm).  All pin definitions correspond to the
 * KiCad schematic in kicad/device.kicad_sch.
 */

#ifndef FERROPROBE_BOARD_H
#define FERROPROBE_BOARD_H

#include <stdint.h>
#include "registers.h"

/* ===================================================================== */
/*  Clock configuration                                                   */
/* ===================================================================== */

/* HSI 64 MHz → PLL1 → SYSCLK 480 MHz
 * PLL1: M=4, N=240, P=2 → VCO = 64/4*240 = 3840 MHz, SYSCLK = 3840/2 = 1920?
 * Correction: HSE 8 MHz → M=1, N=120, P=2 → 480 MHz (using external crystal)
 * On FerroProbe, we use HSI64 → M=32, N=240, P=2 → 64/32=2MHz, 2*240=480MHz, /2=240MHz
 * Actually for 480 MHz: HSI 64 / M(8) = 8 MHz ref, × N(60) = 480 MHz, / P(1) = 480 MHz
 * Let's use the documented H7 SDK approach:
 *   PLL1 source = HSI64, M=8, N=120, P=2, Q=4, R=2
 *   → ref = 64/8 = 8 MHz, VCO = 8×120 = 960 MHz, P=2 → 480 MHz SYSCLK
 */
#define BOARD_HSI_FREQ_HZ     64000000ULL
#define BOARD_SYSCLK_FREQ_HZ   480000000ULL
#define BOARD_HCLK_FREQ_HZ     240000000ULL   /* D1 */
#define BOARD_APB1_FREQ_HZ     120000000ULL   /* D2 APB1 (TIM2, UART4) */
#define BOARD_APB2_FREQ_HZ     120000000ULL   /* D2 APB2 (SPI1, SPI2, TIM1, USART1) */
#define BOARD_APB4_FREQ_HZ     120000000ULL   /* D3 APB4 (I2C1, SYSCFG) */

/* PLL1 configuration */
#define BOARD_PLL1_M           8
#define BOARD_PLL1_N           120
#define BOARD_PLL1_P           2   /* SYSCLK divider */
#define BOARD_PLL1_Q           4   /* USB/SDMMC1 clock = 480/4 = 120 MHz */
#define BOARD_PLL1_R           2   /* Spare */

/* ===================================================================== */
/*  TIM1 excitation PWM — 15.625 kHz center-aligned                      */
/* ===================================================================== */

/* TIM1CLK = APB2 × 2 = 240 MHz when APB2 prescaler ≠ 1
 * APB2 prescaler = 2 → TIM1CLK = 120 × 2 = 240 MHz
 * For center-aligned mode 3: period = 2 × ARR
 * 15.625 kHz → center freq → ARR = 240MHz / (2 × 15.625kHz) = 7680
 */
#define BOARD_TIM1_CLK_HZ      240000000ULL
#define BOARD_EXCIT_FREQ_HZ    15625U   /* Fluxgate excitation */
#define BOARD_2F0_FREQ_HZ      31250U   /* Second harmonic reference */
#define BOARD_TIM1_ARR         (BOARD_TIM1_CLK_HZ / (2ULL * BOARD_EXCIT_FREQ_HZ))
#define BOARD_TIM1_PSC         0U
/* 50% duty → CCR = ARR/2 */
#define BOARD_TIM1_CCR         (BOARD_TIM1_ARR / 2)

/* ===================================================================== */
/*  TIM2 — ADC sampling trigger at 30 kSPS, sync'd to 2f₀                */
/* ===================================================================== */

/* TIM2CLK = APB1 × 2 = 240 MHz
 * 30 kSPS → period = 240MHz / 30000 = 8000
 */
#define BOARD_TIM2_CLK_HZ      240000000ULL
#define BOARD_ADC_RATE_HZ      30000U   /* ADS1256 configured for 30 kSPS */
#define BOARD_TIM2_ARR         8000U
#define BOARD_TIM2_PSC         0U

/* ===================================================================== */
/*  Pin assignments (STM32H743VIT6, 100-pin LQFP)                        */
/* ===================================================================== */

/* SPI1 — ADS1256 24-bit ADC + microSD (chip-select-multiplexed)
 *   PA5  → SPI1_SCK   (AF5)
 *   PA6  → SPI1_MISO  (AF5)
 *   PA7  → SPI1_MOSI  (AF5)
 *   PD14 → ADS1256_CS (GPIO output, active low)
 *   PD15 → ADS1256_DRDY (GPIO input, falling edge interrupt)
 *   PD0  → ADS1256_SYNC/RESET (GPIO output)
 *   PD1  → SD_CS      (GPIO output, active low)
 */
#define ADS_CS_PORT       GPIOD_BASE
#define ADS_CS_PIN        14
#define ADS_DRDY_PORT     GPIOD_BASE
#define ADS_DRDY_PIN      15
#define ADS_SYNC_PORT     GPIOD_BASE
#define ADS_SYNC_PIN      0
#define SD_CS_PORT        GPIOD_BASE
#define SD_CS_PIN         1

/* SPI2 — DAC8568 feedback nulling DAC
 *   PB10 → SPI2_SCK   (AF5)
 *   PC2  → SPI2_MOSI  (AF5)
 *   PC3  → SPI2_MISO  (AF5, unused but configured)
 *   PD4  → DAC8568_CS (GPIO output, active low)
 *   PD5  → DAC8568_LDAC (GPIO output, load DAC, active low)
 */
#define DAC_CS_PORT       GPIOD_BASE
#define DAC_CS_PIN        4
#define DAC_LDAC_PORT     GPIOD_BASE
#define DAC_LDAC_PIN      5

/* I2C1 — BMI270 IMU + SSD1306 OLED
 *   PB6  → I2C1_SCL (AF4)
 *   PB7  → I2C1_SDA (AF4)
 */
#define I2C1_SCL_PORT     GPIOB_BASE
#define I2C1_SCL_PIN      6
#define I2C1_SDA_PORT     GPIOB_BASE
#define I2C1_SDA_PIN      7
#define I2C1_TIMINGR      0x307075B1U   /* 400 kHz at 120 MHz PCLK */

/* USART1 — NEO-M9N GPS (NMEA @ 38400 baud)
 *   PA9  → USART1_TX (AF7)
 *   PA10 → USART1_RX (AF7)
 *   Baud = 38400 → BRR = 120MHz / 38400 = 3125
 */
#define GPS_USART_BASE    USART1_BASE
#define GPS_BRR_VAL       (BOARD_APB2_FREQ_HZ / 38400U)  /* 3125 */

/* UART4 — nRF52840 BLE module (1 Mbps, hardware flow control)
 *   PA0  → UART4_TX (AF8)
 *   PA1  → UART4_RX (AF8)
 *   PA2  → UART4_RTS (AF8)
 *   PA3  → UART4_CTS (AF8)
 */
#define BLE_USART_BASE    UART4_BASE
#define BLE_BRR_VAL       (BOARD_APB1_FREQ_HZ / 1000000U)  /* 120 */

/* TIM1 — Fluxgate excitation PWM (3 complementary pairs for H-bridge)
 *   PE9  → TIM1_CH1  (AF1)  → H-bridge A high (X-axis drive)
 *   PE8  → TIM1_CH1N (AF1)  → H-bridge A low
 *   PE11 → TIM1_CH2  (AF1)  → H-bridge B high
 *   PE10 → TIM1_CH2N (AF1)  → H-bridge B low
 *   PE13 → TIM1_CH3  (AF1)  → Z-axis drive (parallel to CH1 for 3rd axis)
 *   PE12 → TIM1_CH3N (AF1)  → Z-axis drive complement
 */
#define EXC_PWM_PORT      GPIOE_BASE

/* Status indicators
 *   PD8  → RGB_LED_R (GPIO output, PWM via TIM4)
 *   PD9  → RGB_LED_G (GPIO output, PWM via TIM4)
 *   PD10 → RGB_LED_B (GPIO output, PWM via TIM4)
 *   PD11 → BUZZER    (GPIO output, TIM3 PWM)
 *   PD12 → BUTTON_MODE (GPIO input, pull-up, EXTI falling)
 *   PD13 → BUTTON_START (GPIO input, pull-up, EXTI falling)
 *   PC13 → USB_DETECT (GPIO input)
 *   PC14 → BATTERY_SENSE (ADC analog input, internal ADC)
 *   PC15 → CHARGE_STAT (GPIO input from MCP73871)
 */
#define LED_R_PORT        GPIOD_BASE
#define LED_R_PIN         8
#define LED_G_PORT        GPIOD_BASE
#define LED_G_PIN         9
#define LED_B_PORT        GPIOD_BASE
#define LED_B_PIN         10
#define BUZZER_PORT       GPIOD_BASE
#define BUZZER_PIN        11
#define BTN_MODE_PORT     GPIOD_BASE
#define BTN_MODE_PIN      12
#define BTN_START_PORT    GPIOD_BASE
#define BTN_START_PIN     13
#define USB_DETECT_PORT   GPIOC_BASE
#define USB_DETECT_PIN    13
#define CHARGE_STAT_PORT  GPIOC_BASE
#define CHARGE_STAT_PIN   15

/* ===================================================================== */
/*  Physical / sensor constants                                           */
/* ===================================================================== */

/* Fluxgate: ADC counts → nanotesla conversion
 * The ADS1256 at PGA=1, Vref=2.5V has LSB = 2.5V / 2^23 = 0.298 µV
 * After preamp gain of 100 and lock-in, field sensitivity is
 * approximately 0.5 µV/nT at the sense coil → 50 µV/nT after preamp
 * → 50 µV / 0.298 µV = 167.8 counts/nT
 * This is calibrated per-device via the ellipsoid calibration.
 */
#define BOARD_ADC_LSB_UV   0.2984f       /* µV per count at PGA=1 */
#define BOARD_PREAMP_GAIN  100.0f
#define BOARD_SENSE_GAIN   (BOARD_PREAMP_GAIN / 0.5f)  /* counts per nT */
#define BOARD_ADC_VREF_MV  2500.0f

/* Earth's field range: ±65 µT typical (±100 µT measurement range) */
#define BOARD_FIELD_RANGE_UT  100.0f
#define BOARD_FIELD_RANGE_NT  (BOARD_FIELD_RANGE_UT * 1000.0f)

/* Temperature sensor (LM35 → 10 mV/°C, on ADS1256 AIN3) */
#define BOARD_TEMP_GAIN_MV_PER_C  10.0f
#define BOARD_TEMP_OFFSET_MV      0.0f

/* ===================================================================== */
/*  Data logging format                                                    */
/* ===================================================================== */

/* Binary log record: 32 bytes per sample */
#define LOG_RECORD_SIZE   32U

/* Record structure (packed, little-endian):
 *  [0:3]   uint32  timestamp_ms (system tick, ms since boot)
 *  [4:7]   int32   latitude_e7 (×10^7, e.g., 476064000 = 47.6064°)
 *  [8:11]  int32   longitude_e7
 *  [12:15] int32   altitude_mm (millimeters above WGS84 ellipsoid)
 *  [16:19] int32   Bx_nt (calibrated, nanotesla × 10 = 0.1 nT LSB)
 *  [20:23] int32   By_nt
 *  [24:27] int32   Bz_nt
 *  [28]    uint8   roll_deg (±90, 1° resolution)
 *  [29]    uint8   pitch_deg (±90, 1° resolution)
 *  [30]    uint8   heading_deg (0-360, 1° resolution)
 *  [31]    uint8   flags (bit0: GPS fix, bit1: anomaly, bit2: calib valid)
 */

/* ===================================================================== */
/*  BLE protocol framing                                                  */
/* ===================================================================== */

#define BLE_FRAME_SOF     0xAAU   /* Start of frame */
#define BLE_FRAME_EOF     0x55U   /* End of frame */
#define BLE_MAX_PAYLOAD    128U   /* Max payload per frame */
#define BLE_MTU           251U    /* BLE ATT MTU (negotiated up) */

/* BLE command opcodes */
#define BLE_CMD_PING          0x01U
#define BLE_CMD_GET_STATUS    0x02U
#define BLE_CMD_GET_FIELD     0x03U
#define BLE_CMD_START_SURVEY  0x04U
#define BLE_CMD_STOP_SURVEY   0x05U
#define BLE_CMD_START_CALIB   0x06U
#define BLE_CMD_GET_CALIB     0x07U
#define BLE_CMD_SET_RATE      0x08U
#define BLE_CMD_GET_RATE      0x09U
#define BLE_CMD_SET_THRESHOLD 0x0AU
#define BLE_CMD_GET_THRESHOLD 0x0BU
#define BLE_CMD_DOWNLOAD_LOG  0x0CU
#define BLE_CMD_ERASE_LOG     0x0DU
#define BLE_CMD_SET_TIME      0x0EU
#define BLE_CMD_GET_VERSION   0x0FU
#define BLE_CMD_STREAM_START  0x10U
#define BLE_CMD_STREAM_STOP   0x11U

/* BLE response codes */
#define BLE_RSP_OK            0x80U
#define BLE_RSP_ERROR          0x81U
#define BLE_RSP_DATA           0x82U
#define BLE_RSP_STREAM         0x83U
#define BLE_RSP_LOG_CHUNK      0x84U
#define BLE_RSP_CALIB_DATA     0x85U
#define BLE_RSP_STATUS         0x86U
#define BLE_RSP_VERSION        0x87U

/* ===================================================================== */
/*  Firmware version                                                      */
/* ===================================================================== */

#define FW_VERSION_MAJOR   1
#define FW_VERSION_MINOR   0
#define FW_VERSION_PATCH   0
#define FW_VERSION_STRING  "1.0.0"
#define FW_BUILD_DATE      "2026-06-17"
#define FW_AUTHOR          "jayis1"

/* ===================================================================== */
/*  Operating modes (state machine)                                      */
/* ===================================================================== */

typedef enum {
    MODE_BOOT = 0,
    MODE_IDLE,
    MODE_SURVEY,     /* GPS-tagged walking survey */
    MODE_CALIB,      /* 3-axis ellipsoid calibration */
    MODE_MONITOR,   /* Static real-time monitoring */
    MODE_CONFIG,    /* BLE configuration / data download */
    MODE_SHUTDOWN,
} ferro_mode_t;

/* ===================================================================== */
/*  Inline helper: atomic GPIO set/reset                                  */
/* ===================================================================== */

static inline void gpio_set(uint32_t port, uint8_t pin)
{
    GPIO_REG(port, GPIO_BSRR) = (1U << pin);
}

static inline void gpio_reset(uint32_t port, uint8_t pin)
{
    GPIO_REG(port, GPIO_BSRR) = (1U << (pin + 16));
}

static inline uint8_t gpio_read(uint32_t port, uint8_t pin)
{
    return (GPIO_REG(port, GPIO_IDR) >> pin) & 1U;
}

static inline void delay_us(uint32_t us)
{
    /* Uses DWT cycle counter: 480 MHz → 480 cycles/µs */
    uint32_t start = DWT_CYCCNT;
    uint32_t cycles = us * (BOARD_SYSCLK_FREQ_HZ / 1000000U);
    while ((DWT_CYCCNT - start) < cycles)
        ;
}

static inline void delay_ms(uint32_t ms)
{
    delay_us(ms * 1000U);
}

#endif /* FERROPROBE_BOARD_H */