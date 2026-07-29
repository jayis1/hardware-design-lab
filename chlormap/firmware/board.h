/*
 * board.h — Pin assignments and peripheral mappings for ChloroMap
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ---- STM32L432KCU6 pin map --------------------------------------------- */
/*
 * Package: UFQFPN32
 * Flash: 256 KB at 0x08000000
 * SRAM:  64 KB at 0x20000000
 */

/* ---- GPIO helper ---- */
#define GPIO_PORT_A  0
#define GPIO_PORT_B  1
#define GPIO_PORT_C  2
#define GPIO_PIN(port, num)  (((port) << 8) | (num))

/* ---- SPI1: ADS1255 24-bit ADC (photodiode array) ---- */
#define ADS1255_SPI              SPI1
#define ADS1255_SPI_SCK_PIN      GPIO_PIN(GPIO_PORT_A, 5)   /* PA5  */
#define ADS1255_SPI_MISO_PIN     GPIO_PIN(GPIO_PORT_A, 6)   /* PA6  */
#define ADS1255_SPI_MOSI_PIN     GPIO_PIN(GPIO_PORT_A, 7)   /* PA7  */
#define ADS1255_CS_PIN           GPIO_PIN(GPIO_PORT_A, 4)   /* PA4  */
#define ADS1255_DRDY_PIN         GPIO_PIN(GPIO_PORT_A, 1)   /* PA1  EXTI1 */
#define ADS1255_DRDY_EXTI        EXTI1
#define ADS1255_DRDY_IRQ         EXTI1_IRQHandler
#define ADS1255_DRDY_IRQn        EXTI1_IRQn
#define ADS1255_SYNC_PIN         GPIO_PIN(GPIO_PORT_A, 0)   /* PA0  */
#define ADS1255_PDN_PIN          GPIO_PIN(GPIO_PORT_B, 0)   /* PB0  */

/* ---- SPI2: SSD1306 OLED display ---- */
#define OLED_SPI                 SPI2
#define OLED_SPI_SCK_PIN         GPIO_PIN(GPIO_PORT_B, 13)  /* PB13 */
#define OLED_SPI_MOSI_PIN        GPIO_PIN(GPIO_PORT_B, 15)  /* PB15 */
#define OLED_SPI_MISO_PIN        GPIO_PIN(GPIO_PORT_B, 14)  /* PB14 (unused, DC) */
#define OLED_CS_PIN              GPIO_PIN(GPIO_PORT_B, 12)  /* PB12 */
#define OLED_DC_PIN              GPIO_PIN(GPIO_PORT_B, 14)  /* PB14 */
#define OLED_RST_PIN             GPIO_PIN(GPIO_PORT_B, 2)   /* PB2  */

/* ---- SPI3: microSD card ---- */
#define SD_SPI                   SPI3
#define SD_SPI_SCK_PIN           GPIO_PIN(GPIO_PORT_B, 3)   /* PB3  */
#define SD_SPI_MISO_PIN          GPIO_PIN(GPIO_PORT_B, 4)   /* PB4  */
#define SD_SPI_MOSI_PIN          GPIO_PIN(GPIO_PORT_B, 5)   /* PB5  */
#define SD_CS_PIN                GPIO_PIN(GPIO_PORT_A, 15)  /* PA15 */
#define SD_CD_PIN                GPIO_PIN(GPIO_PORT_B, 1)   /* PB1  (card detect) */

/* ---- I2C1: u-blox NEO-M9N GPS ---- */
#define GPS_I2C                  I2C1
#define GPS_I2C_SCL_PIN          GPIO_PIN(GPIO_PORT_B, 6)   /* PB6  */
#define GPS_I2C_SDA_PIN          GPIO_PIN(GPIO_PORT_B, 7)   /* PB7  */
#define GPS_ADDR                 0x42                        /* NEO-M9N I2C addr */

