/*
 * board.h — Hardware pin map and peripheral assignments for Lichenwatch.
 *
 * Target: STM32L476RGT6 (Cortex-M4F, LQFP64).
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef LICHENWATCH_BOARD_H
#define LICHENWATCH_BOARD_H

#include <stdint.h>

/* ------------------------------------------------------------------------ */
/* Crystal / clock                                                          */
/* ------------------------------------------------------------------------ */
#define BOARD_HSE_HZ        16000000UL   /* external TCXO on PD0/PD1        */
#define BOARD_LSE_HZ        32768UL      /* 32.768 kHz watch crystal         */
#define BOARD_SYSCLK_HZ     80000000UL   /* PLL from HSE                     */
#define BOARD_APB1_HZ       40000000UL
#define BOARD_APB2_HZ       80000000UL

/* ------------------------------------------------------------------------ */
/* MCU identity                                                              */
/* ------------------------------------------------------------------------ */
#define BOARD_MCU_NAME      "STM32L476RGT6"
#define BOARD_NODE_ID_DEFAULT "LW-0001"

/* ------------------------------------------------------------------------ */
/* Port / pin helpers                                                        */
/* ------------------------------------------------------------------------ */
#define PORTA   0
#define PORTB   1
#define PORTC   2
#define PORTD   3
#define PORTH   7

#define PIN(port, num)  (((port) << 4) | (num))

/* ------------------------------------------------------------------------ */
/* PAM fluorometer optics                                                    */
/* ------------------------------------------------------------------------ */
#define PAM_LED_ACTINIC_PIN     PIN(PORTC, 6)   /* TIM3 CH1 -> 470 nm LED A   */
#define PAM_LED_ACTINIC2_PIN    PIN(PORTC, 7)   /* TIM3 CH2 -> 470 nm LED B   */
#define PAM_LED_MEASURE_PIN     PIN(PORTC, 8)   /* TIM3 CH3 -> 620 nm LED     */
#define PAM_PD_ADC_PIN          PIN(PORTA, 0)   /* ADC1 IN1 -> Si photodiode  */
#define PAM_SHROUD_PIN          PIN(PORTC, 9)   /* GPIO -> shutter servo/LED */

#define PAM_ACTINIC_CURRENT_UA  1500            /* µmol photons m⁻² s⁻¹ equiv  */
#define PAM_MEASURE_FREQ_HZ     1000
#define PAM_SAT_PULSE_MS        800
#define PAM_DARK_ADAPT_MS       30000

/* ------------------------------------------------------------------------ */
/* I²C sensors (shared I2C1)                                                  */
/* ------------------------------------------------------------------------ */
#define I2C1_SCL_PIN            PIN(PORTB, 8)
#define I2C1_SDA_PIN            PIN(PORTB, 9)
#define I2C1_CLOCK_HZ           100000

#define AS7341_I2C_ADDR         0x39   /* spectral sensor */
#define SHT45_I2C_ADDR          0x44   /* T/RH */
#define SI1145_I2C_ADDR         0x60   /* UV/IR/vis */
#define VEML6075_I2C_ADDR       0x10   /* UV-A/B */

/* ------------------------------------------------------------------------ */
/* CO₂ NDIR (Senseair S8, UART)                                               */
/* ------------------------------------------------------------------------ */
#define S8_UART_PIN_TX          PIN(PORTC, 10)  /* LPUART1 / USART3 */
#define S8_UART_PIN_RX          PIN(PORTC, 11)
#define S8_UART_BAUD            9600

/* ------------------------------------------------------------------------ */
/* Surface wetness (ADC)                                                     */
/* ------------------------------------------------------------------------ */
#define WETNESS_ADC_PIN         PIN(PORTA, 1)   /* ADC1 IN2 */
#define WETNESS_EXCITE_PIN      PIN(PORTB, 1)   /* GPIO excitation */

/* ------------------------------------------------------------------------ */
/* I²S MEMS contact microphone                                               */
/* ------------------------------------------------------------------------ */
#define MIC_I2S_WS_PIN          PIN(PORTB, 12)
#define MIC_I2S_CK_PIN          PIN(PORTB, 13)
#define MIC_I2S_SD_PIN          PIN(PORTB, 14)
#define MIC_I2S_FS_HZ          16000
#define MIC_SAMPLE_COUNT       2048            /* 2 s / 16 kHz = 32768 -> 2 s */
#define MIC_CAPTURE_MS         128             /* 2048/16 kHz */

