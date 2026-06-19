/*
 * board.h — StrataScan Board Configuration & Pin Assignments
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Pin assignments, clock configuration, and physical constants for the
 * StrataScan PCB (180mm × 120mm).  All pin definitions correspond to the
 * KiCad schematic in kicad/device.kicad_sch.
 *
 * STM32H743VIT6 (LQFP-100) pin map:
 *
 *   PA0  — TIM2_CH1  (not used / spare ADC)
 *   PA1  — TIM2_CH2  (not used / spare ADC)
 *   PA2  — USART2_TX (RS-485 for multi-unit array sync)
 *   PA3  — USART2_RX
 *   PA4  — DAC1_OUT1 (VCO coarse tune bias)
 *   PA5  — DAC1_OUT2 (LNA gain control)
 *   PA6  — SPI3_MISO (ADS131M08 MISO via level shifter)
 *   PA7  — SPI3_MOSI (ADS131M08 MOSI)
 *   PA8  — GPIO     (PLL_LE — latch enable for ADF4159)
 *   PA9  — USART1_TX (SAM-M10Q GNSS)
 *   PA10 — USART1_RX
 *   PA11 — USB_OTG_FS_DM
 *   PA12 — USB_OTG_FS_DP
 *   PA15 — TIM2_ETR (ext trigger input / spare)
 *   PB0  — GPIO     (PLL_CE — chip enable ADF4159)
 *   PB1  — GPIO     (VCO_SEL0 — antenna/matching select bit 0)
 *   PB2  — GPIO     (VCO_SEL1 — antenna/matching select bit 1)
 *   PB3  — SPI1_SCK  (ADF4159 SPI clock)
 *   PB4  — SPI1_MISO (ADF4159 MISO — readback)
 *   PB5  — SPI1_MOSI (ADF4159 MOSI)
 *   PB6  — I2C1_SCL  (BMI270 IMU + SSD1309 OLED)
 *   PB7  — I2C1_SDA
 *   PB8  — GPIO     (ADC_nCS — ADS131M08 chip select)
 *   PB9  — GPIO     (ADC_nRST — ADS131M08 reset)
 *   PB10 — GPIO     (ADC_DRDY — ADS131M08 data-ready EXTI10)
 *   PB12 — GPIO     (SD_nCS — MicroSD SPI CS, alternate function)
 *   PB13 — SPI2_SCK (MicroSD — if SDMMC1 not used)
 *   PB14 — SPI2_MISO
 *   PB15 — SPI2_MOSI
 *   PC0  — ADC1_IN10 (battery voltage monitor)
 *   PC1  — ADC1_IN11 (temperature sensor — MCU internal, via channel 18)
 *   PC4  — GPIO     (LNA_EN — LNA power enable)
 *   PC5  — GPIO     (PLL_RF_EN — PLL RF output enable)
 *   PC6  — TIM3_CH1  (Survey wheel encoder A)
 *   PC7  — TIM3_CH2  (Survey wheel encoder B)
 *   PC8  — SDMMC1_D0 (MicroSD 4-bit mode data 0)
 *   PC9  — SDMMC1_D1
 *   PC10 — SDMMC1_D2
 *   PC11 — SDMMC1_D3
 *   PC12 — SDMMC1_CLK
 *   PD2  — SDMMC1_CMD
 *   PC13 — GPIO     (STATUS_LED_R)
 *   PC14 — GPIO     (STATUS_LED_G)
 *   PC15 — GPIO     (STATUS_LED_B)
 *   PD0  — GPIO     (nRF52840_nRST)
 *   PD1  — GPIO     (nRF52840_INT — BLE event interrupt)
 *   PD3  — GPIO     (PWR_EN — main 3V3 regulator enable)
 *   PD4  — GPIO     (CHG_STAT — battery charger status)
 *   PD5  — USART2_RTS (RS-485 DE)
 *   PD6  — USART2_CTS (RS-485 RE)
 *   PD7  — GPIO     (RS485_nRE — not used, DE controls TX)
 *   PD8  — USART3_TX (nRF52840 UART_TX, for BLE module control)
 *   PD9  — USART3_RX (nRF52840 UART_RX)
 *   PD10 — GPIO     (GNSS_PPS — 1PPS from SAM-M10Q, EXTI)
 *   PD11 — GPIO     (GNSS_nRST)
 *   PD12 — GPIO     (BUZZER — survey pace indicator)
 *   PD13 — GPIO     (nRST_BTN — reset button, active low)
 *   PD14 — GPIO     (USER_BTN — user button)
 *   PD15 — GPIO     (nRF52840_BOOT)
 */