/* ---- USART2: NINA-B306 BLE module ---- */
#define BLE_UART                 USART2
#define BLE_UART_TX_PIN          GPIO_PIN(GPIO_PORT_A, 2)   /* PA2  */
#define BLE_UART_RX_PIN          GPIO_PIN(GPIO_PORT_A, 3)   /* PA3  */
#define BLE_UART_CTS_PIN         GPIO_PIN(GPIO_PORT_A, 6)   /* PA6 (alt) */
#define BLE_UART_RTS_PIN         GPIO_PIN(GPIO_PORT_A, 7)   /* PA7 (alt) */
#define BLE_BAUD                 1000000                      /* 1 Mbps */
#define BLE_RST_PIN              GPIO_PIN(GPIO_PORT_A, 8)   /* PA8  */

/* ---- USB-C (STM32L4 native USB FS) ---- */
#define USB_DM_PIN               GPIO_PIN(GPIO_PORT_A, 11)  /* PA11 */
#define USB_DP_PIN               GPIO_PIN(GPIO_PORT_A, 12)  /* PA12 */

/* ---- Trigger & control ---- */
#define TRIGGER_PIN              GPIO_PIN(GPIO_PORT_B, 8)   /* PB8  EXTI8 */
#define TRIGGER_EXTI             EXTI8
#define TRIGGER_IRQ              EXTI8_IRQHandler
#define TRIGGER_IRQn             EXTI8_IRQn

/* ---- LED illumination control (shift register via SPI/gpio) ---- */
#define LED_SR_DATA_PIN          GPIO_PIN(GPIO_PORT_B, 9)   /* PB9  */
#define LED_SR_CLK_PIN           GPIO_PIN(GPIO_PORT_B, 10)  /* PB10 */
#define LED_SR_LATCH_PIN         GPIO_PIN(GPIO_PORT_B, 11)  /* PB11 */
#define LED_WHITE_BIT            0                          /* bit 0 of SR */
#define LED_NIR_BIT              1                          /* bit 1 of SR */
#define LED_AMBIENT_BIT          2                          /* bit 2 (status LED) */

/* ---- Power control ---- */
#define POWER_SWITCH_PIN         GPIO_PIN(GPIO_PORT_A, 0)   /* PA0 (alt: ADC) */
#define VLED_EN_PIN              GPIO_PIN(GPIO_PORT_B, 13)  /* PB13 (alt) */
#define VANA_EN_PIN              GPIO_PIN(GPIO_PORT_A, 9)   /* PA9  */
#define BATT_SENSE_PIN           GPIO_PIN(GPIO_PORT_A, 0)   /* PA0 ADC1_CH0 */
#define BATT_SENSE_CHANNEL       0
#define BATT_DIVIDER             2                          /* 1:2 resistor divider */

/* ---- Analog ---- */
#define ADC_VREF_MV              3300
#define ADC_RESOLUTION           12                         /* 12-bit internal ADC */

/* ---- Timing ---- */
#define SYSCLK_HZ                80000000u                   /* 80 MHz */
#define APB1_HZ                  80000000u
#define SPI1_BAUD                8000000u                    /* 8 MHz SPI to ADC */
#define SPI2_BAUD                4000000u                    /* 4 MHz OLED */
#define SPI3_BAUD                12000000u                   /* 12 MHz SD */

/* ---- Measurement parameters ---- */
#define ARRAY_ELEMENTS           128                         /* photodiode array size */
#define NUM_BANDS                16                          /* spectral bands */
#define DARK_INTEG_MS            20
#define WHITE_INTEG_MS           50
#define NIR_INTEG_MS             50
#define MEAS_CYCLE_MS            300

/* ---- Battery ---- */
#define BATT_FULL_MV             4200
#define BATT_LOW_MV              3400
#define BATT_CRIT_MV             3100

/* ---- Storage ---- */
#define SD_SECTOR_SIZE           512
#define LOG_FILENAME             "chlormap.csv"
#define CALIB_FILENAME           "calib.bin"

/* ---- Flash calibration page ---- */
#define CALIB_FLASH_PAGE         62                          /* last page of 256K */
#define CALIB_MAGIC              0xCF4C1234
#define CALIB_ADDR               (0x08000000 + (CALIB_FLASH_PAGE * 2048))

/* ---- Author / license metadata ---- */
#define AUTHOR                   "jayis1"
#define FIRMWARE_LICENSE         "GPL-2.0"

#endif /* BOARD_H */