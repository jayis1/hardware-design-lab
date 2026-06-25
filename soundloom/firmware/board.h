/*
 * board.h — SoundLoom hardware pin & peripheral map
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * SoundLoom: Bioacoustic Soil Health Monitor
 * Target: STM32H733VIH6 (Cortex-M7, 480 MHz)
 */

#ifndef SOUNDLOOM_BOARD_H
#define SOUNDLOOM_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- MCU identity ---- */
#define MCU_NAME        "STM32H733VIH6"
#define SYSCLK_HZ       480000000u
#define HCLK_HZ         480000000u
#define PCLK1_HZ        120000000u   /* APB1 */
#define PCLK2_HZ        120000000u   /* APB2 */
#define SPI_BAUD_HZ     12000000u     /* ADS131M08 SPI: 12 MHz */

/* ---- Acoustic acquisition: ADS131M08 (4 devices, 32 channels, 24 used) ---- */
#define ADC_NUM_DEVICES     4
#define ADC_CHS_PER_DEVICE  8
#define ADC_TOTAL_CHS       (ADC_NUM_DEVICES * ADC_CHS_PER_DEVICE)  /* 32 */
#define ADC_USED_CHS        24    /* 12 tri-axial geophones */
#define ADC_SAMPLE_RATE     8000  /* Hz per channel */
#define ADC_BLOCK_SAMPLES   1024  /* samples per DMA block */
#define ADC_BLOCK_MS        (ADC_BLOCK_SAMPLES * 1000u / ADC_SAMPLE_RATE) /* 128 ms */

/* SPI2 — ADS131M08 daisy chain */
#define ADC_SPI             SPI2
#define ADC_SPI_CLK_ENABLE  __HAL_RCC_SPI2_CLK_ENABLE
#define ADC_SPI_AF         GPIO_AF5_SPI2
#define ADC_SPI_SCK_PORT   GPIOB
#define ADC_SPI_SCK_PIN    GPIO_PIN_10
#define ADC_SPI_MISO_PORT  GPIOC
#define ADC_SPI_MISO_PIN   GPIO_PIN_2
#define ADC_SPI_MOSI_PORT  GPIOC
#define ADC_SPI_MOSI_PIN   GPIO_PIN_3
#define ADC_NSS_PORT       GPIOB
#define ADC_NSS_PIN        GPIO_PIN_12

/* ADS131M08 control pins */
#define ADC_START_PORT     GPIOB
#define ADC_START_PIN     GPIO_PIN_15
#define ADC_SYNC_PORT     GPIOE
#define ADC_SYNC_PIN      GPIO_PIN_15
#define ADC_DRDY_PORT     GPIOB
#define ADC_DRDY_PIN      GPIO_PIN_4    /* EXTI4 */
#define ADC_RST_PORT      GPIOC
#define ADC_RST_PIN       GPIO_PIN_13

/* ---- LoRaWAN: SX1262 on SPI1 ---- */
#define LORA_SPI           SPI1
#define LORA_NSS_PORT      GPIOA
#define LORA_NSS_PIN       GPIO_PIN_4
#define LORA_BUSY_PORT    GPIOB
#define LORA_BUSY_PIN      GPIO_PIN_0
#define LORA_DIO1_PORT     GPIOB
#define LORA_DIO1_PIN      GPIO_PIN_1   /* EXTI1 */
#define LORA_RST_PORT     GPIOB
#define LORA_RST_PIN      GPIO_PIN_2
#define LORA_ANT_SW_PORT  GPIOB
#define LORA_ANT_SW_PIN   GPIO_PIN_3

/* ---- BLE: STM32WB55 co-processor over USART3 ---- */
#define BLE_UART           USART3
#define BLE_BAUD            115200u
#define BLE_TX_PORT        GPIOB
#define BLE_TX_PIN         GPIO_PIN_9
#define BLE_RX_PORT        GPIOB
#define BLE_RX_PIN         GPIO_PIN_8
#define BLE_CTS_PORT       GPIOB
#define BLE_CTS_PIN        GPIO_PIN_7
#define BLE_NRESET_PORT    GPIOB
#define BLE_NRESET_PIN     GPIO_PIN_6

/* ---- SD card on SDMMC1 ---- */
#define SD_SDMMC           SDMMC1

