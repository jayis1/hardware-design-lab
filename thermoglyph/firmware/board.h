/*
 * board.h — Pin assignments and peripheral configuration for ThermoGlyph
 *
 * Hardware: nRF5340 + SX1262 LoRa + 96-cell thermal TEC array
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 *  Board identity
 * ========================================================================= */
#define BOARD_NAME          "ThermoGlyph"
#define BOARD_REV           "1.0"
#define BOARD_AUTHOR        "jayis1"

/* =========================================================================
 *  nRF5340 GPIO pin assignments (QFAA package, 73-pin)
 *  Pin numbering uses nRF5340 P0.xx / P1.xx convention
 * ========================================================================= */

/* --- Debug / SWD --- */
#define PIN_SWDIO           (0, 20)
#define PIN_SWCLK           (0, 19)
#define PIN_RESET_N         (0, 21)

/* --- USB-C --- */
#define PIN_USB_DPLUS       (0, 13)
#define PIN_USB_DMINUS      (0, 14)
#define PIN_USB_VBUS_DETECT (0, 15)

/* --- LEDs (status indicator) --- */
#define PIN_LED_RED         (0, 28)
#define PIN_LED_GREEN       (0, 29)
#define PIN_LED_BLUE        (0, 30)

/* --- I2C bus 0 (TMP117 skin temp sensors, Si7051, LTR-390UV, MAX17048) --- */
#define PIN_I2C0_SCL        (0, 27)
#define PIN_I2C0_SDA        (0, 26)
#define I2C0_FREQ_HZ        400000U

/* --- I2C bus 1 (second bank of TMP117 sensors) --- */
#define PIN_I2C1_SCL        (1, 0)
#define PIN_I2C1_SDA        (0, 31)
#define I2C1_FREQ_HZ        400000U

/* --- SPI 0: SX1262 LoRa --- */
#define PIN_LORA_SCLK       (0, 22)
#define PIN_LORA_MOSI       (0, 23)
#define PIN_LORA_MISO       (0, 24)
#define PIN_LORA_NSS        (0, 25)
#define PIN_LORA_BUSY       (0, 2)
#define PIN_LORA_DIO1       (0, 3)
#define PIN_LORA_DIO2       (0, 4)
#define PIN_LORA_ANT_SW     (0, 5)   /* RF switch control */
#define PIN_LORA_RESET_N    (0, 6)
#define LORA_SPI_FREQ_HZ    8000000U

/* --- SPI 1: ICM-42688 IMU --- */
#define PIN_IMU_SCLK        (1, 1)
#define PIN_IMU_MOSI        (1, 2)
#define PIN_IMU_MISO        (1, 3)
#define PIN_IMU_CS          (1, 4)
#define PIN_IMU_INT1        (1, 5)
#define PIN_IMU_INT2        (1, 6)
#define IMU_SPI_FREQ_HZ     1000000U

/* --- Thermal array: TLC59711 column drivers (4 daisy-chained) --- */
#define PIN_TLC_SCLK        (0, 7)
#define PIN_TLC_SDATA       (0, 8)
#define PIN_TLC_LATCH       (0, 9)
#define PIN_TLC_BLANK       (0, 10)

/* --- Thermal array: ADG711 row-select (2 chips, 8 rows) --- */
#define PIN_ROW_SEL_A0      (0, 11)
#define PIN_ROW_SEL_A1      (0, 12)
#define PIN_ROW_SEL_A2      (1, 7)
#define PIN_ROW_DIR0        (1, 8)   /* heating/cooling direction row bank 0 */
#define PIN_ROW_DIR1        (1, 9)   /* heating/cooling direction row bank 1 */

/* --- Safety: THERM_FAULT line (active low, hardware comparator) --- */
#define PIN_THERM_FAULT     (1, 10)
#define PIN_TEC_PWR_EN      (1, 11)  /* P-FET gate: 1 = TEC power on */

/* --- BQ25570 solar harvester --- */
#define PIN_SOLAR_OK        (1, 12)  /* VBAT_OK signal from BQ25570 */
#define PIN_SOLAR_VDIV      (0, 16)  /* ADC input: solar voltage divider */

/* --- Battery fuel gauge (MAX17048 on I2C0) --- */
#define PIN_BATT_ALRT       (1, 13)  /* MAX17048 alert interrupt */

/* --- NFC antenna (unused, configured as GPIO) --- */
#define PIN_NFC1            (0, 9)
#define PIN_NFC2            (0, 10)

/* =========================================================================
 *  I2C addresses
 * ========================================================================= */
#define TMP117_BASE_ADDR    0x48U    /* TMP117 addresses 0x48–0x51 */
#define TMP117_COUNT        12U      /* 12 sensors (6 on I2C0, 6 on I2C1) */

#define SI7051_ADDR         0x40U
#define LTR390UV_ADDR       0x53U
#define MAX17048_ADDR       0x36U

