/*
 * board.h — RootTrace Hardware Board Configuration
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Pin assignments, peripheral mappings, and physical constants for the
 * RootTrace rev A PCB built around the STM32H743ZIT6 Cortex-M7
 * microcontroller.  All pin numbers are absolute GPIO port pins (0..15).
 */

#ifndef ROOTTRACE_BOARD_H
#define ROOTTRACE_BOARD_H

#include <stdint.h>
#include <stddef.h>
#include "registers.h"

/* ===================================================================== *
 *  Clock tree configuration
 *  HSE  25 MHz crystal  ->  PLL1  ->  SYSCLK 480 MHz
 *  PLL1: M=5, N=192, P=2  -> 25/5*192/2 = 480 MHz
 *  AHB  = SYSCLK / 2 = 240 MHz
 *  APB1 = 120 MHz  (TIM = 240 MHz domain)
 *  APB2 = 120 MHz
 * ===================================================================== */

#define BOARD_HSE_HZ          25000000UL
#define BOARD_SYSCLK_HZ       480000000UL
#define BOARD_HCLK_HZ         240000000UL
#define BOARD_PCLK1_HZ        120000000UL
#define BOARD_PCLK2_HZ        120000000UL
#define BOARD_TIMCLK_HZ       240000000UL

/* Flash access: 8 wait states for VOS1 @ 480 MHz */
#define BOARD_FLASH_WS         3
#define BOARD_VOS_SCALE         1   /* VOS1, high-performance range */

/* ===================================================================== *
 *  SPI assignments
 * ===================================================================== */

/* SPI1 — AD5940 bioimpedance AFE  (PA5 CLK, PA6 MISO, PA7 MOSI, PA4 CS) */
#define AD5940_SPI             SPI1
#define AD5940_SPI_IRQn        SPI1_IRQn
#define AD5940_CS_PORT         GPIOA
#define AD5940_CS_PIN          4
#define AD5940_IRQ_PORT        GPIOC
#define AD5940_IRQ_PIN         4
#define AD5940_RESET_PORT      GPIOC
#define AD5940_RESET_PIN      5
#define AD5940_SPI_AF          5

/* SPI2 — OLED display (SSD1327) (PB10 CLK, PC2 MISO, PC3 MOSI, PB12 D/C, PB11 CS) */
#define OLED_SPI               SPI2
#define OLED_SPI_IRQn          SPI2_IRQn
#define OLED_CS_PORT           GPIOB
#define OLED_CS_PIN            11
#define OLED_DC_PORT           GPIOB
#define OLED_DC_PIN            12
#define OLED_RESET_PORT        GPIOB
#define OLED_RESET_PIN         13
#define OLED_SPI_AF            5

/* SDMMC1 — microSD card (8-bit) */
#define SDMMC_CLK_PORT         GPIOC
#define SDMMC_CLK_PIN          12
#define SDMMC_CMD_PORT         GPIOD
#define SDMMC_CMD_PIN          2
#define SDMMC_D0_PORT          GPIOC
#define SDMMC_D0_PIN           8
#define SDMMC_D1_PORT          GPIOC
#define SDMMC_D1_PIN          9
#define SDMMC_D2_PORT          GPIOC
#define SDMMC_D2_PIN           10
#define SDMMC_D3_PORT          GPIOC
#define SDMMC_D3_PIN           11
#define SDMMC_CD_PORT          GPIOG
#define SDMMC_CD_PIN           2

/* SPI5 — BLE co-processor (STM32WB55) (PF7 CLK, PF8 MISO, PF9 MOSI — no, use USART) */
/* BLE uses USART2 instead (UART), see below */

/* ===================================================================== *
 *  USART assignments
 * ===================================================================== */

/* USART2 — BLE co-processor link  (PA2 TX, PA3 RX, PD3 CTS, PD4 RTS) */
#define BLE_UART               USART2
#define BLE_UART_IRQn          USART2_IRQn
#define BLE_UART_AF            7
#define BLE_TX_PORT            GPIOA
#define BLE_TX_PIN             2
#define BLE_RX_PORT            GPIOA
#define BLE_RX_PIN             3
#define BLE_CTS_PORT           GPIOD
#define BLE_CTS_PIN            3
#define BLE_RTS_PORT           GPIOD
#define BLE_RTS_PIN            4
#define BLE_RESET_PORT         GPIOG
#define BLE_RESET_PIN          10
#define BLE_BAUD               921600

/* USART3 — USB-C debug / firmware update console (PC10 TX, PC11 RX) */
#define DBG_UART               USART3
#define DBG_UART_IRQn          USART3_IRQn
#define DBG_UART_AF            7
#define DBG_TX_PORT           GPIOC
#define DBG_TX_PIN            10
#define DBG_RX_PORT           GPIOC
#define DBG_RX_PIN            11
#define DBG_BAUD              921600

