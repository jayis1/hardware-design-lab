/*
 * board.h — MûonScape hardware pin map and constants
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Target: STM32H723VIT6 (LQFP-100, 1 MB Flash, 564 KB SRAM, 4 KB ITCM)
 *
 * Pin assignments are project-internal and reflect the schematic in
 * ../kicad/device.kicad_sch. All GPIO are 3.3 V CMOS unless noted.
 */
#ifndef MUONSCAPE_BOARD_H
#define MUONSCAPE_BOARD_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Firmware version ---- */
#define FW_VERSION_MAJOR  1
#define FW_VERSION_MINOR  0
#define FW_VERSION_PATCH  0
#define FW_BUILD_ID       "muonscape-20260705"

#define AUTHOR            "jayis1"
#define COPYRIGHT         "Copyright (c) 2026 jayis1"

/* ---- Clock tree (STM32H723) ---- */
#define HSE_VALUE         25000000UL   /* 25 MHz crystal on board */
#define SYSCLK_MAX        550000000UL
#define HCLK_VALUE        275000000UL /* /2 of SYSCLK */
#define APB1_VALUE        137500000UL /* /2 of HCLK  */
#define APB2_VALUE        137500000UL /* /2 of HCLK  */
#define APB4_VALUE        137500000UL /* /2 of HCLK  */

/* ---- SPI to iCE40 FPGA (front-end hit stream) ----
 * SPI1, full-duplex, master, 50 MHz, CPOL=1/CPHA=1, MSB first.
 *   PA5  = SPI1_SCK   (AF5)
 *   PA6  = SPI1_MISO  (AF5)  <- hit data from FPGA
 *   PA7  = SPI1_MOSI  (AF5)  -> config commands to FPGA
 *   PB2  = CRESET_B   (output, push-pull) — FPGA config reset
 *   PD8  = CDONE      (input)             — FPGA config done
 *   PB10 = FPGA_IRQ   (exti, rising)      — hit FIFO non-empty
 *   PB11 = FPGA_HOLD  (output)            — freeze FIFO
 */
#define FPGA_SPI           SPI1
#define FPGA_SCK_PIN       5
#define FPGA_SCK_PORT      GPIOA
#define FPGA_MISO_PIN      6
#define FPGA_MISO_PORT     GPIOA
#define FPGA_MOSI_PIN      7
#define FPGA_MOSI_PORT     GPIOA
#define FPGA_CRESET_PORT   GPIOB
#define FPGA_CRESET_PIN    2
#define FPGA_CDONE_PORT    GPIOD
#define FPGA_CDONE_PIN     8
#define FPGA_IRQ_PORT      GPIOB
#define FPGA_IRQ_PIN       10
#define FPGA_HOLD_PORT     GPIOB
#define FPGA_HOLD_PIN      11

#define FPGA_HIT_FIFO_DEPTH  8192   /* 32-bit words in iCE40 SPRAM FIFO */
#define FPGA_HIT_WORD_BYTES  4
#define FPGA_BURST_MAX        256    /* max words per SPI burst */

/* ---- I²C bus (SiPM bias DAC, BME280, LSM6DSO, ALS, BQ76952) ----
 * I2C1, master, 400 kHz.
 *   PB8 = SCL (AF4)
 *   PB9 = SDA (AF4)
 */
#define PERIPH_I2C          I2C1
#define PERIPH_I2C_SCL_PORT GPIOB
#define PERIPH_I2C_SCL_PIN  8
#define PERIPH_I2C_SDA_PORT GPIOB
#define PERIPH_I2C_SDA_PIN  9
#define I2C_TIMEOUT_MS      100

/* I²C device addresses (7-bit) */
#define ADDR_SIPM_DAC       0x0C  /* AD5689 dual-channel 16-bit DAC for SiPM bias */
#define ADDR_BME280         0x77
#define ADDR_LSM6DSO        0x6B
#define ADDR_LTR329         0x29
#define ADDR_BQ76952        0x10
#define ADDR_ASIC_BASE      0x40  /* NUS32toA base, +0/+1/+2 per chip */

/* ---- SiPM bias supply ----
 * Programmable boost converter controlled by AD5689 DAC channel A.
 * Bias range 14.0 - 16.5 V. Thermal compensation -27 mV/°C for MICROFC-SMA.
 */
#define SIPM_BIAS_MIN_MV   14000
#define SIPM_BIAS_MAX_MV   16500
#define SIPM_BIAS_DEFAULT  15700
#define SIPM_TEMP_COEFF_MV_PER_C  (-27.0f)  /* mV per °C (negative) */
#define SIPM_CHANNELS     192

/* ---- ASIC threshold / TDC ---- */
#define ASIC_CHIP_COUNT    3
#define ASIC_CHANNELS_PER  64
#define ASIC_TOTAL_CH      (ASIC_CHIP_COUNT * ASIC_CHANNELS_PER) /* 192 */
#define ASIC_THRESHOLD_DAC_MIN  0
#define ASIC_THRESHOLD_DAC_MAX  255
#define TDC_BIN_PS        50      /* per-bin TDC resolution */
#define TDC_BINS          512     /* per channel */

/* ---- Coincidence window (FPGA) ----
 * 5 ns window between any two layers and 10 ns across all three.
 */
#define COINC_WINDOW_NS   5
#define COINC_WINDOW_3NS  10

