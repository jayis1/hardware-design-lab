/*
 * board.h — Hardware pin assignments and peripheral mappings for HivePulse
 *
 * Target MCU: STM32H733VGT6 (Cortex-M7 @ 280 MHz)
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef BOARD_H
#define BOARD_H

#include "registers.h"

/* ---- Clock Configuration ---- */
#define HSE_VALUE           16000000UL   /* External 16 MHz crystal */
#define LSE_VALUE           32768UL      /* 32.768 kHz RTC crystal */
#define SYSCLK_FREQ         280000000UL  /* System clock: 280 MHz */
#define AHB_FREQ            280000000UL  /* AHB bus: 280 MHz */
#define APB1_FREQ           140000000UL  /* APB1: 140 MHz (LoRa, UART, I2C) */
#define APB2_FREQ           140000000UL  /* APB2: 140 MHz (SPI, ADC, TIM) */
#define APB4_FREQ           140000000UL  /* APB4: 140 MHz (SYSCFG, LPUART) */

/* ---- GPIO Pin Assignments ---- */

/* Audio Codec (CS42448) — I2S2 on DMA1 Stream3 */
#define I2S2_WS_PIN         GPIO_PIN_4   /* PORTI */
#define I2S2_SCK_PIN        GPIO_PIN_5   /* PORTI */
#define I2S2_SDO_PIN        GPIO_PIN_6   /* PORTI — data from MCU to codec */
#define I2S2_SDI_PIN        GPIO_PIN_7   /* PORTI — data from codec to MCU */
#define I2S2_MCLK_PIN       GPIO_PIN_3   /* PORTC — master clock output */
#define CS42448_RST_PIN     GPIO_PIN_0   /* PORTB — codec reset (active low) */
#define CS42448_I2C_SCL     GPIO_PIN_10  /* PORTB — I2C2 for codec config */
#define CS42448_I2C_SDA     GPIO_PIN_11  /* PORTB */

/* LoRa Radio (SX1262) — SPI1 */
#define LORA_SPI_NSS_PIN    GPIO_PIN_4   /* PORTA — SPI1 CS */
#define LORA_SPI_SCK_PIN    GPIO_PIN_5   /* PORTA */
#define LORA_SPI_MISO_PIN   GPIO_PIN_6   /* PORTA */
#define LORA_SPI_MOSI_PIN   GPIO_PIN_7   /* PORTA */
#define LORA_BUSY_PIN       GPIO_PIN_1   /* PORTB — SX1262 busy signal */
#define LORA_DIO1_PIN       GPIO_PIN_2   /* PORTB — SX1262 interrupt */
#define LORA_NRST_PIN       GPIO_PIN_10  /* PORTA — SX1262 reset */
#define LORA_ANT_SW_PIN     GPIO_PIN_3   /* PORTB — RF switch control (RX/TX) */

/* BLE Co-processor (CC2640R2F) — UART4 */
#define BLE_UART_TX_PIN     GPIO_PIN_0   /* PORTC — UART4 TX */
#define BLE_UART_RX_PIN     GPIO_PIN_1   /* PORTC — UART4 RX */
#define BLE_UART_RTS_PIN    GPIO_PIN_2   /* PORTC — RTS (flow control) */
#define BLE_UART_CTS_PIN    GPIO_PIN_3   /* PORTC — CTS */
#define BLE_nRESET_PIN      GPIO_PIN_5   /* PORTB — CC2640 reset */
#define BLE_RDY_PIN         GPIO_PIN_6   /* PORTB — CC2640 host-ready interrupt */

/* Temperature Sensors (DS18B20) — 1-Wire on USART3 (half-duplex) */
#define ONEWIRE_PIN         GPIO_PIN_8   /* PORTC — USART3 TX in OD mode */
#define ONEWIRE_PORT        GPIOC

/* Humidity Sensor (SHT45) — I2C1 */
#define SHT45_I2C_SCL       GPIO_PIN_8   /* PORTB */
#define SHT45_I2C_SDA       GPIO_PIN_9   /* PORTB */

/* CO2 Sensor (SCD41) — shared I2C1 */
/* Same I2C1 bus as SHT45 */

/* Load Cell ADC (HX711) — GPIO bit-bang */
#define HX711_SCK_PIN       GPIO_PIN_12  /* PORTB — serial clock */
#define HX711_DOUT_PIN      GPIO_PIN_13  /* PORTB — data output */
#define HX711_SCK_PORT      GPIOB
#define HX711_DOUT_PORT     GPIOB