#ifndef STRATASCAN_BOARD_H
#define STRATASCAN_BOARD_H

#include <stdint.h>
#include "registers.h"

/* ===================================================================== */
/*  Clock configuration                                                   */
/* ===================================================================== */

/*
 * HSE 8 MHz crystal → PLL1 → SYSCLK 480 MHz
 *   PLL1: source=HSE, M=1, N=60, P=2 → VCO=480 MHz, SYSCLK=240 MHz? No.
 *   Correct: M=2 → 4 MHz ref, N=120 → 480 MHz VCO, P=1 → 480 MHz SYSCLK
 *   Actually H7 supports up to 480 MHz with P=2 and N=120:
 *     HSE 8 / M(2) = 4 MHz, × N(120) = 480 MHz, / P(2) = 240 MHz → not 480.
 *   For 480 MHz: M=1, N=60, P=1 → 8×60=480, /1=480. But N min is 6, ok.
 *   Wait — ST's reference config: HSE 25 MHz, M=5, N=160, P=2 → 400 MHz.
 *   For our 8 MHz HSE: M=2, N=120, P=2 → 8/2=4, 4×120=480, 480/2=240 MHz.
 *   To get 480 MHz from 8 MHz HSE: M=1, N=60, P=1 → 8×60=480/1=480 MHz.
 *   However, H7 VCO max is 960 MHz and N can be up to 512, so:
 *     M=1, N=120, P=2 → VCO=960 MHz, SYSCLK=480 MHz. 8×120=960, /2=480. ✓
 */
#define BOARD_HSE_FREQ_HZ      8000000ULL
#define BOARD_SYSCLK_FREQ_HZ  480000000ULL
#define BOARD_HCLK_FREQ_HZ    240000000ULL   /* D1 domain (CPU) */
#define BOARD_APB1_FREQ_HZ    120000000ULL   /* D2 APB1 (TIM2–TIM7, USART2/3) */
#define BOARD_APB2_FREQ_HZ    120000000ULL   /* D2 APB2 (SPI1, TIM1, TIM8) */
#define BOARD_APB4_FREQ_HZ    120000000ULL   /* D3 APB4 (I2C, GPIO) */

/* PLL1 configuration: HSE 8 / M(1) = 8 MHz, ×N(120) = 960 MHz, /P(2) = 480 */
#define PLL1_M   1
#define PLL1_N   120
#define PLL1_P   2

/* ===================================================================== */
/*  Physical & survey constants                                          */
/* ===================================================================== */

#define SPEED_OF_LIGHT_MPS     299792458.0
#define DEFAULT_EPS_R          6.0f     /* typical dry soil */
#define MAX_SWEEP_STEPS        1024
#define MIN_SWEEP_STEPS        64
#define DEFAULT_SWEEP_STEPS    512
#define WHEEL_PPR               1024     /* pulses per revolution */
#define WHEEL_DIAMETER_MM       150      /* survey wheel diameter */
#define WHEEL_CIRCUMFERENCE_MM  (WHEEL_DIAMETER_MM * 3.14159265f)
#define DEFAULT_TRACE_SPACING_MM  20.0f
#define ADC_SAMPLE_RATE_HZ    8000      /* ADS131M08 8 kSPS */
#define BSCAN_BUFFER_DEPTH    4096      /* max traces per B-scan */

/* ===================================================================== */
/*  Pin definitions (port + bit)                                         */
/* ===================================================================== */

