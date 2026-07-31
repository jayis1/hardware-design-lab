/*
 * power.h — Power management for Synthand.
 *
 * Battery gauge (BQ27426), charging status, temperature monitoring,
 * sleep/ship mode control.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SYNTHAND_POWER_H
#define SYNTHAND_POWER_H

#include <stdint.h>
#include "board.h"

/* Initialize power management (BQ27426 gauge, ADC for temp/battery).
 * Returns 0 on success. */
int power_init(void);

/* Read battery voltage in millivolts.
 * Uses BQ27426 if available, falls back to SAADC divider. */
uint32_t power_read_battery_mv(void);

/* Read battery state of charge in percent (0-100). */
uint8_t power_read_battery_pct(void);

/* Read battery current in milliamps (signed: + = charging, - = discharging). */
int16_t power_read_battery_ma(void);

/* Read PCB temperature in milli-celsius (via NTC + SAADC).
 * Also checks nRF5340 internal temp sensor as cross-reference. */
int32_t power_read_temp_mc(void);

/* Check if USB power is connected (charging). */
int power_is_charging(void);

/* Enter low-power sleep mode (STOP equivalent).
 * Wakes on GPIO (button) or BLE event. Sensors are powered down. */
void power_enter_sleep(void);

/* Enter ship mode (cut battery via FET — device is fully off).
 * The only way out is USB-C insertion (which toggles the FET). */
void power_enter_ship_mode(void);

/* Set the power-low warning threshold in millivolts. */
void power_set_low_threshold(uint32_t mv);

/* BQ27426 I²C commands */
#define BQ27426_ADDR        0x55
#define BQ27426_REG_CNTL    0x00
#define BQ27426_REG_TEMP    0x02  /* 0.1°K resolution */
#define BQ27426_REG_VOLT    0x04  /* mV */
#define BQ27426_REG_FLAGS   0x06
#define BQ27426_REG_NOM_AVC 0x08  /* nominal available capacity mAh */
#define BQ27426_REG_AVAIL_AVC 0x0A
#define BQ27426_REG_AMPS    0x0C  /* signed mA */
#define BQ27426_REG_SOC     0x1C  /* state of charge % */
#define BQ27426_REG_OP_STAT 0x3C

/* Control subcommands */
#define BQ27426_CTRL_SUBCTRL_STATUS  0x0000
#define BQ27426_CTRL_DEVICE_TYPE     0x0001
#define BQ27426_CTRL_RESET           0x0041

#endif /* SYNTHAND_POWER_H */