/* IMU (LSM6DSO32X) — SPI3 */
#define IMU_SPI_NSS_PIN     GPIO_PIN_15  /* PORTA — SPI3 CS */
#define IMU_SPI_SCK_PIN     GPIO_PIN_10  /* PORTC */
#define IMU_SPI_MISO_PIN    GPIO_PIN_11  /* PORTC */
#define IMU_SPI_MOSI_PIN    GPIO_PIN_12  /* PORTC */
#define IMU_INT1_PIN        GPIO_PIN_14  /* PORTB — IMU interrupt 1 */

/* Bee Counter — 16 IR break-beam sensors on GPIO EXTI */
#define BEE_COUNTER_PORT    GPIOD
#define BEE_COUNTER_BASE    GPIO_PIN_0   /* PD0-PD15 = 16 channels */
#define BEE_COUNTER_DIR_PIN GPIO_PIN_7   /* PORTC — direction latch output */

/* Power Management (BQ25895) — I2C1 shared */
#define BQ25895_INT_PIN     GPIO_PIN_15  /* PORTB — charger interrupt */
#define SOLAR_VSENSE_PIN    GPIO_PIN_0   /* PORTA — ADC1 CH0, solar voltage */
#define BATT_VSENSE_PIN     GPIO_PIN_1   /* PORTA — ADC1 CH1, battery voltage */
#define BATT_ISENSE_PIN     GPIO_PIN_2   /* PORTA — ADC1 CH2, charge current */

/* Status LEDs */
#define LED_STATUS_PIN      GPIO_PIN_7   /* PORTB — green: OK / heartbeat */
#define LED_ALERT_PIN       GPIO_PIN_8   /* PORTB — red: alert / fault */
#define LED_LORA_PIN        GPIO_PIN_9   /* PORTB — blue: LoRa TX */

/* Debug / Test */
#define DEBUG_UART_TX       GPIO_PIN_8   /* PORTC — LPUART1 for debug log */
#define DEBUG_UART_RX       GPIO_PIN_9   /* PORTC */

/* ---- Peripheral Assignments ---- */
#define AUDIO_I2S           SPI2         /* I2S mode on SPI2 */
#define AUDIO_DMA_STREAM    DMA1_Stream3
#define AUDIO_DMA_CHANNEL   DMA_CHANNEL_0
#define AUDIO_IRQ           SPI2_IRQn

#define LORA_SPI            SPI1
#define LORA_DMA_RX         DMA1_Stream0
#define LORA_DMA_TX         DMA1_Stream1

#define BLE_UART            UART4
#define BLE_DMA_RX          DMA1_Stream2

#define ENV_I2C             I2C1         /* SHT45 + SCD41 + BQ25895 */
#define CODEC_I2C           I2C2         /* CS42448 config */

#define IMU_SPI             SPI3
#define ONEWIRE_UART        USART3       /* 1-Wire via UART half-duplex */

#define DEBUG_LPUART        LPUART1

/* ---- DMA Configuration ---- */
#define AUDIO_DMA_IRQn      DMA1_Stream3_IRQn
#define LORA_DMA_RX_IRQn    DMA1_Stream0_IRQn
#define BLE_DMA_RX_IRQn     DMA1_Stream2_IRQn

/* ---- Timing Constants ---- */
#define SYSTICK_HZ          1000         /* 1 ms tick */
#define AUDIO_SAMPLE_RATE   48000        /* 48 kHz sampling */
#define AUDIO_FFT_SIZE      4096         /* 4096-point FFT */
#define AUDIO_WINDOW_MS     85           /* 4096/48000 = 85.3 ms per window */
#define AUDIO_DMA_BUF_SIZE  4096         /* Half-word per channel, 4 channels */

#define LORA_TX_INTERVAL_S  14400        /* 4 hours between routine uplinks */
#define LISTEN_INTERVAL_S   300          /* 5 min between listening windows */
#define LISTEN_DURATION_S   30           /* 30 sec listening window */
#define WINTER_LISTEN_INT_S 900          /* 15 min in winter mode */

#define BATTERY_LOW_MV      2900         /* LiFePO4 low threshold */
#define BATTERY_CRIT_MV     2600         /* Critical - shutdown non-essential */

/* ---- Board Initialization ---- */
void board_init(void);
void board_clock_config(void);
void board_gpio_config(void);
void board_dma_config(void);

/* Helper macros */
#define ARRAY_SIZE(x)       (sizeof(x) / sizeof((x)[0]))
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))

#endif /* BOARD_H */