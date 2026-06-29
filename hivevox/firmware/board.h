/*
 * board.h — HiveVox hardware pin map and board constants
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * STM32L432KC (UFQFPN32) pin assignments for the HiveVox
 * acoustic beehive health monitor.
 */
#ifndef HIVEVOX_BOARD_H
#define HIVEVOX_BOARD_H

#include <stdint.h>

/* ---- MCU identity --------------------------------------------------- */
#define MCU_NAME        "STM32L432KC"
#define SYSCLK_HZ       80000000UL
#define LSE_HZ          32768UL
#define FLASH_SIZE_KB   256
#define SRAM_SIZE_KB    64

/* ---- Pin map -------------------------------------------------------- */
/*
 *  PA0  : ADC_IN5  — VBAT divider (1/2 LiFePO4)
 *  PA1  : GPIO_OUT — HX711 PD_SCK (power-down / clock)
 *  PA2  : USART2_TX— (debug log, optional)
 *  PA3  : USART2_RX— (debug log, optional)
 *  PA4  : GPIO_IN  — HX711 DOUT (data)
 *  PA5  : SPI1_SCK — (flash, optional)
 *  PA6  : SPI1_MISO— (flash, optional)
 *  PA7  : SPI1_MOSI— (flash, optional)
 *  PA8  : I2C3_SCL — SHT45 + VEML7700
 *  PA9  : I2C3_SDA
 *  PA10 : GPIO_OUT — MIC_EN (MEMS mic power enable)
 *  PA11 : I2S2_SD   — MEMS mic PDM data
 *  PA12 : I2S2_WS   — MEMS mic word select (LRCLK)
 *  PB3  : I2S2_CK   — MEMS mic bit clock
 *  PB4  : GPIO_OUT — SX1276 NSS (SPI CS)
 *  PB5  : GPIO_OUT — SX1276 RST
 *  PB6  : GPIO_IN  — SX1276 DIO0 (TX done IRQ)
 *  PB7  : GPIO_IN  — SX1276 DIO1 (RX done IRQ)
 *  PB8  : SPI2_SCK — SX1276 SPI clock (alt fn 5)
 *  PB12 : SPI2_NSS — (unused, we bit-bang NSS on PB4)
 *  PB13 : SPI2_SCK — alt mapping option
 *  PB14 : SPI2_MISO— SX1276 SPI MISO
 *  PB15 : SPI2_MOSI— SX1276 SPI MOSI
 *  PC14 : OSC32_IN— 32.768 kHz LSE crystal (RTC)
 *  PC15 : OSC32_OUT
 *  1-Wire bus: PB0 (open-drain, external 4.7k pullup to 3V3)
 */

/* GPIO port base offsets (for register.h) */
#define PORT_A  0
#define PORT_B  1
#define PORT_C  2

/* Pin enum for board_init helpers */
typedef enum {
    PIN_VBAT_ADC      = (PORT_A << 8) | 0,
    PIN_HX711_SCK     = (PORT_A << 8) | 1,
    PIN_UART_TX       = (PORT_A << 8) | 2,
    PIN_UART_RX       = (PORT_A << 8) | 3,
    PIN_HX711_DOUT    = (PORT_A << 8) | 4,
    PIN_FLASH_SCK     = (PORT_A << 8) | 5,
    PIN_FLASH_MISO    = (PORT_A << 8) | 6,
    PIN_FLASH_MOSI    = (PORT_A << 8) | 7,
    PIN_I2C_SCL       = (PORT_A << 8) | 8,
    PIN_I2C_SDA       = (PORT_A << 8) | 9,
    PIN_MIC_EN        = (PORT_A << 8) | 10,
    PIN_I2S_SD        = (PORT_A << 8) | 11,
    PIN_I2S_WS        = (PORT_A << 8) | 12,
    PIN_I2S_CK        = (PORT_B << 8) | 3,
    PIN_LORA_NSS      = (PORT_B << 8) | 4,
    PIN_LORA_RST      = (PORT_B << 8) | 5,
    PIN_LORA_DIO0     = (PORT_B << 8) | 6,
    PIN_LORA_DIO1     = (PORT_B << 8) | 7,
    PIN_SPI2_SCK      = (PORT_B << 8) | 13,
    PIN_SPI2_MISO     = (PORT_B << 8) | 14,
    PIN_SPI2_MOSI     = (PORT_B << 8) | 15,
    PIN_ONEWIRE       = (PORT_B << 8) | 0,
} board_pin_t;

/* ---- LoRa radio ----------------------------------------------------- */
#define LORA_FREQ_HZ        915000000UL   /* US band; strap to 868M for EU */
#define LORA_BW_HZ          125000
#define LORA_SF_DEFAULT     7             /* SF7 = fast, SF12 = far */
#define LORA_CODING_RATE    0x04          /* 4/5 */
#define LORA_TX_POWER_DBM   20
#define LORA_SYNC_WORD      0x34           /* private network */
#define LORA_PREAMBLE_LEN   8

/* ---- Power & timing ------------------------------------------------- */
#define VBAT_DIVIDER        2.0f           /* resistor divider ratio */
#define ADC_REF_MV          3000
#define BATTERY_FULL_MV     3600           /* LiFePO4 100% */
#define BATTERY_EMPTY_MV    2400           /* LiFePO4 0% */
#define SOLAR_GOOD_THRESHOLD_MV 3200       /* panel above this = charging */

/* ---- Acoustic ------------------------------------------------------- */
#define FFT_N               512
#define MIC_SAMPLE_RATE_HZ  16000
#define MIC_FRAMES_PER_CYCLE 4              /* 4 frames of 512 = 128 ms capture */
#define MIC_ENABLE_WARMUP_MS 50             /* mic power-up settling */

/* State machine timing */
#define DEFAULT_SAMPLE_INTERVAL_MIN  10
#define MIN_SAMPLE_INTERVAL_MIN       2
#define MAX_SAMPLE_INTERVAL_MIN       60

/* ---- Colony state enum --------------------------------------------- */
typedef enum {
    STATE_QUEENRIGHT = 0,
    STATE_QUEENLESS  = 1,
    STATE_SWARM_PREP = 2,
    STATE_WINTER      = 3,
    STATE_DEAD        = 4,
    STATE_UNKNOWN     = 5,
    STATE_BASELINE    = 6,   /* first 24h, no classification yet */
} colony_state_t;

/* ---- Alert flags (byte 11 of uplink) -------------------------------- */
#define ALERT_FLAG_ALERT        0x01   /* any anomaly requiring attention */
#define ALERT_FLAG_SOLAR_GOOD   0x02   /* solar panel charging */
#define ALERT_FLAG_PROBE_FAULT  0x04   /* a DS18B20 probe is disconnected */

/* ---- Payload sizes -------------------------------------------------- */
#define UPLINK_PAYLOAD_LEN   12
#define DOWNLINK_PAYLOAD_LEN  4

#endif /* HIVEVOX_BOARD_H */