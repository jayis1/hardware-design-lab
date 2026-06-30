/*
 * board.h — Hardware pin/peripheral mapping for SpectraPest
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Defines all GPIO pin assignments, peripheral mappings, and board-level
 * constants for the SpectraPest multispectral+acoustic pest identifier.
 *
 * MCU: STM32H753VIT6 (LQFP100)
 * Board rev: 1.0
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------- *
 *  Board identification
 * ----------------------------------------------------------------- */
#define BOARD_NAME        "SpectraPest"
#define BOARD_VERSION     "1.0"
#define BOARD_AUTHOR      "jayis1"

/* ----------------------------------------------------------------- *
 *  Clock configuration
 * ----------------------------------------------------------------- */
#define HSE_VALUE          16000000UL   /* 16 MHz external crystal */
#define LSE_VALUE          32768UL      /* 32.768 kHz RTC crystal */
#define SYSCLK_FREQ        480000000UL /* 480 MHz system clock */
#define HCLK_FREQ          240000000UL  /* 240 MHz AHB */
#define PCLK1_FREQ         120000000UL  /* 120 MHz APB1 */
#define PCLK2_FREQ         120000000UL  /* 120 MHz APB2 */
#define PLLM               4
#define PLLN               120
#define PLLP               2
#define PLLQ               5
#define PLLR               2

/* ----------------------------------------------------------------- *
 *  GPIO port/pin definitions  (Port, Pin)
 * ----------------------------------------------------------------- */

/* --- LoRa SX1262 --- */
#define LORA_NSS_PORT       GPIOA
#define LORA_NSS_PIN        4
#define LORA_RST_PORT       GPIOA
#define LORA_RST_PIN        3
#define LORA_DIO1_PORT      GPIOB
#define LORA_DIO1_PIN       0
#define LORA_BUSY_PORT      GPIOB
#define LORA_BUSY_PIN       1
#define LORA_ANT_SW_PORT    GPIOC
#define LORA_ANT_SW_PIN     13

/* --- External SPI Flash (W25Q128) --- */
#define FLASH_NSS_PORT      GPIOA
#define FLASH_NSS_PIN       8
#define FLASH_HOLD_PORT     GPIOA
#define FLASH_HOLD_PIN      9
#define FLASH_WP_PORT       GPIOA
#define FLASH_WP_PIN        10

/* --- FRAM (MB85RS2MTA) --- */
#define FRAM_NSS_PORT       GPIOB
#define FRAM_NSS_PIN        12
#define FRAM_HOLD_PORT      GPIOB
#define FRAM_HOLD_PIN       13

/* --- Filter wheel stepper (STSPIN220) --- */
#define STEP_STEP_PORT      GPIOC
#define STEP_STEP_PIN       6
#define STEP_DIR_PORT        GPIOC
#define STEP_DIR_PIN        7
#define STEP_EN_PORT        GPIOC
#define STEP_EN_PIN         8
#define STEP_MS1_PORT        GPIOC
#define STEP_MS1_PIN        9
#define STEP_MS2_PORT        GPIOC
#define STEP_MS2_PIN        10
#define STEP_HOME_PORT      GPIOC     /* home position optical sensor */
#define STEP_HOME_PIN      11

/* --- IMX519 Camera --- */
/* Uses MIPI CSI-2 hardware lanes — no GPIO needed for data */
#define CAM_RESET_PORT      GPIOB
#define CAM_RESET_PIN       5
#define CAM_POWER_PORT      GPIOB
#define CAM_POWER_PIN       6
#define CAM_FLASH_PORT      GPIOB    /* white LED strobe */
#define CAM_FLASH_PIN       7
#define CAM_IR_LED_PORT     GPIOB    /* 850nm IR LED */
#define CAM_IR_LED_PIN      8

/* --- I2S MEMS Microphone (SPH0645) --- */
/* Uses hardware I2S2: CK = PB10, WS = PB12, SD = PB11 */
/* (Note: WS pin conflicts with FRAM NSS on rev 1.0 — WS moved to PC0) */
#define MIC_LR_PORT         GPIOC
#define MIC_LR_PIN          0
#define MIC_SHUTDOWN_PORT   GPIOC
#define MIC_SHUTDOWN_PIN     1

/* --- ESP32-C3 co-processor (UART4) --- */
#define ESP_UART_TX_PORT    GPIOC
#define ESP_UART_TX_PIN     10
#define ESP_UART_RX_PORT    GPIOC
#define ESP_UART_RX_PIN     11
#define ESP_BOOT_PORT       GPIOC
#define ESP_BOOT_PIN        12
#define ESP_EN_PORT         GPIOD
#define ESP_EN_PIN          2

/* --- GNSS NEO-M9N (USART2) --- */
#define GNSS_UART_TX_PORT   GPIOA
#define GNSS_UART_TX_PIN    2
#define GNSS_UART_RX_PORT   GPIOA
#define GNSS_UART_RX_PIN    3
#define GNSS_EN_PORT        GPIOA
#define GNSS_EN_PIN         5

/* --- PMIC MAX77654 + Fuel Gauge MAX17055 --- */
/* Both on I2C1 (PB8 SDA / PB9 SCL) */
#define PMIC_INT_PORT       GPIOB
#define PMIC_INT_PIN        14

/* --- Environmental sensors (I2C1) --- */
/* SHT45, LPS22HB, VEML7700, SCD41 all on I2C1 */
#define ENV_INT_PORT        GPIOB
#define ENV_INT_PIN         15

