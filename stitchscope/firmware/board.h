/* StitchScope board definition
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef STITCHSCOPE_BOARD_H
#define STITCHSCOPE_BOARD_H

#include <stdint.h>

#define BOARD_NAME                  "StitchScope-A1"
#define BOARD_HARDWARE_REVISION     0x0100u
#define CPU_CLOCK_HZ                128000000u
#define MONOTONIC_TIMER_HZ          1000000u
#define FORCE_SAMPLE_HZ             8000u
#define ACCEL_SAMPLE_HZ             26667u
#define MAG_SAMPLE_HZ               1000u
#define TOF_FRAME_HZ                120u
#define PHASE_BIN_COUNT             128u
#define FEATURE_QUEUE_DEPTH         16u
#define LOG_PAGE_BYTES              256u
#define FLASH_BYTES                 (8u * 1024u * 1024u)
#define FLASH_SECTOR_BYTES          4096u
#define BLE_MAX_PAYLOAD             244u

#define PIN_FORCE_ADC_CS            10u
#define PIN_FORCE_ADC_DRDY          11u
#define PIN_ACCEL_CS                12u
#define PIN_ACCEL_INT               13u
#define PIN_HALL_INT                14u
#define PIN_QSPI_CS                 17u
#define PIN_SENSOR_RAIL_EN          20u
#define PIN_ANALOG_RAIL_EN          21u
#define PIN_LED_RED                 22u
#define PIN_LED_GREEN               23u
#define PIN_LED_BLUE                24u
#define PIN_BUTTON                  25u
#define PIN_CHARGE_STATUS           26u

#define I2C_ADDRESS_MAG             0x30u
#define I2C_ADDRESS_TOF             0x41u
#define I2C_ADDRESS_ENV             0x70u
#define I2C_ADDRESS_HAPTIC          0x5Au
#define I2C_ADDRESS_GAUGE           0x36u

#define ADC_FULL_SCALE_UV           1200000
#define FORCE_RANGE_MN              2000
#define BATTERY_EMPTY_MV            3300u
#define BATTERY_FULL_MV             4200u
#define MAX_STITCH_RATE_SPM          3000u
#define MIN_CYCLE_US                (60000000u / MAX_STITCH_RATE_SPM)
#define MAX_CYCLE_US                3000000u

#ifdef STITCHSCOPE_TARGET_NRF5340
void board_gpio_write(uint32_t pin, int value);
int board_gpio_read(uint32_t pin);
uint32_t board_time_us(void);
void board_watchdog_feed(void);
#else
void board_sim_advance(uint32_t microseconds);
#endif

#endif
