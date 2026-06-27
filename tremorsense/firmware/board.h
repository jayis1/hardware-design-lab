/*
 * board.h — TremorSense board configuration and pin assignments
 * TremorSense — Wearable Tremor Characterization Band
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Pin assignments for the nRF5340 QFAA (aQFN73) on the TremorSense PCB.
 * All signals routed through the 4-layer round board (35 mm diameter).
 */

#ifndef TREMORSENSE_BOARD_H
#define TREMORSENSE_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Board identity ---- */
#define BOARD_NAME         "TremorSense-v1"
#define BOARD_REV          1
#define FIRMWARE_VERSION   "1.0.0"
#define AUTHOR             "jayis1"

/* ---- Clock configuration ---- */
#define HFCLK_FREQ_HZ      32000000UL   /* HFXO 32 MHz */
#define LFCLK_FREQ_HZ      32768UL      /* LFXO 32.768 kHz */
#define SYSTICK_FREQ_HZ    1000UL       /* 1 ms tick */

/* ---- GPIO pin assignments (nRF5340 app core P0 port) ---- */
/* IMU: ICM-42688-P via SPI2 */
#define IMU_SPI_INSTANCE   2
#define IMU_CS_PIN         7    /* P0.07 — SPI CS for IMU */
#define IMU_IRQ_PIN        11   /* P0.11 — FIFO watermark interrupt */
#define IMU_SCLK_PIN       4    /* P0.04 — SPI clock */
#define IMU_MOSI_PIN       5    /* P0.05 — SPI MOSI (SDA) */
#define IMU_MISO_PIN       6    /* P0.06 — SPI MISO */

/* Display: GC9A01 OLED via SPI0 */
#define DISP_SPI_INSTANCE  0
#define DISP_CS_PIN        8    /* P0.08 — SPI CS for OLED */
#define DISP_DC_PIN        9    /* P0.09 — Data/Command */
#define DISP_SCLK_PIN      10   /* P0.10 — SPI clock (shared with IMU? No, separate) */
#define DISP_MOSI_PIN      12   /* P0.12 — SPI MOSI */
#define DISP_RST_PIN       13   /* P0.13 — Reset */
#define DISP_BL_PIN        14   /* P0.14 — Backlight enable */

/* Flash: MX25R6435F via SPI1 */
#define FLASH_SPI_INSTANCE 1
#define FLASH_CS_PIN       15   /* P0.15 — SPI CS for flash */
#define FLASH_SCLK_PIN     17   /* P0.17 — SPI clock */
#define FLASH_MOSI_PIN     18   /* P0.18 — SPI MOSI */
#define FLASH_MISO_PIN     19   /* P0.19 — SPI MISO */
#define FLASH_HOLD_PIN     20   /* P0.20 — Hold (active low) */
#define FLASH_WP_PIN       21   /* P0.21 — Write protect (active low) */

/* I²C bus for fuel gauge + environmental sensors */
#define I2C_INSTANCE       0
#define I2C_SCL_PIN        26   /* P0.26 */
#define I2C_SDA_PIN        27   /* P0.27 */
#define I2C_FREQ_HZ        400000UL

/* I²C device addresses (7-bit) */
#define FUEL_GAUGE_ADDR    0x36   /* MAX17048 */
#define SHT40_ADDR         0x44   /* Sensirion SHT40 */
#define LPS22HB_ADDR       0x5D   /* ST LPS22HB (SA1=high) */

/* USB-C */
#define USB_DP_PIN         24   /* P0.24 */
#define USB_DM_PIN         25   /* P0.25 */

/* User button */
#define BUTTON_PIN         23   /* P0.23 — multi-function side button */
#define BUTTON_ACTIVE_LOW  1    /* Pulled high, pressed = GND */

/* Battery monitoring ADC */
#define VBAT_SENSE_PIN     4    /* P0.04 (AIN4) — via divider 2:1 */
#define VBAT_DIVIDER       2    /* 2:1 resistor divider */

/* NFC antenna pins (fixed on nRF5340) */
#define NFC_ANT1_PIN       28   /* P0.28 */
#define NFC_ANT2_PIN       29   /* P0.29 */

/* Charging status from MCP73831 */
#define CHARGE_STAT_PIN    31   /* P0.31 — open-drain, active low = charging */

/* ---- Power management ---- */
#define BATTERY_CAPACITY_MAH  120
#define BATTERY_FULL_MV       4200
#define BATTERY_EMPTY_MV      3200
#define BATTERY_WARN_MV       3400   /* 20% warning */
#define BATTERY_CRIT_MV       3300   /* 5% critical shutdown */

/* ---- IMU configuration ---- */
#define IMU_ODR_HZ          1000   /* Output data rate */
#define IMU_ACC_RANGE_G     16     /* ±16 g */
#define IMU_GYRO_RANGE_DPS  2000   /* ±2000 dps */
#define IMU_FIFO_WATERMARK  50     /* 50 samples = 50 ms at 1 kHz */
#define IMU_SPI_MAX_HZ     24000000UL

/* Sample size: 6 axes × 2 bytes = 12 bytes per sample */
#define IMU_SAMPLE_SIZE     12
#define IMU_WINDOW_SAMPLES  50     /* 50 ms window at 1 kHz */
#define IMU_WINDOW_BYTES    (IMU_WINDOW_SAMPLES * IMU_SAMPLE_SIZE) /* 600 bytes */