/* --- User LED + button --- */
#define LED_STATUS_PORT     GPIOE
#define LED_STATUS_PIN      0
#define LED_ACTIVITY_PORT   GPIOE
#define LED_ACTIVITY_PIN    1
#define LED_ERROR_PORT       GPIOE
#define LED_ERROR_PIN       2
#define BTN_USER_PORT       GPIOE
#define BTN_USER_PIN        3

/* ----------------------------------------------------------------- *
 *  I2C device addresses (7-bit)
 * ----------------------------------------------------------------- */
#define I2C_ADDR_SHT45      0x44
#define I2C_ADDR_LPS22HB    0x5C
#define I2C_ADDR_VEML7700   0x10
#define I2C_ADDR_SCD41      0x62
#define I2C_ADDR_MAX17055   0x36
#define I2C_ADDR_MAX77654   0x40

/* ----------------------------------------------------------------- *
 *  SPI peripheral assignments
 * ----------------------------------------------------------------- */
#define SPI_LORA            SPI1   /* PA5/PA6/PA7 (SCK/MISO/MOSI) */
#define SPI_FLASH           SPI1   /* Shared with LoRa via NSS */
#define SPI_FRAM            SPI2   /* PB14/PB15/PB3 alternate */

/* ----------------------------------------------------------------- *
 *  DMA channels
 * ----------------------------------------------------------------- */
#define DMA_STREAM_CAM      DMA1_Stream3   /* DCMI -> SRAM */
#define DMA_STREAM_MIC       DMA1_Stream4   /* I2S2 -> SRAM */
#define DMA_STREAM_LORA_SPI  DMA1_Stream5   /* SPI1 TX */
#define DMA_STREAM_FLASH_SPI DMA1_Stream6   /* SPI1 RX (shared) */
#define DMA_STREAM_FRAM_SPI  DMA1_Stream7   /* SPI2 TX */

/* ----------------------------------------------------------------- *
 *  Memory allocation constants
 * ----------------------------------------------------------------- */
#define SRAM_BASE            0x20000000UL
#define SRAM_SIZE            (1024 * 1024)  /* 1 MB */
#define HEAP_SIZE            (64 * 1024)
#define STACK_SIZE           (64 * 1024)
#define AUDIO_BUF_SIZE       (3 * 96000 * 2)  /* 3s @ 96kHz, 16-bit = 576 KB */
#define IMAGE_BUF_SIZE       (128 * 96 * 2)    /* Downsampled frame: 128x96, 16-bit */
#define ML_MODEL_SIZE         (128 * 1024)     /* 128 KB model in flash */
#define EVENT_LOG_FRAM_SIZE  (256 * 1024)      /* 256 KB FRAM for event log */

/* ----------------------------------------------------------------- *
 *  Operational parameters
 * ----------------------------------------------------------------- */
#define DETECTION_INTERVAL_SEC   60      /* Default: detect every 60 s */
#define LORA_FREQ_HZ             915000000  /* US ISM band */
#define LORA_BW_KHZ              125
#define LORA_SF                  7
#define LORA_TX_POWER_DBM        20
#define MESH_TTL_MAX             8
#define MESH_MAX_NODES           64
#define BATTERY_LOW_MV           5800    /* 5.8V = low (2S LiFePO4) */
#define BATTERY_CRIT_MV          5400
#define SOLAR_MIN_MV             100     /* Below this = nighttime */
#define TEMP_MIN_INSECT_C        10.0f   /* Below 10C: insects inactive */

/* ----------------------------------------------------------------- *
 *  Filter wheel bands
 * ----------------------------------------------------------------- */
typedef enum {
    BAND_450 = 0,   /* Blue */
    BAND_530,        /* Green */
    BAND_660,        /* Red */
    BAND_740,        /* Red-edge */
    BAND_810,        /* NIR-1 */
    BAND_850,        /* NIR-2 */
    BAND_COUNT
} filter_band_t;

#define FILTER_STEPS_PER_BAND   200   /* microsteps per band switch */

/* ----------------------------------------------------------------- *
 *  Detection result structure
 * ----------------------------------------------------------------- */
#define SPECIES_COUNT    60
#define SPECIES_NONE     60  /* index 60 = "no pest" */

typedef struct {
    uint32_t timestamp;
    float    latitude;
    float    longitude;
    uint8_t  species_id;
    float    confidence;
    uint8_t  severity;        /* 0=none, 1=low, 2=med, 3=high */
    float    wingbeat_hz;
    float    ndvi;
    float    ndre;
    float    damage_area_pct;
    float    temp_c;
    float    humidity_pct;
    uint16_t co2_ppm;
    uint8_t  node_id;
} detection_event_t;

#define SEVERITY_NONE  0
#define SEVERITY_LOW   1
#define SEVERITY_MED   2
#define SEVERITY_HIGH  3

/* ----------------------------------------------------------------- *
 *  Power management states
 * ----------------------------------------------------------------- */
typedef enum {
    PWR_SLEEP = 0,      /* Deep sleep, ~0.8 mA */
    PWR_WAKE_ENV,        /* Environmental check, ~12 mA */
    PWR_ACTIVE_CAPTURE,  /* Full detection cycle, ~350 mA */
    PWR_MESH_RX,         /* LoRa receive window, ~18 mA */
    PWR_CHARGING,        /* Solar charging active */
} power_state_t;

/* ----------------------------------------------------------------- *
 *  Function prototypes (board-level)
 * ----------------------------------------------------------------- */
void board_init(void);
void board_led_set(uint8_t led, uint8_t on);
void board_enter_sleep(void);
void board_wakeup(void);
uint32_t board_get_tick_ms(void);
void board_delay_ms(uint32_t ms);
uint16_t board_read_battery_mv(void);
uint16_t board_read_solar_mv(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */