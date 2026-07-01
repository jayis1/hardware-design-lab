/*
 * board.h — Soilchord board-level definitions
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Pin map for the Soilchord reference board (STM32L432KCU6).
 * All constants and pin assignments used across the firmware live here.
 */
#ifndef SOILCHORD_BOARD_H
#define SOILCHORD_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ----------------------------- MCU & clocks ----------------------------- */
#define BOARD_MCU_NAME          "STM32L432KCU6"
#define BOARD_HSI_HZ            16000000UL
#define BOARD_HSE_HZ            0UL                 /* no external xtal on L432 */
#define BOARD_SYSCLK_HZ          80000000UL
#define BOARD_HCLK_HZ            80000000UL
#define BOARD_PCLK1_HZ           40000000UL
#define BOARD_PCLK1_TMR_HZ       80000000UL          /* x2 for timers */
#define BOARD_PCLK2_HZ           80000000UL
#define BOARD_LSE_HZ             32768UL

/* ----------------------------- Chord geometry ---------------------------- */
#define NCHORDS                 6
#define SAMPLE_RATE_HZ          16000UL
#define SAMPLE_DURATION_MS      512UL
#define NSAMPLES_PER_CHORD      ((SAMPLE_RATE_HZ / 1000UL) * SAMPLE_DURATION_MS)
#define FFT_SIZE                4096                /* real-input FFT */

/* Default free-air fundamental frequencies (Hz) — staggered A3..A5 */
static const float kChordFreeAirHz[NCHORDS] = {
    220.00f, 294.00f, 392.00f, 523.25f, 698.46f, 880.00f
};
/* Nominal depths (metres) */
static const float kChordDepthM[NCHORDS] = {
    0.00f, 0.30f, 0.60f, 0.90f, 1.20f, 1.50f
};
/* Peak-search half-band as a fraction of the nominal f0 (±0.15) */
#define PEAK_BAND_FRAC          0.15f

/* ----------------------------- Pin map ----------------------------------- */
/* Port letters map to GPIOA..GPIOC; macros give a (port, pin) tuple the
 * HAL layer interprets. We keep them abstract here so the firmware is
 * portable to a different STM32 variant with minimal churn. */

/* Piezo / signal chain ADC (ADC1) */
#define PIN_PIEZO_ADC           { 'A', 0 }          /* ADC1_IN1 */
#define PIN_PGA_CS              { 'B', 4 }           /* SPI1 NSS for MCP6S22 (software) */
#define PIN_PGA_SCK             { 'B', 3 }           /* SPI1 SCK  */
#define PIN_PGA_MOSI            { 'B', 5 }           /* SPI1 MOSI */
#define PIN_PGA_MISO            { 'B', 6 }           /* SPI1 MISO (unused, held high) */
#define PIN_ANALOG_LDO_EN       { 'B', 1 }           /* enable for the +3.0 V analog rail */

/* Solenoid drivers (4 solenoids multiplexed across 6 chords) */
#define PIN_SOL_DRV0            { 'A', 8 }           /* TIM1 CH1 → MOSFET gate */
#define PIN_SOL_DRV1            { 'A', 9 }           /* TIM1 CH2 */
#define PIN_SOL_DRV2            { 'A', 10 }          /* TIM1 CH3 */
#define PIN_SOL_DRV3            { 'A', 11 }          /* TIM1 CH4 */
#define PLUCK_MS                30                   /* solenoid on-time per pluck */
#define PLUCK_SETTLE_MS         20                   /* transient settle after pluck */

/*
 * Solenoid→chord mapping. 4 solenoids drive 6 chords via a small mechanical
 * multiplexer (two solenoids each serve a pair of chords through a lever that
 * the previous pluck primes). The table gives, for each chord, the solenoid
 * index to fire.
 */
static const uint8_t kSolenoidForChord[NCHORDS] = { 0, 1, 2, 3, 0, 1 };
/* Reverse-lever priming: before plucking chord 5 (idx 4) or chord 6 (idx 5)
 * we fire the partner solenoid briefly in the opposite direction so the lever
 * is positioned. In practice this is a no-op on the simple reference board; the
 * actuator driver honours this table for future levered revisions. */

/* 1-Wire bus for DS18B20 temperature sensors (one per chord) */
#define PIN_ONEWIRE             { 'B', 7 }           /* open-drain GPIO */
#define DS18B18_CONV_MS         750                  /* 12-bit max-conversion */

/* LoRa radio (SX1262) on SPI2 */
#define PIN_LORA_NSS            { 'B', 12 }
#define PIN_LORA_SCK            { 'B', 13 }
#define PIN_LORA_MOSI           { 'B', 15 }
#define PIN_LORA_MISO           { 'B', 14 }
#define PIN_LORA_BUSY           { 'A', 15 }
#define PIN_LORA_DIO1          { 'B', 2 }           /* EXTI2 */
#define PIN_LORA_NRST          { 'B', 8 }
#define PIN_LORA_ANT_SW        { 'A', 4 }           /* RF switch ctrl */

/* NOR flash (W25Q64) on a bit-banged SPI to keep SPI1/SPI2 free */
#define PIN_FLASH_NSS           { 'A', 12 }
#define PIN_FLASH_SCK           { 'A', 13 }
#define PIN_FLASH_MOSI          { 'A', 14 }
#define PIN_FLASH_MISO          { 'A', 5 }

/* Fuel gauge (MAX17048) on I2C1 */
#define PIN_I2C1_SCL            { 'B', 10 }
#define PIN_I2C1_SDA            { 'B', 11 }
#define MAX17048_I2C_ADDR       0x36

/* RTC alarm line (wakes from STOP2) */
#define PIN_RTC_ALARM           { 'C', 14 }          /* internally tied to EXTI14 */

/* Status LED (single, tri-colour common-anode) */
#define PIN_LED_R               { 'B', 0 }
#define PIN_LED_G               { 'B', 9 }
#define PIN_LED_B               { 'C', 13 }

/* ----------------------------- Power ------------------------------------- */
#define VBATT_MIN_MV            2800                 /* hard floor, measurements stop */
#define VBATT_TX_MIN_MV         3000                 /* below this, no LoRa TX */
#define BATTERY_NOM_MV          3200
#define SOLAR_OK_MV             4000

/* ----------------------------- Timing ------------------------------------ */
#define DEFAULT_INTERVAL_S      600                  /* 6 measurements/hour */
#define URGENT_INTERVAL_S       120                  /* adaptive fast mode */
#define URGENT_BURST_CYCLES     24
#define LORA_RETRIES            3
#define LORA_TX_JITTER_MS_MAX   512

/* ----------------------------- Anomaly detector -------------------------- */
#define ANOMALY_ALPHA           0.05f                /* EMA smoothing */
#define ANOMALY_Z_THRESH        3.0f
#define ANOMALY_PERSIST         3                    /* consecutive cycles to flag */

/* ----------------------------- Flash log -------------------------------- */
#define FLASH_LOG_MAX_RECORDS   2400                 /* ~10 days at default rate */

#endif /* SOILCHORD_BOARD_H */