/* PLL SPI (SPI1) */
#define PLL_SCK_PORT   GPIOB
#define PLL_SCK_PIN    3
#define PLL_MISO_PORT  GPIOB
#define PLL_MISO_PIN   4
#define PLL_MOSI_PORT  GPIOB
#define PLL_MOSI_PIN   5
#define PLL_LE_PORT    GPIOA
#define PLL_LE_PIN     8
#define PLL_CE_PORT    GPIOB
#define PLL_CE_PIN     0
#define PLL_RF_EN_PORT  GPIOC
#define PLL_RF_EN_PIN  5

/* ADC SPI (SPI3) — ADS131M08 */
#define ADC_MISO_PORT  GPIOA
#define ADC_MISO_PIN   6
#define ADC_MOSI_PORT  GPIOA
#define ADC_MOSI_PIN   7
#define ADC_SCK_PORT   GPIOC  /* SPI3 SCK on PC10? No — conflicts with SDMMC1.
                                 Use SPI3 on PB3(SCK) PB4(MISO) PB5(MOSI)?
                                 Those are used by PLL. Use software SPI. */
#define ADC_SCK_PORT   GPIOB
#define ADC_SCK_PIN    13     /* reuse SPI2 SCK as software bit-bang for ADC */
#define ADC_NCS_PORT   GPIOB
#define ADC_NCS_PIN    8
#define ADC_NRST_PORT  GPIOB
#define ADC_NRST_PIN   9
#define ADC_DRDY_PORT  GPIOB
#define ADC_DRDY_PIN   10
#define ADC_DRDY_EXTI  10     /* EXTI line 10 */

/* I2C1 — BMI270 IMU + SSD1309 OLED */
#define I2C_SCL_PORT   GPIOB
#define I2C_SCL_PIN    6
#define I2C_SDA_PORT   GPIOB
#define I2C_SDA_PIN    7

/* USART1 — SAM-M10Q GNSS */
#define GNSS_TX_PORT   GPIOA
#define GNSS_TX_PIN    9
#define GNSS_RX_PORT   GPIOA
#define GNSS_RX_PIN    10
#define GNSS_PPS_PORT  GPIOD
#define GNSS_PPS_PIN   10
#define GNSS_NRST_PORT GPIOD
#define GNSS_NRST_PIN  11
#define GNSS_BAUD      9600

/* USART2 — RS-485 */
#define RS485_TX_PORT  GPIOA
#define RS485_TX_PIN   2
#define RS485_RX_PORT  GPIOA
#define RS485_RX_PIN   3
#define RS485_DE_PORT  GPIOD
#define RS485_DE_PIN   5
#define RS485_BAUD     115200

/* USART3 — nRF52840 BLE */
#define BLE_TX_PORT    GPIOD
#define BLE_TX_PIN     8
#define BLE_RX_PORT    GPIOD
#define BLE_RX_PIN     9
#define BLE_NRST_PORT  GPIOD
#define BLE_NRST_PIN   0
#define BLE_INT_PORT   GPIOD
#define BLE_INT_PIN    1
#define BLE_BAUD       460800

/* TIM3 — survey wheel encoder */
#define WHEEL_CHA_PORT GPIOC
#define WHEEL_CHA_PIN  6
#define WHEEL_CHB_PORT GPIOC
#define WHEEL_CHB_PIN  7

/* SDMMC1 — MicroSD */
#define SD_D0_PORT     GPIOC
#define SD_D0_PIN      8
#define SD_D1_PORT     GPIOC
#define SD_D1_PIN      9
#define SD_D2_PORT     GPIOC
#define SD_D2_PIN      10
#define SD_D3_PORT     GPIOC
#define SD_D3_PIN      11
#define SD_CLK_PORT    GPIOC
#define SD_CLK_PIN     12
#define SD_CMD_PORT    GPIOD
#define SD_CMD_PIN     2

/* USB OTG FS */
#define USB_DM_PORT    GPIOA
#define USB_DM_PIN     11
#define USB_DP_PORT    GPIOA
#define USB_DP_PIN     12

