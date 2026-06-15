/* ============================================
 * board.h — WaveForge Pin Definitions
 * ============================================ */

#ifndef WAVEFORGE_BOARD_H
#define WAVEFORGE_BOARD_H

#include "registers.h"

/* ---- Pin Definitions ---- */

/* MIDI */
#define MIDI_IN_PIN        3   /* PC3  — UART4_RX (MIDI IN via opto) */
#define MIDI_THRU_PIN      7   /* PE7  — UART4_TX (MIDI THRU) */
#define MIDI_UART           UART4

/* I2S1 Audio (to codec) */
#define I2S1_SDO_PIN       1   /* PA1  — I2S1 data out */
#define I2S1_SDI_PIN       2   /* PA2  — I2S1 data in */
#define I2S1_BCLK_PIN      3   /* PA3  — I2S1 bit clock */
#define I2S1_LRCK_PIN      4   /* PA4  — I2S1 LR clock */
#define I2S1_MCLK_PIN      10  /* PE10 — I2S1 master clock */

/* I2S2 Audio (from codec ADC, line-in) */
#define I2S2_SDI_PIN       5   /* PB5  — I2S2 data in */
#define I2S2_BCLK_PIN      6   /* PB6  — I2S2 bit clock */
#define I2S2_LRCK_PIN      7   /* PB7  — I2S2 LR clock */

/* SPI1 (flash) */
#define SPI1_SCK_PIN       5   /* PA5  — SPI1 clock */
#define SPI1_MISO_PIN      6   /* PA6  — SPI1 MISO */
#define SPI1_MOSI_PIN      7   /* PA7  — SPI1 MOSI */
#define SPI1_NCS_PIN       2   /* PB2  — SPI1 chip select */

/* I2C1 (codec, EEPROM, HP amp) */
#define I2C1_SDA_PIN       4   /* PC4  — I2C1 data */
#define I2C1_SCL_PIN       5   /* PC5  — I2C1 clock */

/* USB */
#define USB_DP_PIN         11  /* PE11 — USB D+ */
#define USB_DM_PIN         12  /* PE12 — USB D- */

/* ADC (CV inputs) */
#define CV1_PIN            0   /* PC0  — ADC1_IN10 */
#define CV2_PIN            1   /* PC1  — ADC1_IN11 */
#define CV3_PIN            2   /* PC2  — ADC1_IN12 */
#define CV4_PIN            13  /* PC13 — ADC1_IN13 */

/* Control GPIOs */
#define LED_POWER_PIN      0   /* PB0  — Power LED (active low) */
#define LED_MIDI_PIN       1   /* PB1  — MIDI LED (active low) */
#define CODEC_RESET_PIN    8   /* PE8  — WM8778 reset (active low) */
#define HPAMP_EN_PIN       9   /* PE9  — TPA6130 enable (active high) */
#define FLASH_WP_PIN       8   /* PB8  — W25Q256 write protect */
#define FLASH_HOLD_PIN     9   /* PB9  — W25Q256 hold */
#define USER_BUTTON_PIN    3   /* PB3  — User button (active low) */

/* Alternate Function assignments (STM32H7) */
#define AF_SPI1            5   /* AF5 for SPI1 on PA5/PA6/PA7 */
#define AF_I2C1            4   /* AF4 for I2C1 on PC4/PC5 */
#define AF_I2S1            6   /* AF6 for I2S1 on PA1/PA2/PA3/PA4 */
#define AF_I2S1_MCLK       6   /* AF6 for I2S1_MCLK on PE10 */
#define AF_UART4           8   /* AF8 for UART4 on PC3/PE7 */
#define AF_USB             10  /* AF10 for USB on PE11/PE12 */
#define AF_I2S2            5   /* AF5 for I2S2 on PB5/PB6/PB7 */
#define AF_ADC            0   /* AF0 for ADC (analog mode) */

/* ---- LED Macros ---- */
#define LED_POWER_ON()    (GPIOB->BSRR = (1U << (LED_POWER_PIN + 16)))  /* Active low */
#define LED_POWER_OFF()   (GPIOB->BSRR = (1U << LED_POWER_PIN))
#define LED_MIDI_ON()     (GPIOB->BSRR = (1U << (LED_MIDI_PIN + 16)))
#define LED_MIDI_OFF()    (GPIOB->BSRR = (1U << LED_MIDI_PIN))

/* ---- Codec Reset ---- */
#define CODEC_RESET_LOW()  (GPIOE->BSRR = (1U << (CODEC_RESET_PIN + 16)))
#define CODEC_RESET_HIGH()  (GPIOE->BSRR = (1U << CODEC_RESET_PIN))

/* ---- HP Amp Enable ---- */
#define HPAMP_ENABLE()     (GPIOE->BSRR = (1U << HPAMP_EN_PIN))
#define HPAMP_DISABLE()    (GPIOE->BSRR = (1U << (HPAMP_EN_PIN + 16)))

/* ---- Flash Control ---- */
#define FLASH_CS_LOW()     (GPIOB->BSRR = (1U << (SPI1_NCS_PIN + 16)))
#define FLASH_CS_HIGH()    (GPIOB->BSRR = (1U << SPI1_NCS_PIN))
#define FLASH_WP_ENABLE()  (GPIOB->BSRR = (1U << (FLASH_WP_PIN + 16)))
#define FLASH_WP_DISABLE() (GPIOB->BSRR = (1U << FLASH_WP_PIN))
#define FLASH_HOLD_LOW()   (GPIOB->BSRR = (1U << (FLASH_HOLD_PIN + 16)))
#define FLASH_HOLD_HIGH()  (GPIOB->BSRR = (1U << FLASH_HOLD_PIN))

/* ---- Audio Buffer Sizes ---- */
#define AUDIO_BUFFER_SIZE    256   /* Samples per DMA half-buffer */
#define AUDIO_NUM_BUFFERS    2     /* Double-buffered */
#define SAMPLE_RATE          48000 /* Hz */
#define BIT_DEPTH            24    /* bits */

/* ---- MIDI ---- */
#define MIDI_BAUD_RATE       31250

/* ---- I2C Addresses ---- */
#define WM8778_I2C_ADDR      0x1A
#define EEPROM_I2C_ADDR      0x50
#define TPA6130_I2C_ADDR     0x1C

#endif /* WAVEFORGE_BOARD_H */