/* ---- DSP configuration ---- */
#define FFT_SIZE             256
#define TREMOR_BAND_LOW_HZ   3.5f
#define TREMOR_BAND_HIGH_HZ  12.0f
#define FFT_HOP_SAMPLES       32      /* 87.5% overlap with 256-pt window */
#define SAMPLE_RATE_HZ        1000.0f
#define FREQ_RESOLUTION_HZ    (SAMPLE_RATE_HZ / (float)FFT_SIZE) /* ~3.9 Hz */

/* ---- ML model ---- */
#define ML_FEATURE_COUNT     16
#define ML_CLASS_COUNT       5
#define ML_CONFIDENCE_THRESH 0.70f
#define ML_MIN_EPISODE_SEC   3.0f    /* 3-second sustained detection → episode */

/* ---- Storage ---- */
#define FLASH_SIZE_BYTES     (8UL * 1024UL * 1024UL)
#define EPISODE_MAX_SIZE     2048
#define EPISODE_MAGIC        0x54530301  /* 'TS01' */
#define FLASH_RETENTION_DAYS 14
#define FLASH_PAGE_SIZE      4096
#define FLASH_SECTOR_SIZE    65536

/* ---- BLE ---- */
#define BLE_ADV_INTERVAL_MS      1000
#define BLE_CONN_INTERVAL_MIN_MS 100
#define BLE_CONN_INTERVAL_MAX_MS 200
#define BLE_TX_POWER_DBM         0
#define BLE_NOTIFY_RATE_HZ       1

/* BLE custom service UUIDs (16-bit shortened for readability) */
#define BLE_UUID_TREMOR_SERVICE  0x0001
#define BLE_UUID_TREMOR_STATUS   0x0002
#define BLE_UUID_TREMOR_SCORE    0x0003
#define BLE_UUID_DATA_SERVICE    0x0010
#define BLE_UUID_EPISODE_CHUNK   0x0011
#define BLE_UUID_DOWNLOAD_CMD    0x0012
#define BLE_UUID_CONFIG_SERVICE  0x0020
#define BLE_UUID_SAMPLE_RATE     0x0021
#define BLE_UUID_SENSITIVITY     0x0022

/* ---- Display ---- */
#define DISP_WIDTH   240
#define DISP_HEIGHT  240
#define DISP_SPI_HZ  40000000UL

/* ---- Timing constants ---- */
#define DISPLAY_REFRESH_IDLE_MS   1000  /* 1 Hz when idle */
#define DISPLAY_REFRESH_ACTIVE_MS 67   /* ~15 Hz during tremor */
#define BLE_BATCH_INTERVAL_MS     2000 /* 0.5 Hz for batch features */

/* ---- State machine states ---- */
typedef enum {
    STATE_IDLE = 0,
    STATE_SAMPLE_IMU,
    STATE_PREPROCESS,
    STATE_FEATURE_EXTRACT,
    STATE_ML_INFER,
    STATE_LOG_EPISODE,
    STATE_UPDATE_DISPLAY,
    STATE_BLE_NOTIFY,
    STATE_SLEEP,
    STATE_CALIBRATION,
    STATE_LOW_BATTERY,
    STATE_USB_CONNECTED,
} device_state_t;

/* ---- Global board configuration structure ---- */
typedef struct {
    uint8_t  sample_rate_hz;
    uint8_t  sensitivity;       /* 1=low, 2=medium, 3=high */
    float    freq_low_hz;
    float    freq_high_hz;
    float    confidence_thresh;
    uint8_t  display_on;
    uint8_t  ble_enabled;
    uint8_t  nfc_dose_logging;
    uint32_t device_id;
} board_config_t;

/* Default configuration */
#define DEFAULT_CONFIG { \
    .sample_rate_hz = 100, \
    .sensitivity = 2, \
    .freq_low_hz = 3.5f, \
    .freq_high_hz = 12.0f, \
    .confidence_thresh = 0.70f, \
    .display_on = 1, \
    .ble_enabled = 1, \
    .nfc_dose_logging = 1, \
    .device_id = 0x54530001 \
}

/* ---- Function prototypes ---- */
void board_init(void);
void board_deinit(void);
uint32_t board_millis(void);
void board_delay_ms(uint32_t ms);
void board_enter_sleep(void);
void board_wakeup(void);
uint16_t board_read_battery_mv(void);
uint8_t  board_battery_percent(void);
uint8_t  board_is_charging(void);
uint32_t board_get_device_id(void);

/* GPIO helpers */
void gpio_set(uint8_t pin, uint8_t val);
uint8_t gpio_read(uint8_t pin);
void gpio_toggle(uint8_t pin);
void gpio_config_output(uint8_t pin);
void gpio_config_input(uint8_t pin, uint8_t pullup);

/* Interrupt enable/disable */
uint32_t irq_save(void);
void irq_restore(uint32_t primask);

/* Critical section helpers */
#define CRITICAL_ENTER()  uint32_t __pm = irq_save()
#define CRITICAL_EXIT()    irq_restore(__pm)

#endif /* TREMORSENSE_BOARD_H */