/* Status LEDs */
#define LED_R_PORT     GPIOC
#define LED_R_PIN      13
#define LED_G_PORT     GPIOC
#define LED_G_PIN      14
#define LED_B_PORT     GPIOC
#define LED_B_PIN      15

/* Power control */
#define PWR_EN_PORT    GPIOD
#define PWR_EN_PIN     3
#define CHG_STAT_PORT  GPIOD
#define CHG_STAT_PIN   4

/* RF control GPIOs */
#define VCO_SEL0_PORT  GPIOB
#define VCO_SEL0_PIN   1
#define VCO_SEL1_PORT  GPIOB
#define VCO_SEL1_PIN   2
#define LNA_EN_PORT    GPIOC
#define LNA_EN_PIN     4

/* DAC outputs */
#define DAC_VCO_BIAS_PORT  GPIOA
#define DAC_VCO_BIAS_PIN   4
#define DAC_LNA_GAIN_PORT  GPIOA
#define DAC_LNA_GAIN_PIN   5

/* Buttons */
#define NRST_BTN_PORT  GPIOD
#define NRST_BTN_PIN   13
#define USER_BTN_PORT  GPIOD
#define USER_BTN_PIN   14
#define BUZZER_PORT    GPIOD
#define BUZZER_PIN     12

/* ===================================================================== */
/*  I2C device addresses                                                  */
/* ===================================================================== */
#define IMU_I2C_ADDR     0x68   /* BMI270 (SDO=GND) */
#define OLED_I2C_ADDR    0x3C   /* SSD1309 */

/* ===================================================================== */
/*  LED helper macros                                                     */
/* ===================================================================== */
#define LED_R_ON()   (LED_R_PORT->BSRR  = (1U << (LED_R_PIN  + 16)))
#define LED_R_OFF()  (LED_R_PORT->BSRR  = (1U << LED_R_PIN))
#define LED_G_ON()   (LED_G_PORT->BSRR  = (1U << (LED_G_PIN  + 16)))
#define LED_G_OFF()  (LED_G_PORT->BSRR  = (1U << LED_G_PIN))
#define LED_B_ON()   (LED_B_PORT->BSRR  = (1U << (LED_B_PIN  + 16)))
#define LED_B_OFF() (LED_B_PORT->BSRR  = (1U << LED_B_PIN))

/* ===================================================================== */
/*  RF band presets                                                       */
/* ===================================================================== */

typedef struct {
    const char *name;
    uint32_t f_start_mhz;   /* sweep start frequency in MHz */
    uint32_t f_stop_mhz;    /* sweep stop frequency in MHz */
    uint16_t num_steps;     /* number of frequency steps */
    uint16_t dwell_us;      /* dwell time per step in microseconds */
    uint8_t  vco_sel;       /* VCO select code (0=585, 1=5841, 2=divider) */
    float    lna_gain_db;   /* LNA gain setting */
} band_preset_t;

extern const band_preset_t band_presets[5];

/* VCO select codes */
#define VCO_SEL_HMC585   0   /* 500 MHz – 1 GHz (LO/DEEP lower half) */
#define VCO_SEL_HMC5841  1   /* 1 – 3 GHz (HI/UHI) */
#define VCO_SEL_DIVIDER  2   /* divider output for < 500 MHz (LO/DEEP upper) */

/* ===================================================================== */
/*  System state machine                                                  */
/* ===================================================================== */

typedef enum {
    STATE_BOOT = 0,
    STATE_IDLE,
    STATE_CALIBRATE,
    STATE_SURVEY,
    STATE_PAUSE,
    STATE_SHUTDOWN
} sys_state_t;

extern volatile sys_state_t g_state;

/* ===================================================================== */
/*  Global timing                                                        */
/* ===================================================================== */
extern volatile uint32_t g_tick_ms;
uint32_t board_get_tick_ms(void);
void     board_delay_ms(uint32_t ms);
void     board_led_set(uint8_t r, uint8_t g, uint8_t b);

#endif /* STRATASCAN_BOARD_H */