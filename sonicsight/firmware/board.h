/**
 * @file    board.h
 * @brief   SonicSight — Board-level pin mappings, peripheral configuration, and
 *          hardware constants.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * This header centralises all hardware-dependent definitions for the SonicSight
 * acoustic tomography scanner PCB (rev 1.0). Every GPIO, peripheral instance,
 * timer channel, DMA stream, and memory-mapped address used by the firmware is
 * declared here. Changing a pin assignment or adding a new peripheral requires
 * editing only this file and the KiCad netlist.
 */

#ifndef BOARD_H
#define BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*  Oscillators & Clock                                                       */
/* ========================================================================== */
#define HSE_VALUE              (25000000U)    /* 25 MHz main crystal */
#define LSE_VALUE              (32768U)       /* 32.768 kHz RTC crystal */
#define HSI_VALUE              (64000000U)    /* Internal 64 MHz RC (fallback) */
#define SYSCLK_FREQ            (480000000U)   /* Final CPU clock (480 MHz)  */
#define APB1_CLOCK             (120000000U)   /* APB1 timer clock (÷4) */
#define APB2_CLOCK             (240000000U)   /* APB2 timer clock (÷2) */

/* ========================================================================== */
/*  Memory Regions (STM32H743ZIT6)                                            */
/* ========================================================================== */
#define FLASH_BASE             (0x08000000U)
#define FLASH_SIZE             (0x00200000U)  /* 2 MB */
#define ITCM_RAM_BASE          (0x00000000U)
#define ITCM_RAM_SIZE          (0x00010000U)  /* 64 KB */
#define DTCM_RAM_BASE          (0x20000000U)
#define DTCM_RAM_SIZE          (0x00020000U)  /* 128 KB */
#define AXI_SRAM_BASE          (0x24000000U)
#define AXI_SRAM_SIZE          (0x00080000U)  /* 512 KB */
#define SRAM1_BASE             (0x30000000U)
#define SRAM1_SIZE             (0x00020000U)  /* 128 KB */
#define SRAM2_BASE             (0x30020000U)
#define SRAM2_SIZE             (0x00020000U)  /* 128 KB */
#define SRAM3_BASE             (0x30040000U)
#define SRAM3_SIZE             (0x00010000U)  /* 64 KB */
#define SDRAM_BASE             (0xD0000000U)  /* FMC — external SDRAM */
#define SDRAM_SIZE             (0x00800000U)  /* 8 MB */
#define QSPI_BASE              (0x90000000U)  /* Quad-SPI flash */
#define QSPI_SIZE              (0x01000000U)  /* 16 MB */

/* ========================================================================== */
/*  ADC – ADS5281 × 2 (8 channels, 50 MSPS, 12-bit LVDS)                     */
/*  Connected via DCMI (Digital Camera Interface) in serial-parallel mode.    */
/* ========================================================================== */
#define ADC_SPI_INST           SPI5          /* SPI5 for ADC register R/W */
#define ADC_CS_PIN             GPIO_PIN_12   /* PF12 — ADC chip select */
#define ADC_CS_PORT            GPIOF
#define ADC_CLK_PIN            GPIO_PIN_6    /* PI6 — ADC bit clock (DCMI PIXCK) */
#define ADC_CLK_PORT           GPIOI
#define ADC_DATA_PIN           GPIO_PIN_7    /* PI7 — ADC serial data (DCMI D0) */
#define ADC_DATA_PORT          GPIOI
#define ADC_SYNC_PIN           GPIO_PIN_11   /* PF11 — ADC frame sync */
#define ADC_SYNC_PORT          GPIOF
#define ADC_NUM_CHANNELS       (8U)          /* 8 physical ADC channels */
#define ADC_SAMPLES_PER_TRACE  (1024U)       /* Samples captured per channel per TX */
#define ADC_SAMPLE_RATE        (50000000U)   /* 50 MSPS */
#define ADC_RESOLUTION_BITS    (12U)

/* ========================================================================== */
/*  TX Pulse — High-Voltage Pulser (MD1210 + TC6320)                         */
/*  TIM1 generates burst waveform. TIM8 provides trigger timing.              */
/* ========================================================================== */
#define TX_TIM_INST            TIM1          /* Advanced timer — burst generation */
#define TX_TIM_CH1             TIM_CHANNEL_1 /* PE9 — PWM burst output */
#define TX_TIM_CH1_PIN         GPIO_PIN_9
#define TX_TIM_CH1_PORT        GPIOE
#define TX_TIM_CH2             TIM_CHANNEL_2 /* PE11 — complementary (half-bridge) */
#define TX_TIM_CH2_PIN         GPIO_PIN_11
#define TX_TIM_CH2_PORT        GPIOE
#define TX_TRIG_TIM_INST       TIM8          /* Timer — acquisition trigger timing */
#define TX_TRIG_CH1            TIM_CHANNEL_1 /* PC6 — trigger to ADC frame-sync */
#define TX_TRIG_CH1_PIN        GPIO_PIN_6
#define TX_TRIG_CH1_PORT       GPIOC