/* ===================================================================== *
 *  I2C assignments
 * ===================================================================== */

/* I2C1 — SHT41 T/RH + MAX17048 fuel gauge (PB6 SCL, PB7 SDA) */
#define ENV_I2C                I2C1
#define ENV_I2C_IRQn_EV        I2C1_EV_IRQn
#define ENV_I2C_IRQn_ER        I2C1_ER_IRQn
#define ENV_I2C_AF             4
#define ENV_SCL_PORT           GPIOB
#define ENV_SCL_PIN            6
#define ENV_SDA_PORT           GPIOB
#define ENV_SDA_PIN            7

/* I2C3 — RTC (PCF85263A) (PA8 SCL, PC9 SDA) */
#define RTC_I2C                I2C3
#define RTC_I2C_IRQn_EV        I2C3_EV_IRQn
#define RTC_I2C_IRQn_ER        I2C3_ER_IRQn
#define RTC_I2C_AF             4
#define RTC_SCL_PORT           GPIOA
#define RTC_SCL_PIN            8
#define RTC_SDA_PORT           GPIOC
#define RTC_SDA_PIN            9

/* ===================================================================== *
 *  GPIO — multiplexer + misc control pins
 * ===================================================================== */

/* 16-electrode multiplexer address bus (A0-A3, 4 bits per MUX chip) */
#define MUX_A0_PORT            GPIOE
#define MUX_A0_PIN             0
#define MUX_A1_PORT            GPIOE
#define MUX_A1_PIN             1
#define MUX_A2_PORT            GPIOE
#define MUX_A2_PIN             2
#define MUX_A3_PORT            GPIOE
#define MUX_A3_PIN             3

/* MUX enable lines (one per ADG725, active low) */
#define MUX_EN_EXC_POS         GPIOE   /* excitation positive selector */
#define MUX_EN_EXC_POS_PIN     4
#define MUX_EN_EXC_NEG         GPIOE   /* excitation negative selector */
#define MUX_EN_EXC_NEG_PIN     5
#define MUX_EN_MEAS_POS        GPIOE   /* measurement positive selector */
#define MUX_EN_MEAS_POS_PIN    6
#define MUX_EN_MEAS_NEG        GPIOE   /* measurement negative selector */
#define MUX_EN_MEAS_NEG_PIN    7

/* Soil moisture capacitive sensor (analog) */
#define SOIL_MOIST_PORT        GPIOF
#define SOIL_MOIST_PIN         10      /* ADC3_IN8 via PC3 — reassigned below */
#define SOIL_MOIST_ADC_CH      8

/* DS18B20 soil temperature 1-Wire */
#define DS18B20_PORT           GPIOG
#define DS18B20_PIN            11

/* USB-C VBUS detect */
#define USB_VBUS_PORT          GPIOG
#define USB_VBUS_PIN           6

/* User button */
#define BTN_PORT               GPIOC
#define BTN_PIN                13

/* Status LED (RGB) */
#define LED_R_PORT             GPIOB
#define LED_R_PIN              0
#define LED_G_PORT             GPIOB
#define LED_G_PIN              1
#define LED_B_PORT             GPIOB
#define LED_B_PIN              14

/* ===================================================================== *
 *  AD5940 excitation and measurement parameters
 * ===================================================================== */

#define AD5940_MCLK_HZ         16000000UL   /* AD5940 internal 16 MHz oscillator */
#define AD5940_ADC_CLK_DIV     8             /* ADC clock = 2 MHz */
#define AD5940_ADC_CLK_HZ      (AD5940_MCLK_HZ / AD5940_ADC_CLK_DIV)
#define AD5940_ADC_16BIT        1
#define AD5940_DFT_LEN          1024          /* DFT samples per measurement */

/* Excitation frequencies for fd-EIT sweep (Hz) */
#define EIT_FREQ_COUNT         4
#define EIT_FREQ_0             1000UL
#define EIT_FREQ_1             10000UL
#define EIT_FREQ_2             50000UL
#define EIT_FREQ_3             100000UL

/* Excitation current amplitude (amplitude code, 0..0x3FF = 0..511 µA) */
#define EIT_CURRENT_UA         200            /* 200 µA default */
#define EIT_CURRENT_MAX_UA     500

/* Number of electrodes in the ring */
#define EIT_NUM_ELECTRODES     16

/* Number of stimulation patterns (adjacent pairs) */
#define EIT_NUM_STIM          EIT_NUM_ELECTRODES

/* Number of measurements per stimulation (adjacent, excluding stim pair ±1) */
#define EIT_NUM_MEAS_PER_STIM 13