/* =========================================================================
 *  Thermal array geometry
 * ========================================================================= */
#define THERMAL_COLS        12U
#define THERMAL_ROWS        8U
#define THERMAL_CELLS       (THERMAL_COLS * THERMAL_ROWS)  /* 96 */

/* Temperature safety limits (degrees C × 100 for fixed-point) */
#define TEMP_MIN_C          1800U    /* 18.00 °C minimum skin temp target */
#define TEMP_MAX_C          4200U    /* 42.00 °C maximum skin temp target */
#define TEMP_FAULT_C        4400U    /* 44.00 °C hardware fault threshold */
#define TEMP_AMBIENT_OFFSET 200U     /* 2.00 °C from ambient baseline */

/* PWM driver configuration */
#define TLC_PWM_STEPS       65536U   /* TLC59711 16-bit PWM */
#define ROW_SCAN_HZ         200U     /* row scan frequency */
#define ROW_SLOT_US         (1000000U / ROW_SCAN_HZ / THERMAL_ROWS)  /* 625 µs */

/* PID configuration */
#define PID_RATE_HZ         200U
#define PID_KP              120
#define PID_KI              8
#define PID_KD              15
#define PID_WINDUP_LIMIT    2000U

/* =========================================================================
 *  LoRa configuration
 * ========================================================================= */
#define LORA_FREQ_HZ        868100000U  /* EU868 default; 915000000 for US915 */
#define LORA_BW_KHZ         125U
#define LORA_SF             7U          /* Spreading factor 7 */
#define LORA_CR             5U          /* Coding rate 4/5 */
#define LORA_TX_POWER_DBM   17U
#define LORA_PREAMBLE_LEN   8U
#define LORA_SYNC_WORD      0x34U      /* private network */

/* =========================================================================
 *  BLE configuration
 * ========================================================================= */
#define BLE_DEVICE_NAME     "ThermoGlyph"
#define BLE_SERVICE_UUID    0x54, 0x47, 0x01, 0x00  /* "TG01" */
#define BLE_CHAR_GLYPH      0x54, 0x47, 0x01, 0x01  /* write: glyph command */
#define BLE_CHAR_GESTURE    0x54, 0x47, 0x02, 0x01  /* notify: gesture event */
#define BLE_CHAR_TELEMETRY  0x54, 0x47, 0x03, 0x01  /* notify: telemetry */
#define BLE_CHAR_CONFIG     0x54, 0x47, 0x04, 0x01  /* write: config */
#define BLE_CHAR_DFU        0x54, 0x47, 0x05, 0x01  /* write: OTA DFU */

/* =========================================================================
 *  Glyph engine configuration
 * ========================================================================= */
#define GLYPH_FRAME_HZ      50U       /* 50 Hz frame rate */
#define GLYPH_QUEUE_LEN     32U       /* max queued glyph commands */
#define GLYPH_SCROLL_SPEED  4U        /* columns per second for text scroll */

/* =========================================================================
 *  Power management
 * ========================================================================= */
#define POWER_ACTIVE_MA     35U
#define POWER_IDLE_MA       3U
#define POWER_SLEEP_MA      0.3U
#define BATTERY_CAPACITY_MAH 500U
#define BATTERY_LOW_PCT     15U
#define BATTERY_CRIT_PCT    5U

/* Solar: if VBAT_OK deasserts, enter solar-sustain mode */
#define SOLAR_SUSTAIN_THRESHOLD_MA  10U

/* =========================================================================
 *  IMU gesture thresholds
 * ========================================================================= */
#define GESTURE_TAP_ACCEL_THRESH   2000U    /* 2.0 g in mg */
#define GESTURE_TAP_DUR_MS         80U
#define GESTURE_DOUBLE_TAP_GAP_MS  400U
#define GESTURE_FLIP_PITCH_DEG     90U
#define GESTURE_FLIP_DUR_MS        500U
#define GESTURE_SHAKE_ACCEL_THRESH 1500U    /* 1.5 g */
#define GESTURE_SHAKE_DUR_MS       300U

/* =========================================================================
 *  Watchdog
 * ========================================================================= */
#define WATCHDOG_TIMEOUT_MS  200U     /* thermal_task must kick every 50 ms */

/* =========================================================================
 *  FreeRTOS configuration
 * ========================================================================= */
#define RTOS_TICK_HZ        1000U
#define GLYPH_TASK_PRIO     4U
#define THERMAL_TASK_PRIO   5U
#define IMU_TASK_PRIO       3U
#define COMM_TASK_PRIO     2U

#define GLYPH_STACK_BYTES   2048U
#define THERMAL_STACK_BYTES 2048U
#define IMU_STACK_BYTES     1536U
#define COMM_STACK_BYTES    2048U

#endif /* BOARD_H */