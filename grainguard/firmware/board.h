/*
 * board.h — GrainGuard hardware pin definitions and board configuration
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Target: STM32WL55JCI6 (QFN-32, Cortex-M4 @ 48 MHz + LoRa SX1262)
 * Board:  GrainGuard In-Silo Grain Condition Probe (rev 1.0)
 */

#ifndef GRAINGUARD_BOARD_H
#define GRAINGUARD_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Clock configuration ---- */
#define BOARD_HSI_FREQ_HZ        48000000UL   /* HSI48 for MCU core */
#define BOARD_LSE_FREQ_HZ        32768UL      /* RTC crystal */
#define BOARD_HSE_FREQ_HZ        32000000UL   /* TCXO for LoRa radio */

/* ---- MCU port/pin assignments (STM32WL55JCI6 QFN-32) ---- */
/* Port A */
#define PA0__CO2_SDA             0    /* I2C1 SDA -> SCD41 + SHT45 + 24LC02 */
#define PA1__CO2_SCL             1    /* I2C1 SCL */
#define PA2__LORA_TX             2    /* USART2 TX (debug; optional) */
#define PA3__LORA_RX             3    /* USART2 RX */
#define PA4__SPI_NCS_FLASH       4    /* GPIO OUT -> W25R80 CS */
#define PA5__SPI_SCK             5    /* SPI1 SCK -> W25R80 */
#define PA6__SPI_MISO            6    /* SPI1 MISO */
#define PA7__SPI_MOSI            7    /* SPI1 MOSI */
#define PA8__ONEWIRE             8    /* GPIO OD PP -> DS18B20 data (1-Wire) */
#define PA9__AE_PGA_GAIN         9    /* GPIO OUT -> PGA gain select */
#define PA10__AE_MODE            10   /* GPIO OUT -> raw (1) / envelope (0) */
#define PA11__NFC_IRQ            11   /* GPIO IN (falling) -> ST25DV */
#define PA12__NFC_DISABLE        12   /* GPIO OUT -> ST25DV disable */
#define PA13__SWDIO              13   /* SWD */
#define PA14__SWCLK              14   /* SWD */
#define PA15__LED                15   /* GPIO OUT -> status LED */

/* Port B */
#define PB0__AE_ENVELOPE_ADC     0    /* ADC1 IN5 -> envelope signal */
#define PB1__AE_RAW_ADC          1    /* ADC1 IN6 -> raw AE (192 kS/s) */
#define PB2__BOOT1               2    /* Boot config */
#define PB3__LORA_BUSY            3    /* GPIO IN -> SX1262 Busy */
#define PB4__LORA_NCS             4    /* GPIO OUT -> SX1262 NSS */
#define PB5__LORA_RESET           5    /* GPIO OUT -> SX1262 NRST */
#define PB6__LORA_DIO1            6    /* GPIO IN (exti) -> SX1262 DIO1 */
#define PB7__NFC_LPDOWN           7    /* GPIO OUT -> ST25DV LPDOWN */
#define PB8__CO2_RESET            8    /* GPIO OUT -> SCD41 reset */
#define PB9__BAT_DIV              9    /* ADC1 IN9 -> battery divider */
#define PB10__I2C1_ALT_SDA        10   /* (reserved, alt I2C) */
#define PB11__I2C1_ALT_SCL        11   /* (reserved, alt I2C) */
#define PB12__SPI2_NCS_RF          12   /* (unused; SX on same SPI1) */
#define PB13__LORA_SCK             13   /* SPI1 SCK (alt, unused; radio uses sub-GHz internal) */
#define PB14__LORA_MISO            14   /* SPI1 MISO (alt) */
#define PB15__LORA_MOSI            15   /* SPI1 MOSI (alt) */

/* Port C */
#define PC0__TEMP_SUPPLY_EN        0    /* GPIO OUT -> DS18B20 strong-pullup MOSFET gate */
#define PC1__CO2_SUPPLY_EN         1    /* GPIO OUT -> SCD41 power gate */
#define PC2__AE_SUPPLY_EN          2    /* GPIO OUT -> acoustic AFE power gate */
#define PC3__BUZZER                3    /* GPIO OUT/PWM -> commissioning buzzer */
#define PC4__SUPER_CAP_SENSE       4    /* ADC1 IN13 -> supercap voltage */
#define PC6__LORA_RF_SWITCH_CTRL   6    /* GPIO OUT -> RF switch (TX/RX) */

/* ---- I2C addresses ---- */
#define CO2_I2C_ADDR          0x62    /* SCD41 (7-bit) */
#define SHT45_I2C_ADDR        0x44    /* SHT45 (7-bit) */
#define EEPROM_I2C_ADDR       0x50    /* 24LC02 (7-bit) */
#define NFC_I2C_ADDR          0x53    /* ST25DV (7-bit) */

/* ---- DS18B20 1-Wire ---- */
#define ONEWIRE_MAX_DEVICES   9
#define DS18B20_CMD_CONVERT_T  0x44
#define DS18B20_CMD_READ_SCRATCH 0xBE
#define DS18B20_CMD_MATCH_ROM  0x55
#define DS18B20_CMD_SKIP_ROM   0xCC
#define DS18B20_CMD_READ_ROM   0x33
#define DS18B20_CMD_SEARCH_ROM 0xF0
#define DS18B20_CONV_TIME_MS  750    /* 12-bit conversion */