/* Total independent measurements per frame */
#define EIT_FRAME_SIZE        (EIT_NUM_STIM * EIT_NUM_MEAS_PER_STIM)  /* 208 */

/* Reconstruction mesh parameters */
#define EIT_MESH_NODES         576
#define EIT_MESH_ELEMENTS      1082
#define EIT_GN_ITERATIONS      3            /* Gauss-Newton iterations */
#define EIT_REG_LAMBDA_DEFAULT 0.01f        /* Tikhonov regularization parameter */

/* ===================================================================== *
 *  Timing
 * ===================================================================== */

#define EIT_MUX_SETTLE_US      200
#define EIT_AD5940_SETTLE_US   100
#define EIT_FRAME_INTERVAL_MS  400           /* 2.5 fps default */
#define EIT_INTER_FRAME_MS     50            /* gap between frames */

/* ===================================================================== *
 *  Battery / power
 * ===================================================================== */

#define BATT_CELLS             1
#define BATT_NOMINAL_MV         3700
#define BATT_FULL_MV            4200
#define BATT_EMPTY_MV           3200
#define BATT_WARN_PCT          20

/* USB-C PD voltage (fixed 5V) */
#define USB_VBUS_MV_THRESH     4500

/* ===================================================================== *
 *  SD card logging
 * ===================================================================== */

#define SD_FILENAME_PREFIX    "rtframe"
#define SD_MAX_FILES          9999
#define SD_LOG_RAW_FRAMES     1               /* 1 = log raw frames */
#define SD_LOG_RECON          1               /* 1 = log reconstruction images */
#define SD_LOG_INTERVAL_S     10              /* snapshot interval (s) */

/* ===================================================================== *
 *  BLE protocol
 * ===================================================================== */

#define BLE_MAX_PACKET         244
#define BLE_FRAME_CHUNK_SIZE    180            /* max GATT MTU minus header */
#define BLE_FRAME_RETRANSMIT    3
#define BLE_CONNECT_TIMEOUT_MS  5000

/* BLE command opcodes (firmware <-> companion app) */
#define BLE_CMD_PING           0x01
#define BLE_CMD_START_SCAN       0x02
#define BLE_CMD_STOP_SCAN        0x03
#define BLE_CMD_GET_FRAME        0x04
#define BLE_CMD_GET_STATUS       0x05
#define BLE_CMD_GET_CALIB        0x06
#define BLE_CMD_SET_CALIB        0x07
#define BLE_CMD_SET_FREQ         0x08
#define BLE_CMD_SET_CURRENT      0x09
#define BLE_CMD_GET_ENV          0x0A
#define BLE_CMD_ERASE_LOGS       0x0B
#define BLE_CMD_GET_FILE_LIST    0x0C
#define BLE_CMD_GET_FILE         0x0D
#define BLE_CMD_SELF_TEST        0x0E
#define BLE_CMD_SET_MESH         0x0F
#define BLE_CMD_GET_BATT         0x10
#define BLE_CMD_ENTER_DFU        0x11

/* BLE response/status codes */
#define BLE_RESP_OK             0x00
#define BLE_RESP_ERR            0x80
#define BLE_RESP_ERR_BUSY       0x81
#define BLE_RESP_ERR_CALIB      0x82
#define BLE_RESP_ERR_NO_SD       0x83
#define BLE_RESP_ERR_HW          0x84

/* ===================================================================== *
 *  Display (SSD1327)
 * ===================================================================== */

#define OLED_WIDTH             128
#define OLED_HEIGHT            64
#define OLED_COL_SIZE          16           /* 4-bit grayscale, 2 px/byte */
#define OLED_FB_SIZE           (OLED_WIDTH * OLED_HEIGHT / 2)

/* ===================================================================== *
 *  Flash calibration storage (last 16 KB sector of bank 1)
 * ===================================================================== */

#define CALIB_FLASH_SECTOR     127
#define CALIB_FLASH_BASE       (0x081FF000UL)
#define CALIB_MAGIC            0x52544331    /* "RTC1" */
#define CALIB_VERSION          1

/* ===================================================================== *
 *  Utility macros
 * ===================================================================== */

#define BIT(n)                 (1U << (n))
#define ARRAY_SIZE(a)          (sizeof(a) / sizeof((a)[0]))
#define MIN(a,b)               ((a) < (b) ? (a) : (b))
#define MAX(a,b)               ((a) > (b) ? (a) : (b))
#define CLAMP(v, lo, hi)        ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

#define PIN_SET(port, pin)     ((port)->BSRR = (1U << (pin)))
#define PIN_CLR(port, pin)     ((port)->BSRR = (1U << ((pin) + 16)))
#define PIN_GET(port, pin)     (((port)->IDR >> (pin)) & 1U)

#endif /* ROOTTRACE_BOARD_H */