/* ---- Detector geometry ---- */
#define LAYER_COUNT       3
#define STRIPS_PER_LAYER  16
#define STRIP_PITCH_CM    6.0f     /* 6 cm strip pitch */
#define STRIP_LENGTH_CM   96.0f   /* 96 cm active length */
#define LAYER_SPACING_CM  10.0f   /* distance between layer centres */
#define ACTIVE_AREA_CM    96.0f   /* 96 x 96 cm effective area */
#define TRACK_ANG_RES_MRAD 5.0f   /* angular resolution, mrad */

/* ---- Tomographic volume ---- */
#define VOX_X   96
#define VOX_Y   96
#define VOX_Z   32
#define VOX_SIZE_CM 3.0f
#define VOX_BYTES   (VOX_X * VOX_Y * VOX_Z * sizeof(float))

/* ---- External SRAM (8 MB pSRAM, FMC bank 1, 16-bit) ---- */
#define PSRAM_BASE        0x68000000UL
#define PSRAM_SIZE_BYTES  (8UL * 1024UL * 1024UL)

/* ---- SD card (SDMMC1) ---- */
#define SDMMC_SD          SDMMC1
#define SD_DETECT_PIN     12
#define SD_DETECT_PORT    GPIOC

/* ---- USB-C ---- */
#define USB_OTG           USB1
#define USB_VBUS_PIN       9
#define USB_VBUS_PORT      GPIOA

/* ---- Wi-Fi ATWINC1500 (SPI3) ---- */
#define WIFI_SPI          SPI3
#define WIFI_IRQ_PIN      0
#define WIFI_IRQ_PORT     GPIOD
#define WIFI_CS_PIN       4
#define WIFI_CS_PORT      GPIOD
#define WIFI_RESET_PIN    5
#define WIFI_RESET_PORT   GPIOD
#define WIFI_PORT         7000   /* TCP listen port for app commands */

/* ---- GPS NEO-M9N (USART2) ---- */
#define GPS_USART         USART2
#define GPS_PPS_PIN       4
#define GPS_PPS_PORT      GPIOD
#define GPS_BAUD          9600

/* ---- RS-485 (USART3, optional stereo link) ---- */
#define RS485_USART       USART3
#define RS485_DE_PIN      1
#define RS485_DE_PORT     GPIOC

/* ---- Battery / charger ----
 * BQ76952 on I2C1 (addr 0x10). BQ24650 charger controlled by PWM on TIM8 CH1.
 * Solar input on PA0 (analog), USB-C PD on PA1 (analog).
 */
#define CHARGER_PWM_TIM   TIM8
#define CHARGER_PWM_CH    1
#define SOLAR_VSENSE_ADC  ADC1
#define SOLAR_VSENSE_CH   0
#define USB_VSENSE_CH     1
#define BATT_VSENSE_CH    2
#define BATT_ISENSE_CH    3

/* ---- Status LEDs ---- */
#define LED_STATUS_PORT   GPIOE
#define LED_STATUS_PIN    0     /* green: armed/acquiring */
#define LED_FAULT_PORT    GPIOE
#define LED_FAULT_PIN     1     /* red: fault */
#define LED_GPS_PORT      GPIOE
#define LED_GPS_PIN       2     /* blue: GPS lock */
#define LED_WIFI_PORT     GPIOE
#define LED_WIFI_PIN      3     /* amber: Wi-Fi connected */

/* ---- Push-button ---- */
#define BTN_USER_PORT     GPIOC
#define BTN_USER_PIN      13
#define BTN_DEBOUNCE_MS   50

/* ---- Acquisition timing ---- */
#define ACQ_DEFAULT_SEC   (12UL * 3600UL)  /* 12 h default */
#define ACQ_MAX_SEC       (90UL * 24UL * 3600UL) /* 90 d max */
#define PREVIEW_UPDATE_MS (5UL * 60UL * 1000UL) /* 5 min preview cadence */
#define EVENT_FLUSH_MS    1000     /* flush events to SD every 1 s */
#define STATS_LOG_MS      60000    /* log stats every minute */

/* ---- muon flux constants ---- */
#define MUON_VERT_FLUX   70.0f    /* muons / m^2 / s / sr vertical */
#define MUON_AREA_M2     0.9216f   /* 96 x 96 cm */
#define MUON_RATE_HZ     (MUON_VERT_FLUX * MUON_AREA_M2 * 2.0f * 3.14159f / 2.0f)

/* ---- Error codes ---- */
typedef enum {
    MUON_OK = 0,
    MUON_ERR_SPI,
    MUON_ERR_I2C,
    MUON_ERR_FPGA_LOAD,
    MUON_ERR_ASIC_INIT,
    MUON_ERR_SIPM_BIAS,
    MUON_ERR_SD,
    MUON_ERR_WIFI,
    MUON_ERR_GPS,
    MUON_ERR_BATT,
    MUON_ERR_THERMAL,
    MUON_ERR_MEMORY,
    MUON_ERR_INVALID_ARG,
    MUON_ERR_BUSY,
    MUON_ERR_TIMEOUT
} muon_status_t;

/* ---- System state ---- */
typedef enum {
    STATE_BOOT = 0,
    STATE_IDLE,
    STATE_CALIBRATING,
    STATE_ACQUIRING,
    STATE_PROCESSING,
    STATE_SLEEP,
    STATE_FAULT
} system_state_t;

/* ---- Return code helpers ---- */
#define MUON_RETURN_ON_ERR(x) do { muon_status_t _s = (x); if (_s != MUON_OK) return _s; } while (0)
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_BOARD_H */