/* ---- Environmental sensors ---- */
/* Teros-12 soil moisture/EC/temp: SDI-12 on UART4 */
#define SDI12_UART         UART4
#define SDI12_TX_PORT      GPIOC
#define SDI12_TX_PIN       GPIO_PIN_10
#define SDI12_RX_PORT      GPIOC
#define SDI12_RX_PIN       GPIO_PIN_11
#define SDI12_DIR_PORT     GPIOC
#define SDI12_DIR_PIN      GPIO_PIN_12   /* 0=RX, 1=TX */

/* GMP343 CO2 NDIR: I2C1 (0x48) */
#define CO2_I2C            I2C1
#define CO2_I2C_ADDR       0x48u
#define CO2_I2C_SPEED_HZ   100000u
#define CO2_SCL_PORT      GPIOB
#define CO2_SCL_PIN       GPIO_PIN_8
#define CO2_SDA_PORT      GPIOB
#define CO2_SDA_PIN       GPIO_PIN_9

/* SHT45 air temp/RH: I2C1 (0x44) */
#define SHT45_I2C_ADDR     0x44u

/* DS18B20 depth thermometers: 1-Wire on GPIO */
#define ONEWIRE_PORT       GPIOC
#define ONEWIRE_PIN        GPIO_PIN_5
#define DS18B20_COUNT      4

/* LSM6DSO32 IMU: SPI3 */
#define IMU_SPI            SPI3
#define IMU_NSS_PORT       GPIOA
#define IMU_NSS_PIN        GPIO_PIN_15

/* ---- Power management ---- */
#define POWER_VBAT_SENSE_PORT  GPIOC
#define POWER_VBAT_SENSE_PIN   GPIO_PIN_1   /* ADC1 channel 1 */
#define POWER_EN_3V3_PORT      GPIOB
#define POWER_EN_3V3_PIN       GPIO_PIN_14   /* main rail enable */
#define POWER_EN_SENSORS_PORT  GPIOC
#define POWER_EN_SENSORS_PIN   GPIO_PIN_0   /* sensor rail enable */
#define POWER_EN_ADC_PORT      GPIOC
#define POWER_EN_ADC_PIN       GPIO_PIN_3   /* ADC front-end enable */
#define POWER_SOLAR_SENSE_PORT GPIOC
#define POWER_SOLAR_SENSE_PIN  GPIO_PIN_4   /* ADC1 channel 4 */

/* ---- LEDs & status ---- */
#define LED_STATUS_PORT     GPIOE
#define LED_STATUS_PIN      GPIO_PIN_1
#define LED_FAULT_PORT      GPIOE
#define LED_FAULT_PIN       GPIO_PIN_2

/* ---- RTC ---- */
#define RTC_BACKUP_DOMAIN

/* ---- Operating cadence (milliseconds) ---- */
#define CADENCE_HIGHRES_MS       (15u * 60u * 1000u)   /* 15 min */
#define CADENCE_STANDARD_MS      (30u * 60u * 1000u)   /* 30 min */
#define CADENCE_LONGTERM_MS      (2u  * 60u * 60u * 1000u) /* 2 h  */
#define LISTEN_WINDOW_MS         30000u                /* 30 s active listen */
#define DEFAULT_CADENCE_MS       CADENCE_STANDARD_MS

/* ---- Classifier ---- */
#define CLS_NUM_CLASSES     10
#define CLS_MEL_BINS        40
#define CLS_FRAMES          8
#define CLS_CONF_THRESHOLD  60  /* percent */

/* ---- Beamformer ---- */
#define BF_NUM_RECEIVERS    12
#define BF_NUM_PAIRS        (BF_NUM_RECEIVERS * (BF_NUM_RECEIVERS - 1) / 2)  /* 66 */
#define BF_MAX_ITER         5
#define BF_MAX_SOURCES      64  /* per listen window */

/* ---- Logging / storage ---- */
#define LOG_BUF_EVENTS      512
#define LOG_MAX_EVENTS_SD   200000u
#define LORA_UPLINK_LEN      51u   /* bytes */
#define LORA_UPLINK_PERIOD   15u   /* minutes default */

/* ---- Software version ---- */
#define FW_VERSION_MAJOR  1
#define FW_VERSION_MINOR  0
#define FW_VERSION_PATCH  0
#define FW_AUTHOR         "jayis1"
#define FW_NAME           "SoundLoom"

#endif /* SOUNDLOOM_BOARD_H */