/* ---- SCD41 CO2 sensor ---- */
#define SCD41_CMD_START_PERIODIC   0x21B1
#define SCD41_CMD_READ_MEAS        0xEC05
#define SCD41_CMD_STOP             0x3FFF
#define SCD41_CMD_REINIT           0x3646
#define SCD41_CMD_SINGLE_SHOT      0x2196
#define SCD41_MEAS_TIME_MS        5000   /* single-shot full cycle */
#define SCD41_WARMUP_MS            1000

/* ---- SHT45 RH+T sensor ---- */
#define SHT45_CMD_MEASURE_NOCLKSTRETCH 0x2400
#define SHT45_CMD_SOFTRESET       0x30A2
#define SHT45_MEAS_TIME_MS        10

/* ---- W25R80 SPI flash (8 MB) ---- */
#define W25R80_CMD_READ           0x03
#define W25R80_CMD_PAGE_PROGRAM   0x02
#define W25R80_CMD_SECTOR_ERASE   0x20
#define W25R80_CMD_READ_STATUS    0x05
#define W25R80_CMD_WRITE_ENABLE   0x06
#define W25R80_CMD_READ_ID        0x9F
#define W25R80_PAGE_SIZE         256
#define W25R80_SECTOR_SIZE       4096
#define W25R80_FLASH_SIZE        (8 * 1024 * 1024)

/* ---- Acoustic AFE ---- */
#define AE_ADC_SAMPLE_RATE_RAW   192000UL  /* Hz, raw mode */
#define AE_ADC_SAMPLE_RATE_ENV   1000UL    /* Hz, envelope mode */
#define AE_ADC_RESOLUTION        12        /* bits */
#define AE_WINDOW_S              300       /* 5-min listening window */
#define AE_EVENT_MIN_DUR_MS      2
#define AE_EVENT_RATIO           3     /* short/long RMS ratio for event */
#define AE_SHORT_WIN_MS          10
#define AE_LONG_WIN_MS           500

/* ---- LoRa mesh ---- */
#define LORA_FREQ_HZ_EU868       868100000UL
#define LORA_FREQ_HZ_US915        915000000UL
#define LORA_BANDWIDTH_KHZ        125
#define LORA_SF                   7
#define LORA_CODING_RATE          5     /* 4/5 */
#define LORA_TX_POWER_DBM        22
#define LORA_MAX_PACKET_SIZE      32
#define MESH_MAX_HOPS             8
#define MESH_RECENT_CACHE_SIZE    32

/* ---- Power management ---- */
#define VBAT_DIVIDER_RATIO        2     /* R1=R2=1M -> ratio 2 */
#define VBAT_FULL_MV             3600  /* LiSOCl2 fresh */
#define VBAT_LOW_MV              3200
#define VBAT_CRIT_MV             3000

/* ---- Scheduler intervals (seconds) ---- */
#define SCHED_INTERVAL_T_RH_CO2  900    /* 15 min */
#define SCHED_INTERVAL_ACOUSTIC   21600  /* 6 hr */
#define SCHED_INTERVAL_TX         1800   /* 30 min */
#define SCHED_INTERVAL_STORAGE   900    /* 15 min (log each measurement) */

/* ---- EEPROM layout (24LC02, 256 bytes) ---- */
#define EEPROM_ADDR_SERIAL       0x00   /* 8 bytes ASCII serial */
#define EEPROM_ADDR_GRAIN_TYPE   0x08   /* 1 byte: 1=wheat,2=corn,... */
#define EEPROM_ADDR_SAFE_MC      0x09   /* 1 byte: ×10 (135=13.5%) */
#define EEPROM_ADDR_SRI_THRESH_LO 0x0A   /* 1 byte: caution threshold */
#define EEPROM_ADDR_SRI_THRESH_HI 0x0B   /* 1 byte: critical threshold */
#define EEPROM_ADDR_MEAS_INTERVAL 0x0C  /* 2 bytes: seconds */
#define EEPROM_ADDR_CAL_T_OFFSET 0x0E   /* 9× int8_t DS18B20 offsets */
#define EEPROM_ADDR_CAL_RH_OFFSET 0x17  /* 1 byte: SHT45 RH offset */
#define EEPROM_ADDR_CAL_CO2_OFFSET 0x18 /* 2 bytes: int16 SCD41 offset */
#define EEPROM_ADDR_AES_KEY      0x20   /* 16 bytes: AES-128 key */
#define EEPROM_ADDR_MAGIC        0xF0   /* 4 bytes: 0x47523147 ("GR1G") */

/* ---- Grain types (for EMC) ---- */
#define GRAIN_WHEAT    1
#define GRAIN_CORN     2
#define GRAIN_BARLEY   3
#define GRAIN_RICE     4
#define GRAIN_OATS     5
#define GRAIN_SOYBEAN  6
#define GRAIN_COUNT    6

/* ---- SRI thresholds (defaults) ---- */
#define SRI_DEFAULT_CAUTION  40
#define SRI_DEFAULT_CRITICAL 70

/* ---- Status LED blink codes ---- */
#define LED_BLINK_BOOT_OK     2
#define LED_BLINK_CONFIG_MODE 3
#define LED_BLINK_TX          1
#define LED_BLINK_ERROR       5

/* ---- Utility macros ---- */
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) (MAX((lo), MIN((hi), (x))))

/* ---- Inline delay ---- */
static inline void delay_ms(volatile uint32_t ms) {
    /* Filled by timer; approximate: 48000 cycles/ms at 48 MHz */
    volatile uint32_t cycles = ms * 12000; /* ~12k iterations/ms at -O2 */
    while (cycles--) { __asm volatile("nop"); }
}

#endif /* GRAINGUARD_BOARD_H */