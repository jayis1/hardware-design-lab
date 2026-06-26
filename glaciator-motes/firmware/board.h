/*
 * board.h — Pin assignments and peripheral mappings for Glaciator-Motes
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- STM32WL55JC pin map -------------------------------------------------- */

/* ADS1256 24-bit seismic ADC (SPI1) */
#define ADS1256_SPI              SPI1
#define ADS1256_SPI_SCK_PIN      GPIO_PIN(0, 5)   /* PA5 */
#define ADS1256_SPI_MISO_PIN     GPIO_PIN(0, 6)   /* PA6 */
#define ADS1256_SPI_MOSI_PIN     GPIO_PIN(0, 7)   /* PA7 */
#define ADS1256_CS_PIN           GPIO_PIN(0, 4)   /* PA4 */
#define ADS1256_DRDY_PIN         GPIO_PIN(0, 1)   /* PA1 EXTI1 */
#define ADS1256_DRDY_EXTI        EXTI1
#define ADS1256_DRDY_IRQ         EXTI1_IRQHandler
#define ADS1256_DRDY_IRQn        EXTI1_IRQn
#define ADS1256_SYNC_PIN         GPIO_PIN(0, 0)   /* PA0 (optional SYNC) */

/* ICM-42688 MEMS accelerometer (SPI1, shared with ADS1256 via CS) */
#define ICM42688_CS_PIN          GPIO_PIN(0, 8)   /* PA8 */
#define ICM42688_DRDY_PIN        GPIO_PIN(0, 9)   /* PA9 EXTI9 */
#define ICM42688_DRDY_EXTI       EXTI9
#define ICM42688_DRDY_IRQ        EXTI9_IRQHandler
#define ICM42688_DRDY_IRQn       EXTI9_IRQn

/* microSD card (SPI2) */
#define SD_SPI                   SPI2
#define SD_SPI_SCK_PIN           GPIO_PIN(2, 13)  /* PC13 */
#define SD_SPI_MISO_PIN          GPIO_PIN(2, 2)   /* PB2 */
#define SD_SPI_MOSI_PIN          GPIO_PIN(2, 3)   /* PB3 */
#define SD_CS_PIN                GPIO_PIN(2, 0)   /* PB0 */
#define SD_CD_PIN                GPIO_PIN(2, 1)   /* PB1 (card detect) */

/* GPS NEO-M9N (USART1 + PPS) */
#define GPS_UART                 USART1
#define GPS_UART_TX_PIN          GPIO_PIN(0, 9)   /* PA9 shared with MEMS DRDY? no — reuse PA12 */
#define GPS_UART_RX_PIN          GPIO_PIN(0, 10)  /* PA10 */
#define GPS_TX_PIN               GPIO_PIN(0, 12)  /* PA12 */
#define GPS_RX_PIN               GPIO_PIN(0, 11)  /* PA11 */
#define GPS_PPS_PIN              GPIO_PIN(0, 3)   /* PA3 EXTI3 */
#define GPS_PPS_EXTI             EXTI3
#define GPS_PPS_IRQ              EXTI3_IRQHandler
#define GPS_PPS_IRQn             EXTI3_IRQn
#define GPS_EN_PIN               GPIO_PIN(1, 2)   /* PB2 — power MOSFET gate */

/* BQ25895 charger (I2C1) */
#define BQ25895_I2C              I2C1
#define BQ25895_I2C_SCL_PIN      GPIO_PIN(0, 14)  /* PA14 */
#define BQ25895_I2C_SDA_PIN      GPIO_PIN(0, 15)  /* PA15 */
#define BQ25895_I2C_ADDR         (0x6B << 1)
#define BQ25895_INT_PIN          GPIO_PIN(2, 6)   /* PC6 EXTI6 */

/* LoRa / sub-GHz radio: internal SX1262 on STM32WL55 (SUBGHZSPI) */
#define RADIO_IRQ_PIN            GPIO_PIN(1, 3)   /* PB3 (radio IRQ via SUBGHZ) */
#define RADIO_BUSY_PIN           GPIO_PIN(1, 4)   /* PB4 */
#define RADIO_NSS_PIN            GPIO_PIN(1, 5)   /* PB5 (internal SUBGHZSPI) */
#define RADIO_RESET_PIN          GPIO_PIN(1, 6)   /* PB6 */

/* TCXO enable */
#define TCXO_26M_EN_PIN          GPIO_PIN(2, 0)   /* PC0 */
#define TCXO_32K_EN_PIN          GPIO_PIN(2, 1)   /* PC1 */

/* Status LED (low-current, 1.8 V) */
#define LED_STATUS_PIN           GPIO_PIN(2, 15)  /* PC15 */

/* Battery heater MOSFET gate */
#define HEATER_EN_PIN            GPIO_PIN(1, 7)   /* PB7 */

/* ---- Timing --------------------------------------------------------------- */
#define SYSTEM_CORE_CLOCK_HZ     48000000UL
#define LSE_CLOCK_HZ             32768UL
#define RADIO_FREQ_HZ            868000000UL     /* EU */
#define RADIO_FREQ_HZ_US         915000000UL
#define ADC_SAMPLE_RATE_HZ       200UL
#define ADC_CHANNELS             8               /* 3 geophone + mems + t + vbat + vsol + cal */
#define MEMS_SAMPLE_RATE_HZ      1000UL

/* ---- Mesh parameters ------------------------------------------------------ */
#define MESH_BEACON_PERIOD_S     60
#define MESH_SUPERFRAME_S        30
#define MESH_SLOTS_PER_FRAME     15
#define MESH_SLOT_S              2
#define MESH_MAX_GROUPS          4
#define MESH_MAX_NODES           (MESH_SLOTS_PER_FRAME * MESH_MAX_GROUPS)
#define MESH_PAYLOAD_MAX         110
#define MESH_MY_NODE_ID          0x01            /* overwritten at provisioning */

/* ---- Event detection ------------------------------------------------------ */
#define STA_WINDOW_S             0.5f
#define LTA_WINDOW_S             10.0f
#define STA_LTA_THRESHOLD        3.0f
#define EVENT_PRE_TRIGGER_S      5.0f
#define EVENT_POST_TRIGGER_S     5.0f
#define EVENT_TEMPLATES_MAX      8
#define EVENT_TEMPLATE_LEN       512             /* samples, one-bit */

/* ---- Power FSM thresholds ------------------------------------------------- */
#define VBAT_FULL_MV             3600
#define VBAT_OK_MV               3200
#define VBAT_LOW_MV              3100
#define VBAT_CRIT_MV             3000
#define VSOL_GOOD_MV             4500
#define TEMP_COLD_C              -200            /* -20.0 C (0.1 C units) */
#define TEMP_VERYCOLD_C          -300            /* -30.0 C */

/* ---- GPIO helper ---------------------------------------------------------- */
#define GPIO_PIN(port, num)   ((port) * 16 + (num))
#define GPIO_PORT(p)          ((p) / 16)
#define GPIO_NUM(p)           ((p) % 16)

/* ---- Lightweight register/MMIO shim --------------------------------------- */
/* Lets the firmware compile against an STM32-style register model. */
#define __IO  volatile
#define __I   volatile const
#define REG32(addr)  (*(volatile uint32_t *)(addr))

#endif /* BOARD_H */