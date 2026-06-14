/*
 * SkyPilot — board.h — Pin definitions for STM32H743VIT6
 * Already defined in phase4_software_stack.md
 * This file is the canonical copy for the firmware build.
 */

#ifndef SKYPILOT_BOARD_H
#define SKYPILOT_BOARD_H

#include "registers.h"

/* ==================== Clock Configuration ==================== */
#define HSE_FREQ        24000000UL   /* 24 MHz external crystal */
#define SYSCLK_FREQ     480000000UL  /* 480 MHz system clock */
#define AHB_FREQ        240000000UL  /* 240 MHz AHB */
#define APB1_FREQ       120000000UL  /* 120 MHz APB1 */
#define APB2_FREQ       120000000UL  /* 120 MHz APB2 */
#define APB4_FREQ        60000000UL  /*  60 MHz APB4 */

/* ==================== Pin Definitions ==================== */

/* SPI1 — ICM-42688-P (Primary IMU) */
#define IMU1_CS_PIN        4    /* PA4 */
#define IMU1_CS_PORT        GPIOA
#define IMU1_SCK_PIN        5    /* PA5 */
#define IMU1_MISO_PIN       6    /* PA6 */
#define IMU1_MOSI_PIN       7    /* PA7 */
#define IMU1_INT1_PIN       3    /* PB3 */
#define IMU1_INT1_PORT      GPIOB
#define IMU1_INT2_PIN       11   /* PD11 */
#define IMU1_INT2_PORT      GPIOD

/* SPI2 — BMI270 (Secondary IMU) */
#define IMU2_CS_PIN         4    /* PC4 */
#define IMU2_CS_PORT        GPIOC
#define IMU2_SCK_PIN        5    /* PC5 */
#define IMU2_SCK_PORT       GPIOC
#define IMU2_MISO_PIN       13   /* PB13 */
#define IMU2_MISO_PORT      GPIOB
#define IMU2_MOSI_PIN       14   /* PB14 */
#define IMU2_MOSI_PORT      GPIOB
#define IMU2_INT1_PIN       4    /* PB4 */
#define IMU2_INT1_PORT      GPIOB
#define IMU2_INT2_PIN       12   /* PD12 */
#define IMU2_INT2_PORT      GPIOD

/* I2C1 — BMP390 + RV-3032 */
#define I2C1_SCL_PIN        6    /* PB6 */
#define I2C1_SCL_PORT       GPIOB
#define I2C1_SDA_PIN        5    /* PB5 */
#define I2C1_SDA_PORT       GPIOB

/* UART1 — SAM-M10Q GNSS */
#define GNSS_TX_PIN         8    /* PB8 */
#define GNSS_TX_PORT        GPIOB
#define GNSS_RX_PIN         7    /* PB7 */
#define GNSS_RX_PORT        GPIOB
#define GNSS_RST_PIN        7    /* PD7 */
#define GNSS_RST_PORT       GPIOD

/* UART8 — LARA-R6 LTE Modem */
#define LTE_TX_PIN          10   /* PC10 */
#define LTE_TX_PORT         GPIOC
#define LTE_RX_PIN          9    /* PC9 */
#define LTE_RX_PORT         GPIOC
#define LTE_RTS_PIN         14   /* PD14 */
#define LTE_RTS_PORT        GPIOD
#define LTE_CTS_PIN         15   /* PD15 */
#define LTE_CTS_PORT        GPIOD
#define LTE_PWR_ON_PIN      0    /* PA0 */
#define LTE_PWR_ON_PORT     GPIOA
#define LTE_STATUS_PIN       9    /* PD9 */
#define LTE_STATUS_PORT      GPIOD

/* USB — Configuration port */
#define USB_DM_PIN          11   /* PA11 */
#define USB_DP_PIN          12   /* PA12 */
#define USB_DM_PORT         GPIOA
#define USB_DP_PORT         GPIOA

/* DShot Motor Outputs */
#define MOTOR1_PIN          8    /* PE8  TIM1_CH2 */
#define MOTOR2_PIN          9    /* PE9  TIM1_CH3 */
#define MOTOR3_PIN          10   /* PE10 TIM1_CH3 */
#define MOTOR4_PIN          11   /* PE11 TIM1_CH4 */
#define MOTOR5_PIN          12   /* PE12 TIM1_CH1N */
#define MOTOR6_PIN          13   /* PE13 TIM1_CH2N */
#define MOTOR7_PIN          14   /* PE14 TIM1_CH3N */
#define MOTOR8_PIN          0    /* PB0  TIM3_CH3 */
#define MOTOR9_PIN          1    /* PB1  TIM3_CH4 */
#define MOTOR10_PIN         10   /* PA10 TIM8_CH1 */
#define MOTOR11_PIN         11   /* PB11 TIM2_CH4 */
#define MOTOR12_PIN         10   /* PB10 TIM2_CH3 */

/* ADC */
#define ADC_BATT_PIN        0    /* PA0 → ADC1_INP16 */
#define ADC_CURR_PIN        1    /* PA1 → ADC1_INP17 */

/* LEDs */
#define LED_POWER_PIN       2    /* PD2 */
#define LED_POWER_PORT      GPIOD
#define LED_STATUS_PIN      7    /* PE7 */
#define LED_STATUS_PORT     GPIOE
#define LED_LTE_PIN         1    /* PD1 */
#define LED_LTE_PORT        GPIOD

/* Buttons */
#define BTN_RESET_PIN       3    /* PD3 */
#define BTN_RESET_PORT      GPIOD
#define BTN_BOOT_PIN        4    /* PD4 */
#define BTN_BOOT_PORT       GPIOD

/* SPI4 — W25Q128 Flash */
#define FLASH_CS_PIN        1    /* PE1 */
#define FLASH_CS_PORT       GPIOE
#define FLASH_SCK_PIN       2    /* PE2 */
#define FLASH_SCK_PORT      GPIOE
#define FLASH_MISO_PIN      3    /* PE3 */
#define FLASH_MISO_PORT     GPIOE
#define FLASH_MOSI_PIN      4    /* PE4 */
#define FLASH_MOSI_PORT     GPIOE

/* ==================== Helper Macros ==================== */
#define BIT(n)              (1UL << (n))
#define READ_REG(reg)       (*(volatile uint32_t *)&(reg))
#define WRITE_REG(reg, val) ((*(volatile uint32_t *)&(reg)) = (val))
#define MODIFY_REG(reg, clear, set) \
    WRITE_REG(reg, (READ_REG(reg) & ~(clear)) | (set))

#endif /* SKYPILOT_BOARD_H */