/* ------------------------------------------------------------------------ */
/* SPI flash (Winbond W25Q80, SPI1)                                          */
/* ------------------------------------------------------------------------ */
#define FLASH_SPI_NSS_PIN       PIN(PORTA, 4)
#define FLASH_SPI_SCK_PIN       PIN(PORTA, 5)
#define FLASH_SPI_MISO_PIN      PIN(PORTA, 6)
#define FLASH_SPI_MOSI_PIN      PIN(PORTA, 7)
#define FLASH_SPI_CLOCK_HZ      16000000
#define FLASH_SIZE_BYTES        (8UL * 1024UL * 1024UL)

/* ------------------------------------------------------------------------ */
/* LoRa (SX1262, SPI2)                                                       */
/* ------------------------------------------------------------------------ */
#define LORA_SPI_NSS_PIN        PIN(PORTB, 2)
#define LORA_SPI_SCK_PIN        PIN(PORTB, 10)
#define LORA_SPI_MISO_PIN       PIN(PORTC, 2)
#define LORA_SPI_MOSI_PIN       PIN(PORTC, 3)
#define LORA_BUSY_PIN           PIN(PORTC, 4)
#define LORA_DIO1_PIN           PIN(PORTC, 5)
#define LORA_NRST_PIN           PIN(PORTC, 1)
#define LORA_SPI_CLOCK_HZ       8000000

#define LORA_FREQ_HZ            868100000UL   /* EU868 default */
#define LORA_TX_POWER_DBM       22
#define LORA_SF                 10
#define LORA_BW_HZ              125000
#define LORA_CR                 5             /* 4/5 */

/* ------------------------------------------------------------------------ */
/* nRF52840 BLE co-MCU (UART link to STM32)                                  */
/* ------------------------------------------------------------------------ */
#define BLE_UART_PIN_TX         PIN(PORTA, 9)  /* USART1 */
#define BLE_UART_PIN_RX         PIN(PORTA, 10)
#define BLE_UART_BAUD           115200
#define BLE_NRESET_PIN          PIN(PORTA, 15)
#define BLE_IRQ_PIN             PIN(PORTB, 4)

/* ------------------------------------------------------------------------ */
/* Power management (bq25570 + battery gauge)                                */
/* ------------------------------------------------------------------------ */
#define POWER_VBAT_ADC_PIN      PIN(PORTA, 3)   /* ADC1 IN8 (div /2)        */
#define POWER_SOLAR_ADC_PIN     PIN(PORTA, 4)   /* (shared NSS — muxed)      */
#define POWER_VBAT_DIVIDER       2.0f
#define POWER_BAT_MIN_MV        3300
#define POWER_BAT_MAX_MV        3600
#define POWER_BAT_CRIT_MV       3200

/* ------------------------------------------------------------------------ */
/* TPL5111 nano-power timer (drives MCU enable)                              */
/* ------------------------------------------------------------------------ */
#define TPL5111_DONE_PIN        PIN(PORTB, 0)
#define TPL5111_DRIVE_PIN       PIN(PORTB, 5)   /* input from TPL5111        */

/* ------------------------------------------------------------------------ */
/* Status LED                                                                */
/* ------------------------------------------------------------------------ */
#define STATUS_LED_PIN          PIN(PORTC, 13)

/* ------------------------------------------------------------------------ */
/* Timing configuration (minutes)                                            */
/* ------------------------------------------------------------------------ */
#define DEFAULT_MEASURE_INTERVAL_MIN   15
#define DEFAULT_UPLINK_INTERVAL_MIN    60
#define MIN_MEASURE_INTERVAL_MIN       5
#define MAX_MEASURE_INTERVAL_MIN       240

/* ------------------------------------------------------------------------ */
/* Storage ring geometry                                                     */
/* ------------------------------------------------------------------------ */
#define STORAGE_RECORD_BYTES     64
#define STORAGE_MAX_RECORDS      (FLASH_SIZE_BYTES / STORAGE_RECORD_BYTES)
#define STORAGE_MAGIC            0x4C574E31UL  /* 'LWN1' */

/* ------------------------------------------------------------------------ */
/* System tick                                                               */
/* ------------------------------------------------------------------------ */
#define SYSTICK_HZ               1000

/* ------------------------------------------------------------------------ */
/* Author / version metadata                                                 */
/* ------------------------------------------------------------------------ */
#define FW_AUTHOR                "jayis1"
#define FW_VERSION               "1.0.0"
#define FW_BUILD_DATE             "2026-07-08"

#endif /* LICHENWATCH_BOARD_H */