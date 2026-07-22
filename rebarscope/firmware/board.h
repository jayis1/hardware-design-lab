/*
 * board.h — Hardware pin/peripheral map for RebarScope
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Target MCU : STM32L4R5ZIT6 (Cortex-M4F @ 120 MHz)
 * All pin assignments match the KiCad schematic in ../kicad/.
 */
#ifndef REBARSCOPE_BOARD_H
#define REBARSCOPE_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- GPIO port/pin shorthand ---- */
#define PORTA 0
#define PORTB 1
#define PORTC 2
#define PORTD 3
#define PORTH 4

#define MAKE_PIN(port, num)  (((port) << 4) | (num))

/* ---- MCU status / control pins ---- */
#define PIN_LED_STATUS       MAKE_PIN(PORTC, 7)   /* green LED, active high   */
#define PIN_LED_ERR          MAKE_PIN(PORTB, 7)   /* red LED, active high     */
#define PIN_PWR_HOLD         MAKE_PIN(PORTA, 12)  /* keep PSU on until SW off */

/* ---- Analog block power-gating load switches (PMOS gates, active low) ---- */
#define PIN_PWR_ADS1220      MAKE_PIN(PORTC, 10)  /* HCP ADC front-end supply */
#define PIN_PWR_AD5940       MAKE_PIN(PORTC, 11)  /* Wenner + LPR AFE supply */
#define PIN_PWR_EDDY         MAKE_PIN(PORTB, 5)   /* eddy driver + DDS supply */
#define PIN_PWR_OLED         MAKE_PIN(PORTA, 15)

/* ---- ADS1220 (24-bit ΔΣ ADC, SPI1) ---- */
#define PIN_ADS1220_CS       MAKE_PIN(PORTA, 4)
#define PIN_ADS1220_DRDY     MAKE_PIN(PORTC, 4)
#define SPI_ADS1220          1                     /* SPI1 */

/* ---- AD5940 (impedance AFE, SPI2) ---- */
#define PIN_AD5940_CS        MAKE_PIN(PORTB, 12)
#define PIN_AD5940_DRDY      MAKE_PIN(PORTC, 6)
#define SPI_AD5940           2                     /* SPI2 */

/* ---- AD9833 DDS (eddy excitation, SPI3) ---- */
#define PIN_AD9833_CS        MAKE_PIN(PORTE, 4)
#define PIN_AD9833_FSYNC      PIN_AD9833_CS
#define PIN_EDDY_DRV_EN       MAKE_PIN(PORTC, 3)  /* OPA569 enable */
#define PIN_EDDY_SENSE_SEL    MAKE_PIN(PORTB, 0)  /* analog mux: 0=decay, 1=zero */
#define SPI_AD9833            3                   /* SPI3 */

/* ---- BGM220E BLE (USART2, HCI UART) ---- */
#define USART_BLE            2
#define PIN_BLE_TX           MAKE_PIN(PORTA, 2)   /* USART2_TX */
#define PIN_BLE_RX           MAKE_PIN(PORTA, 3)
#define PIN_BLE_CTS          MAKE_PIN(PORTA, 6)
#define PIN_BLE_RTS          MAKE_PIN(PORTA, 7)
#define PIN_BLE_RST_N        MAKE_PIN(PORTC, 8)
#define BLE_BAUD             921600

/* ---- OLED SSD1306 128x64 (SPI1, shared with ADS1220 CS-isolated) ---- */
#define PIN_OLED_CS          MAKE_PIN(PORTB, 1)
#define PIN_OLED_DC          MAKE_PIN(PORTB, 2)
#define PIN_OLED_RST         MAKE_PIN(PORTA, 8)
#define SPI_OLED             1

/* ---- microSD (SPI1, separate CS) ---- */
#define PIN_SD_CS            MAKE_PIN(PORTC, 5)
#define SPI_SD               1

/* ---- MAX17048 fuel gauge (I2C1) ---- */
#define I2C_FUEL             1
#define FUEL_ADDR            0x36                  /* 7-bit */

/* ---- LSM6DSO IMU (I2C1) ---- */
#define I2C_IMU              1
#define IMU_ADDR            0x6A

/* ---- Wheel encoder (TIM3 in encoder-mode) ---- */
#define PIN_ENC_A            MAKE_PIN(PORTA, 6)   /* PA6 = TIM3_CH1 ... reassigned below */
#define PIN_ENC_B            MAKE_PIN(PORTA, 7)   /* PA7 = TIM3_CH2 */
#define TIM_ENCODER          3
#define ENC_PPR              200
#define ENC_WHEEL_MM         (3.14159265f * 31.83f) /* circumference */
#define ENC_RES_MM           (ENC_WHEEL_MM / (ENC_PPR * 4.0f))

/* ---- User controls ---- */
#define PIN_BTN_TRIGGER      MAKE_PIN(PORTC, 13)  /* blue button on wand */
#define PIN_BTN_MODE         MAKE_PIN(PORTC, 14)  /* mode select */
#define PIN_BTN_PWR          MAKE_PIN(PORTC, 15)

/* ---- ADC channel indices (internal) ---- */
#define ADC_CHAN_VBAT         9    /* via divider */
#define ADC_CHAN_EDDY         15   /* PA0, demodulated eddy signal */
#define ADC_CHAN_TEMP         17   /* internal TSENSE */

/* ---- Board constants ---- */
#define BOARD_NAME           "rebarscope-v1"
#define BOARD_AUTHOR         "jayis1"
#define BOARD_FIRMWARE_VER   0x0100   /* 1.0.0 */
#define DEVICE_SERIAL_LEN    16

/* ---- Default calibration (overridden from flash if present) ---- */
#define CAL_HCP_ZERO_MV          0.0f
#define CAL_AGAGCL_OFFSET_MV    70.0f
#define CAL_WENNER_ALPHA_MM     50.0f
#define CAL_WENNER_K            1.0f
#define CAL_EDDY_T0_US         12.0f
#define CAL_LPR_AREA_CM2       10.0f
#define CAL_LPR_K               2.0f
#define CAL_LPR_B_ACTIVE_MV    26.0f
#define CAL_LPR_B_PASSIVE_MV   52.0f

#endif /* REBARSCOPE_BOARD_H */