/* Number of cycles per burst (3–20 programmable at runtime) */
#define TX_BURST_CYCLES_DEFAULT (5U)
/* Transmit frequency (Hz) — default 50 kHz, tunable 20–150 kHz */
#define TX_FREQ_HZ_DEFAULT     (50000U)
/* Transmit voltage divider tap (PWM duty cycle controls HV amplitude) */
#define TX_AMPLITUDE_DEFAULT   (100U)       /* percent */

/* ========================================================================== */
/*  Transmit Multiplexer — 4× ADG732 (each 32:1, routed to HV pulser output)  */
/*  Aggregated for 32 transducers. Controlled via GPIO expander or direct port.*/
/* ========================================================================== */
#define TX_MUX_SEL_PORT        GPIOA
#define TX_MUX_SEL0_PIN        GPIO_PIN_0    /* PA0 — mux address LSB */
#define TX_MUX_SEL1_PIN        GPIO_PIN_1    /* PA1 */
#define TX_MUX_SEL2_PIN        GPIO_PIN_2    /* PA2 */
#define TX_MUX_SEL3_PIN        GPIO_PIN_3    /* PA3 */
#define TX_MUX_SEL4_PIN        GPIO_PIN_4    /* PA4 */
#define TX_MUX_EN_PIN          GPIO_PIN_5    /* PA5 — mux enable (active low) */
#define TX_MUX_EN_PORT         GPIOA
#define TX_NUM_CHANNELS        (32U)         /* Total transducer channels */

/* ========================================================================== */
/*  Receive Multiplexer — 4× ADG732 (32:1) feeding 4 dual-ADC channels       */
/*  Each mux routes one of 32 transducers to one of 8 LNA inputs.             */
/* ========================================================================== */
#define RX_MUX0_SEL_PORT       GPIOB
#define RX_MUX0_SEL0_PIN       GPIO_PIN_0
#define RX_MUX0_SEL1_PIN       GPIO_PIN_1
#define RX_MUX0_SEL2_PIN       GPIO_PIN_2
#define RX_MUX0_SEL3_PIN       GPIO_PIN_3
#define RX_MUX0_SEL4_PIN       GPIO_PIN_4
#define RX_MUX0_EN_PIN         GPIO_PIN_5
#define RX_MUX0_EN_PORT        GPIOB

/* Remaining muxes (RX_MUX1..3) share the same address bits on successive port bits */
#define RX_MUX1_SEL_PORT       GPIOB
#define RX_MUX1_SEL_PINS       (0xFFU << 8)  /* PB8–PB12 */
#define RX_MUX_BANK_COUNT      (4U)

/* ========================================================================== */
/*  Variable-Gain Amplifier — AD8331 × 8 channels                            */
/*  Gain controlled via SPI DAC or direct voltage on GAIN pin.               */
/* ========================================================================== */
#define VGA_SPI_INST           SPI3
#define VGA_GAIN_MIN_DB        (0)
#define VGA_GAIN_MAX_DB        (40)
#define VGA_GAIN_DEFAULT_DB    (20)
/* DAC for VGA gain voltage reference */
#define VGA_DAC_INST           SPI4
#define VGA_DAC_CS_PIN         GPIO_PIN_8    /* PB8 */
#define VGA_DAC_CS_PORT        GPIOB

/* ========================================================================== */
/*  Programmable Filter — LTC1563-2 (4th-order, fc = 20–200 kHz)             */
/*  Set via serial interface (SPI) to configure internal resistor ratios.     */
/* ========================================================================== */
#define FILTER_SPI_INST        SPI6
#define FILTER_CS_PIN          GPIO_PIN_9    /* PG9 */
#define FILTER_CS_PORT         GPIOG
#define FILTER_FC_MIN_HZ       (20000U)
#define FILTER_FC_MAX_HZ       (200000U)
#define FILTER_FC_DEFAULT_HZ   (50000U)

