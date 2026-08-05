/*
 * board.h — FrostSentinel pin map, clock configuration, and constants
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef FROSTSENTINEL_BOARD_H
#define FROSTSENTINEL_BOARD_H

#include <stdint.h>
#include "registers.h"

/* ------------------------------------------------------------------ */
/*  Board identity                                                     */
/* ------------------------------------------------------------------ */
#define BOARD_NAME      "FrostSentinel"
#define BOARD_AUTHOR    "jayis1"
#define BOARD_VERSION   "1.0.0"
#define BOARD_COPYRIGHT "Copyright (C) 2026 jayis1"

/* ------------------------------------------------------------------ */
/*  Clock tree: HSI16 → PLL1 (×20 / ÷2) → SYSCLK 160 MHz               */
/*  AHB = SYSCLK / 1 = 160 MHz                                         */
/*  APB1 = SYSCLK / 1 = 160 MHz                                        */
/*  APB2 = SYSCLK / 1 = 160 MHz                                        */
/*  ADC clock = HSI16 (independent)                                    */
/* ------------------------------------------------------------------ */
#define SYSCLK_HZ       160000000u
#define HSI_HZ          16000000u
#define APB1_HZ         160000000u
#define APB2_HZ         160000000u
#define ADC_CLK_HZ      16000000u

/* ------------------------------------------------------------------ */
/*  Pin map                                                            */
/*                                                                    */
/*  PB6  — I2C1 SCL   (MLX90632, SHT45, BMP390, RV-3028, LC709203F)   */
/*  PB7  — I2C1 SDA                                                   */
/*  PB10 — I2C2 SCL   (reserved for second MLX90632)                  */
/*  PB11 — I2C2 SDA                                                   */
/*  PA5  — SPI1 SCK   (W25Q80 flash)                                  */
/*  PA6  — SPI1 MISO                                                  */
/*  PA7  — SPI1 MOSI                                                  */
/*  PB0  — SPI1 CS0   (flash)                                         */
/*  PB1  — SPI1 CS1   (SX1262)                                        */
/*  PC10 — SPI3 SCK   (reserved)                                      */
/*  PA2  — USART2 TX  (CC2642R BLE module UART)                       */
/*  PA3  — USART2 RX                                                  */
/*  PC4  — USART3 TX  (USB-C debug shell)                             */
/*  PC5  — USART3 RX                                                  */
/*  PA9  — LPUART1 TX (low-power backhaul, optional)                  */
/*  PA10 — LPUART1 RX                                                 */
/*  PA0  — ADC1_IN1   (PT100 dry-bulb, via analog mux)                */
/*  PA1  — ADC1_IN2   (PT100 wet-bulb)                                */
/*  PA8  — ADC1_IN7   (acoustic emission, OPA2376 output)             */
/*  PB4  — TIM3_CH1 input capture (leaf-wetness NE555 frequency)      */
/*  PB5  — GPIO output (fan enable, active high)                      */
/*  PB8  — GPIO output (BLE module reset, active low)                 */
/*  PB9  — GPIO output (flash CS default high)                        */
/*  PC13 — GPIO input  (SX1262 DIO1, EXTI13)                          */
/*  PC14 — GPIO input  (SX1262 BUSY)                                  */
/*  PC15 — GPIO output (SX1262 NRST, active low)                      */
/*  PH3  — GPIO input  (BOOT0)                                        */
/* ------------------------------------------------------------------ */

/* I2C addresses (7-bit) */
#define I2C_ADDR_MLX90632    0x3B
#define I2C_ADDR_SHT45       0x44
#define I2C_ADDR_BMP390      0x77
#define I2C_ADDR_RV3028      0x32
#define I2C_ADDR_LC709203    0x0B

/* SX1262 SPI commands (subset) */
#define SX1262_CMD_SET_STANDBY   0x80
#define SX1262_CMD_SET_RX        0x82
#define SX1262_CMD_SET_TX        0x83
#define SX1262_CMD_WRITE_BUFFER  0x0E
#define SX1262_CMD_READ_BUFFER   0x1E
#define SX1262_CMD_SET_RF_FREQ   0x86
#define SX1262_CMD_SET_TX_PARAMS 0x8E
#define SX1262_CMD_SET_PACKET    0x8C
#define SX1262_CMD_SET_CAD       0xC0
#define SX1262_CMD_GET_STATUS    0xC0

/* W25Q80 commands */
#define W25Q80_CMD_READ          0x03
#define W25Q80_CMD_PAGE_PROGRAM  0x02
#define W25Q80_CMD_SECTOR_ERASE  0x20
#define W25Q80_CMD_READ_STATUS   0x05
#define W25Q80_CMD_WRITE_ENABLE  0x06
#define W25Q80_CMD_READ_ID       0x9F
#define W25Q80_SECTOR_SIZE       4096u
#define W25Q80_TOTAL_SIZE        0x100000u  /* 1 MB */
#define W25Q80_PAGES_PER_SECTOR  16u

