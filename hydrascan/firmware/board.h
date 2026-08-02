/*
 * board.h — HydraScan hardware pin map and constants
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * STM32H733VG peripheral assignment for the HydraScan liquid
 * fingerprinting instrument. Register-level definitions live in
 * registers.h; this header holds the logical board map and
 * compile-time hardware constants used by the drivers.
 */
#ifndef HYDRASCAN_BOARD_H
#define HYDRASCAN_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Compile-time hardware constants --------------------------------- */

#define MCU_HCLK_HZ        240000000u   /* HCLK after PLL from 16 MHz HSE   */
#define MCU_PCLK1_HZ       120000000u   /* APB1 (SPI2/3, I2C1, UART4)        */
#define MCU_PCLK2_HZ       120000000u   /* APB2 (SPI1, ADC1, USB)           */
#define MCU_SYSTICK_HZ     1000u        /* 1 ms tick                        */

#define QSPI_FLASH_SIZE    (8u * 1024u * 1024u)   /* W25Q64 64 Mbit          */
#define MAX_LIQUID_CLASSES 64u
#define FEATURE_DIM_RAW    49u          /* 8 optical + 40 EIS + 1 temp     */
#define FEATURE_DIM_PCA    16u          /* whitened projection dimension   */
#define EIS_FREQ_POINTS    20u          /* log-spaced 1 Hz..100 kHz        */
#define OPTICAL_WAVELENGHS 8u
#define EIS_AVG_SWEEPS     4u           /* averages per frequency point    */

/* ---- GPIO pin map (logical names → port/pin) ------------------------ */

typedef struct {
    volatile uint32_t *port;   /* &GPIOA..&GPIOI  */
    uint8_t            pin;
} hgpio_t;

#define PIN_LED_MUX_SCK   { &GPIOA, 5 }    /* SPI1 SCK → 74HC595 clock      */
#define PIN_LED_MUX_MOSI  { &GPIOA, 7 }    /* SPI1 MOSI → 74HC595 data       */
#define PIN_LED_MUX_LATCH { &GPIOA, 4 }    /* latch pulse to 74HC595        */
#define PIN_LED_OE_N      { &GPIOB, 3 }    /* active-low output enable      */

#define PIN_AD5940_CS     { &GPIOB, 12 }   /* SPI2 NSS (manual)             */
#define PIN_AD5940_SCK    { &GPIOB, 13 }
#define PIN_AD5940_MISO   { &GPIOB, 14 }
#define PIN_AD5940_MOSI   { &GPIOB, 15 }
#define PIN_AD5940_IRQ    { &GPIOE, 3 }
#define PIN_AD5940_RST    { &GPIOE, 4 }
#define PIN_AD5940_GPIO1  { &GPIOE, 5 }     /* general-purpose flag from 5940*/

#define PIN_OLED_CS       { &GPIOB, 5 }
#define PIN_OLED_DC       { &GPIOB, 4 }
#define PIN_OLED_RST      { &GPIOB, 6 }
#define PIN_OLED_SCK      { &GPIOC, 10 }    /* SPI3                          */
#define PIN_OLED_MOSI     { &GPIOC, 12 }

#define PIN_I2C1_SCL      { &GPIOB, 8 }     /* TMP117 + BQ25895              */
#define PIN_I2C1_SDA      { &GPIOB, 9 }

#define PIN_BLE_TX        { &GPIOA, 0 }     /* UART4                         */
#define PIN_BLE_RX        { &GPIOA, 1 }
#define PIN_BLE_CTS       { &GPIOA, 2 }
#define PIN_BLE_RTS       { &GPIOA, 3 }
#define PIN_BLE_EN        { &GPIOB, 0 }

#define PIN_USB_DM        { &GPIOA, 11 }
#define PIN_USB_DP        { &GPIOA, 12 }
#define PIN_USB_VBUS      { &GPIOA, 9 }

#define PIN_QSPI_CS       { &GPIOB, 10 }
#define PIN_QSPI_SCK      { &GPIOB, 2 }
#define PIN_QSPI_IO0      { &GPIOD, 11 }
#define PIN_QSPI_IO1      { &GPIOD, 12 }
#define PIN_QSPI_IO2      { &GPIOD, 13 }
#define PIN_QSPI_IO3      { &GPIOD, 14 }

#define PIN_BUTTON        { &GPIOC, 0 }     /* capacitive touch (TSC G1)    */
#define PIN_CHG_INT       { &GPIOC, 13 }    /* BQ25895 interrupt            */
#define PIN_CHG_CE         { &GPIOB, 1 }

/* ADC channels (ADC1, single-ended) */
#define ADC_CH_SAMPLE_PD  3u   /* PA3  — sample photodiode OPT101         */
#define ADC_CH_REF_PD     8u   /* PB0? — reference photodiode OPT101       */
/* NOTE: ref photodiode shares PB0 with BLE_EN in early revs; rev B moves
 * it to PC4/ADC1 ch14. The driver reads whichever the build flag below
 * selects. */
#define ADC_CH_REF_PD_REV_B 14u

/* ---- I2C addresses -------------------------------------------------- */
#define TMP117_I2C_ADDR   0x48u
#define BQ25895_I2C_ADDR  0x6Bu

/* ---- Result / status enums ----------------------------------------- */
typedef enum {
    HYDRA_OK = 0,
    HYDRA_ERR_IO,         /* SPI/I2C/ADC communication failure             */
    HYDRA_ERR_TIMEOUT,
    HYDRA_ERR_NOMEM,
    HYDRA_ERR_NOCLASS,    /* nothing matched above threshold              */
    HYDRA_ERR_CALIB,      /* library not calibrated / empty              */
    HYDRA_ERR_BUSY
} hydra_err_t;

/* ---- Time helpers (1 ms tick) -------------------------------------- */
void     board_delay_ms(uint32_t ms);
uint32_t board_millis(void);

#endif /* HYDRASCAN_BOARD_H */