/* ========================================================================== */
/*  BLE Module — NRF52840 (standalone, UART HCI + SPI data channel)           */
/* ========================================================================== */
#define BLE_UART_INST          UART4
#define BLE_UART_TX_PIN        GPIO_PIN_10   /* PD10 — TX to NRF52 RX */
#define BLE_UART_TX_PORT       GPIOD
#define BLE_UART_RX_PIN        GPIO_PIN_9    /* PD9 — RX from NRF52 TX */
#define BLE_UART_RX_PORT       GPIOD
#define BLE_UART_BAUD          (115200U)
#define BLE_RTS_PIN            GPIO_PIN_11   /* PD11 — RTS */
#define BLE_RTS_PORT           GPIOD
#define BLE_CTS_PIN            GPIO_PIN_12   /* PD12 — CTS */
#define BLE_CTS_PORT           GPIOD
#define BLE_RESET_PIN          GPIO_PIN_13   /* PD13 — NRF52 reset */
#define BLE_RESET_PORT         GPIOD
#define BLE_SPI_INST           SPI2          /* SPI2 — high-speed data channel */
#define BLE_SPI_SCK_PIN        GPIO_PIN_13   /* PB13 */
#define BLE_SPI_SCK_PORT       GPIOB
#define BLE_SPI_MISO_PIN       GPIO_PIN_14   /* PB14 */
#define BLE_SPI_MISO_PORT      GPIOB
#define BLE_SPI_MOSI_PIN       GPIO_PIN_15   /* PB15 */
#define BLE_SPI_MOSI_PORT      GPIOB
#define BLE_SPI_CS_PIN         GPIO_PIN_1    /* PD1 */
#define BLE_SPI_CS_PORT        GPIOD

/* ========================================================================== */
/*  LCD — GC9A01 240×240 IPS SPI                                              */
/* ========================================================================== */
#define LCD_SPI_INST           SPI1
#define LCD_SPI_SCK_PIN        GPIO_PIN_5    /* PA5 */
#define LCD_SPI_SCK_PORT       GPIOA
#define LCD_SPI_MOSI_PIN       GPIO_PIN_7    /* PA7 */
#define LCD_SPI_MOSI_PORT      GPIOA
#define LCD_SPI_MISO_PIN       GPIO_PIN_6    /* PA6 (not used by GC9A01) */
#define LCD_SPI_MISO_PORT      GPIOA
#define LCD_CS_PIN             GPIO_PIN_4    /* PA4 */
#define LCD_CS_PORT            GPIOA
#define LCD_DC_PIN             GPIO_PIN_3    /* PC3 — data/command */
#define LCD_DC_PORT            GPIOC
#define LCD_RST_PIN            GPIO_PIN_2    /* PC2 — reset */
#define LCD_RST_PORT           GPIOC
#define LCD_BL_PIN             GPIO_PIN_1    /* PC1 — backlight PWM */
#define LCD_BL_PORT            GPIOC
#define LCD_WIDTH              (240U)
#define LCD_HEIGHT             (240U)
#define LCD_BL_TIM             TIM3          /* Timer for backlight PWM */
#define LCD_BL_TIM_CH          TIM_CHANNEL_3

/* ========================================================================== */
/*  microSD Card (SPI mode)                                                   */
/* ========================================================================== */
#define SD_SPI_INST            SPI6
#define SD_SPI_SCK_PIN         GPIO_PIN_5    /* PG5 */
#define SD_SPI_SCK_PORT        GPIOG
#define SD_SPI_MOSI_PIN        GPIO_PIN_7    /* PG7 */
#define SD_SPI_MOSI_PORT       GPIOG
#define SD_SPI_MISO_PIN        GPIO_PIN_6    /* PG6 */
#define SD_SPI_MISO_PORT       GPIOG
#define SD_CS_PIN              GPIO_PIN_8    /* PG8 */
#define SD_CS_PORT             GPIOG
#define SD_DETECT_PIN          GPIO_PIN_15   /* PD15 — card detect switch */
#define SD_DETECT_PORT         GPIOD

/* ========================================================================== */
/*  IMU — ICM-20948 (9-axis, SPI)                                            */
/* ========================================================================== */
#define IMU_SPI_INST           SPI2
#define IMU_CS_PIN             GPIO_PIN_0    /* PD0 */
#define IMU_CS_PORT            GPIOD
#define IMU_INT_PIN            GPIO_PIN_1    /* PE1 — data-ready interrupt */
#define IMU_INT_PORT           GPIOE

/* ========================================================================== */
/*  Temperature Sensor — TMP117 (±0.1 °C, I2C)                               */
/* ========================================================================== */
#define TMP117_I2C_INST        I2C1
#define TMP117_I2C_SCL_PIN     GPIO_PIN_8    /* PB8 */
#define TMP117_I2C_SCL_PORT    GPIOB
#define TMP117_I2C_SDA_PIN     GPIO_PIN_9    /* PB9 */
#define TMP117_I2C_SDA_PORT    GPIOB
#define TMP117_ADDR            (0x48U)       /* 7-bit I2C address (ADDR = GND) */