/* ------------------------------------------------------------------ */
/*  Application constants                                              */
/* ------------------------------------------------------------------ */
#define SAMPLE_INTERVAL_SEC       300u    /* 5 minutes */
#define FAN_ON_DURATION_MS        8000u   /* psychrometer fan run */
#define AE_WINDOW_MS              40u     /* acoustic emission burst */
#define AE_SAMPLE_RATE_HZ         500000u /* 500 kSPS for AE ADC */
#define AE_FFT_SIZE               128u
#define MESH_MAX_NODES            32u
#define MESH_SUPERFRAME_MS        10000u  /* 10 s slot cycle */
#define MESH_SLOTS                32u
#define MESH_PAYLOAD_BYTES        19u
#define MESH_TAG_BYTES            4u
#define MESH_NETWORK_KEY_BYTES    16u
#define FLASH_RECORD_BYTES        24u
#define FLASH_RECORDS_MAX         ((W25Q80_TOTAL_SIZE / 2) / FLASH_RECORD_BYTES)

/* RFRI thresholds */
#define RFRI_GREEN                0.30f
#define RFRI_YELLOW               0.60f
#define RFRI_RED                  0.85f
#define TWET_CRITICAL_C           0.5f    /* wet-bulb frost trigger */
#define DELTA_RAD_FROST_K         20.0f   /* radiative deficit threshold */
#define LEAF_WET_DEW_THRESHOLD    28.0f   /* normalized 0-100, dew onset */

/* GPIO convenience */
#define FAN_ON()      SET_BITS(GPIOB->ODR, (1u << 5))
#define FAN_OFF()     CLR_BITS(GPIOB->ODR, (1u << 5))
#define BLE_RESET_N() CLR_BITS(GPIOB->ODR, (1u << 8))
#define BLE_RESET_DEASSERT() SET_BITS(GPIOB->ODR, (1u << 8))
#define FLASH_CS_LOW()  CLR_BITS(GPIOB->ODR, (1u << 0))
#define FLASH_CS_HIGH() SET_BITS(GPIOB->ODR, (1u << 0))
#define SX1262_CS_LOW() CLR_BITS(GPIOB->ODR, (1u << 1))
#define SX1262_CS_HIGH() SET_BITS(GPIOB->ODR, (1u << 1))

/* ------------------------------------------------------------------ */
/*  System tick (1 kHz from TIM6)                                      */
/* ------------------------------------------------------------------ */
extern volatile uint32_t g_tick_ms;
extern volatile uint32_t g_rtc_seconds;

static inline uint32_t tick_ms(void) { return g_tick_ms; }
static inline uint32_t time_ms(void) {
    uint32_t t;
    do { t = g_tick_ms; } while (t != g_tick_ms);
    return t;
}
static inline uint32_t elapsed_ms(uint32_t since) {
    return time_ms() - since;
}
static inline void delay_ms(uint32_t ms) {
    uint32_t start = time_ms();
    while (elapsed_ms(start) < ms) { /* spin */ }
}

/* ------------------------------------------------------------------ */
/*  Global system state                                                */
/* ------------------------------------------------------------------ */
typedef enum {
    SYS_STATE_SLEEP = 0,
    SYS_STATE_SAMPLE,
    SYS_STATE_COMPUTE,
    SYS_STATE_TRANSMIT,
    SYS_STATE_FROST_WATCH,
} sys_state_t;

typedef struct {
    sys_state_t  state;
    uint32_t     last_sample_ms;
    uint32_t     last_tx_ms;
    uint32_t     last_ae_ms;
    uint8_t      node_id;
    uint8_t      mesh_role;       /* 0=leaf, 1=relay, 2=root */
    uint8_t      mesh_hops;
    uint8_t      flags;
    uint8_t      sample_interval; /* minutes */
    uint8_t      battery_pct;
    uint16_t     battery_mv;
    int16_t      rfri_q8;         /* Q8.8 RFRI */
    int16_t      twet_cx100;      /* wet-bulb × 100 */
    int16_t      sky_t_cx100;
    int16_t      air_t_cx100;
    int16_t      delta_rad_cx100;
    uint16_t     leaf_wet;        /* 0-1000 normalized */
    uint16_t     ae_energy;       /* cumulative AE event energy */
    uint8_t      ae_status;       /* 0=idle, 1=armed, 2=nucleation */
    uint8_t      wick_dry;        /* 1 if psychrometer wick dry */
    uint32_t     records_written; /* flash journal counter */
} sys_state_block_t;

extern sys_state_block_t g_sys;

/* Flag bits */
#define SYS_FLAG_ALERT_ACTIVE   (1u << 0)
#define SYS_FLAG_AE_ARMED       (1u << 1)
#define SYS_FLAG_WICK_DRY       (1u << 2)
#define SYS_FLAG_LOW_BATTERY    (1u << 3)
#define SYS_FLAG_MESH_ROOT      (1u << 4)
#define SYS_FLAG_SOLAR_GOOD     (1u << 5)

#endif /* FROSTSENTINEL_BOARD_H */