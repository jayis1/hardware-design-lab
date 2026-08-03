/*
 * board.h — Inkwell smart fountain pen board configuration
 *
 * Defines the nRF52833 pinout, peripheral assignments, and board-level
 * constants for the Inkwell smart pen.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef INKWELL_BOARD_H
#define INKWELL_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Main clock ---- */
#define BOARD_HFCLK_HZ        (64000000UL)  /* Cortex-M4F core freq */
#define BOARD_LFCLK_HZ        (32768UL)     /* RTC source (RC32k) */

/* ---- LED (status indicator, rear of barrel) ---- */
#define LED_PIN               (28U)   /* P0.28 active-low status LED */
#define LED_ON()              (nrf_gpio_pin_clear(LED_PIN))
#define LED_OFF()             (nrf_gpio_pin_set(LED_PIN))
#define LED_TOGGLE()          (nrf_gpio_pin_toggle(LED_PIN))

/* ---- User button (hidden tactile, rear cap) ---- */
#define BUTTON_PIN            (29U)   /* P0.29, active-low, pull-up */
#define BUTTON_PRESSED()      ((nrf_gpio_pin_read(BUTTON_PIN) == 0))

/* ---- Cap detect (magnet in cap detected by BMM150, handled in firmware) ---- */

/* ---- BMI270 + BMM150 on SPI0 ---- */
#define SPI0_SCK_PIN          (3U)
#define SPI0_MOSI_PIN         (4U)
#define SPI0_MISO_PIN         (5U)
#define BMI270_CS_PIN         (6U)
#define BMM150_CS_PIN         (7U)
#define BMI270_INT1_PIN       (8U)   /* data-ready / FIFO watermark */
#define SPI0_FREQ_HZ          (8000000UL)

/* ---- HX711 pressure ADC ---- */
#define HX711_SCK_PIN         (10U)  /* clock output to HX711 */
#define HX711_DOUT_PIN        (11U)  /* data input, INT on falling edge */
#define HX711_RATE_500HZ      1      /* see pressure.c */

/* ---- PMW3360 optical flow on SPI1 ---- */
#define SPI1_SCK_PIN          (12U)
#define SPI1_MOSI_PIN         (13U)
#define SPI1_MISO_PIN         (14U)
#define PMW3360_CS_PIN        (15U)
#define PMW3360_MOTION_PIN    (16U)  /* active-low motion interrupt */
#define SPI1_FREQ_HZ          (2000000UL)

/* ---- W25Q64 SPI flash on SPI2 ---- */
#define SPI2_SCK_PIN          (20U)
#define SPI2_MOSI_PIN         (21U)
#define SPI2_MISO_PIN         (22U)
#define W25Q64_CS_PIN         (23U)
#define SPI2_FREQ_HZ          (16000000UL)
#define W25Q64_SECTOR_SIZE    (4096U)
#define W25Q64_NUM_SECTORS    (2048U)  /* 8 MB / 4 KB */
#define W25Q64_PAGE_SIZE      (256U)

/* ---- MAX17048 fuel gauge on TWI0 ---- */
#define TWI0_SCL_PIN          (26U)
#define TWI0_SDA_PIN          (27U)
#define MAX17048_ADDR         (0x36U)

/* ---- MCP73831 charger status (open-drain, read via GPIO) ---- */
#define CHG_STAT_PIN          (30U)   /* P0.30, low while charging */
#define IS_CHARGING()         (nrf_gpio_pin_read(CHG_STAT_PIN) == 0)

/* ---- USB-C for CDC + DFU ---- */
#define USB_DP_PIN            (34U)   /* P1.02 (USB+) */
#define USB_DM_PIN            (35U)   /* P1.03 (USB-) */

/* ---- Timing ---- */
#define SAMPLE_RATE_HZ        (1000U)
#define SAMPLE_PERIOD_US     (1000U)
#define BLE_SEGMENT_PERIOD_MS (20U)
#define PRESSURE_RATE_HZ      (500U)

/* ---- Flash journal ---- */
#define JOURNAL_MAGIC         (0x494E4B57U)  /* 'INKW' */
#define JOURNAL_VERSION       (1U)

/* ---- Pen-lift defaults (overridden by calibration) ---- */
#define DEFAULT_PEN_DOWN_MN   (150U)   /* 150 mN down threshold */
#define DEFAULT_PEN_UP_RATIO  (60U)    /* up at 60% of down */
#define PEN_LIFT_DEBOUNCE     (4U)    /* samples */

/* ---- AHRS ---- */
#define MADGWICK_BETA_DEFAULT (0.041f)
#define AHRS_SAMPLE_HZ        (1000U)

/* ---- BLE connection defaults ---- */
#define BLE_CONN_INTERVAL_MIN (12U)   /* 15 ms */
#define BLE_CONN_INTERVAL_MAX (24U)   /* 30 ms */
#define BLE_CONN_LATENCY      (0U)
#define BLE_CONN_TIMEOUT      (300U)  /* 3 s */

/* ---- Power states ---- */
typedef enum {
    PWR_STATE_OFF = 0,
    PWR_STATE_ADVERTISING,
    PWR_STATE_CONNECTED_IDLE,
    PWR_STATE_WRITING,
    PWR_STATE_CHARGING
} power_state_t;

/* ---- Forward decls for nRF GPIO helpers (provided by minimal HAL shim) ---- */
void nrf_gpio_pin_clear(uint32_t pin);
void nrf_gpio_pin_set(uint32_t pin);
void nrf_gpio_pin_toggle(uint32_t pin);
uint32_t nrf_gpio_pin_read(uint32_t pin);
void nrf_gpio_cfg_output(uint32_t pin, uint32_t pull);
void nrf_gpio_cfg_input(uint32_t pin, uint32_t pull);

#endif /* INKWELL_BOARD_H */