/* ========================================================================== */
/*  External SDRAM — IS42S32800J (8 MB, 32-bit, FMC)                         */
/* ========================================================================== */
#define SDRAM_FMC_BANK         FMC_BANK1_SDRAM1
#define SDRAM_COL_BITS         (8U)
#define SDRAM_ROW_BITS         (11U)
#define SDRAM_CLOCK_DIV        (2U)          /* SDCLK = HCLK / 2 = 120 MHz */
#define SDRAM_BURST_LEN        (1U)          /* No burst (single access) */
#define SDRAM_REFRESH_COUNT    (4096U)
#define SDRAM_REFRESH_RATE     (64U)         /* ms — 64 ms refresh window */

/* ========================================================================== */
/*  Debug UART (USB-C CDC)                                                    */
/* ========================================================================== */
#define DEBUG_UART_INST        USART3
#define DEBUG_UART_TX_PIN      GPIO_PIN_8    /* PD8 */
#define DEBUG_UART_TX_PORT     GPIOD
#define DEBUG_UART_RX_PIN      GPIO_PIN_9    /* PD9 */
#define DEBUG_UART_RX_PORT     GPIOD
#define DEBUG_UART_BAUD        (115200U)

/* ========================================================================== */
/*  System LEDs & Button                                                      */
/* ========================================================================== */
#define LED_STATUS_PIN         GPIO_PIN_14   /* PE14 — status LED (green) */
#define LED_STATUS_PORT        GPIOE
#define LED_ERROR_PIN          GPIO_PIN_15   /* PE15 — error LED (red) */
#define LED_ERROR_PORT         GPIOE
#define LED_BLE_PIN            GPIO_PIN_0    /* PE0 — BLE connected LED (blue) */
#define LED_BLE_PORT           GPIOE
#define BTN_START_PIN          GPIO_PIN_13   /* PC13 — start/scan button */
#define BTN_START_PORT         GPIOC
#define BTN_START_EXTI         EXTI13_IRQn

/* ========================================================================== */
/*  Tomography Constants                                                      */
/* ========================================================================== */
#define TOMO_MAX_SENSORS       (32U)
#define TOMO_MAX_RAYS          (496U)        /* N×(N−1)/2 for N=32 */
#define TOMO_GRID_DEFAULT      (64U)         /* 64×64 reconstruction grid */
#define TOMO_GRID_MIN          (32U)
#define TOMO_GRID_MAX          (128U)
#define TOMO_LAMBDA_DEFAULT    (0.5f)        /* Default Tikhonov parameter */
#define TOMO_LAMBDA_MIN        (0.01f)
#define TOMO_LAMBDA_MAX        (100.0f)
#define TOMO_TEMP_ALPHA        (-0.007f)     /* Velocity temp coeff (per °C) */
#define TOMO_TEMP_REF          (20.0f)       /* Reference temperature °C */

/* ========================================================================== */
/*  Acquisition Timing                                                        */
/* ========================================================================== */
#define ACQ_SETTLE_TIME_US     (10U)         /* Settle after mux switch (µs) */
#define ACQ_PRECHARGE_TIME_US  (5U)          /* Precharge HV line (µs) */
#define ACQ_DEAD_TIME_US       (50U)         /* Dead time between TX firings */
#define ACQ_CROSS_CORR_THRESH  (3.0f)        /* SNR threshold for XCorr (dB) */
#define ACQ_MAX_AMPLITUDE      (2047)        /* 12-bit ADC full-scale */

/* ========================================================================== */
/*  BLE GATT Service UUIDs (custom 128-bit, base reversed)                    */
/* ========================================================================== */
#define BLE_SVC_SONICSIGHT_UUID   "ef44e4a4-1a2b-4c3d-8e5f-6a7b8c9d0e1f"
#define BLE_CHAR_TOMOGRAM_UUID    "ef44e4a5-1a2b-4c3d-8e5f-6a7b8c9d0e1f"
#define BLE_CHAR_STATUS_UUID      "ef44e4a6-1a2b-4c3d-8e5f-6a7b8c9d0e1f"
#define BLE_CHAR_COMMAND_UUID     "ef44e4a7-1a2b-4c3d-8e5f-6a7b8c9d0e1f"
#define BLE_CHAR_TOF_RAW_UUID     "ef44e4a8-1a2b-4c3d-8e5f-6a7b8c9d0e1f"
#define BLE_CHAR_GAIN_UUID        "ef44e4a9-1a2b-4c3d-8e5f-6a7b8c9d0e1f"

/* ========================================================================== */
/*  Misc                                                                      */
/* ========================================================================== */
#define FIRMWARE_VERSION_MAJOR  (1U)
#define FIRMWARE_VERSION_MINOR  (0U)
#define FIRMWARE_VERSION_PATCH  (0U)
#define DEVICE_NAME             "SonicSight-001"
#define MANUFACTURER_NAME       "jayis1"
#define SERIAL_NUMBER_LEN       (16U)

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */