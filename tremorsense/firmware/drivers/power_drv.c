/*
 * power_drv.c — Power management driver implementation
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Manages battery (MAX17048 fuel gauge), charging (MCP73831),
 * voltage regulation (TPS62840), and nRF5340 power states.
 */

#include "power_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

static uint8_t  batt_percent = 80;
static uint16_t batt_mv = 3700;
static uint8_t  charging = 0;
static int8_t   charge_current = 0;
static uint32_t cpu_freq_mhz = 128;

/* ---- I²C helpers ---- */
static void i2c_write(uint8_t addr, uint8_t reg, uint16_t value)
{
    /* In production: use nRF TWIM0 */
    (void)addr; (void)reg; (void)value;
}

static uint16_t i2c_read16(uint8_t addr, uint8_t reg)
{
    /* In production: TWIM0 read 2 bytes */
    (void)addr; (void)reg;
    return 0;
}

static void i2c_read_buf(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    (void)addr; (void)reg; (void)buf; (void)len;
}

/* ---- MAX17048 fuel gauge ---- */
int power_init(void)
{
    /* Initialize MAX17048:
     * 1. Read STATUS register, check for POR (power-on reset)
     * 2. If POR: write CONFIG(0x8000) to initiate software reset
     * 3. Wait 500ms for model loading
     * 4. Read and verify VERSION register (expected 0x0040)
     * 5. Write custom model table if needed
     * 6. Read initial VCELL and SOC
     */

    uint16_t status = i2c_read16(FUEL_GAUGE_ADDR, FG_REG_STATUS);
    if (status & 0x0001) {  /* RI bit: reset indicator */
        /* Quick-start: write CONFIG with reset bit */
        i2c_write(FUEL_GAUGE_ADDR, FG_REG_CONFIG, 0x8400);
        /* board_delay_ms(500); */
    }

    /* Read initial battery state */
    batt_mv = power_get_battery_mv();
    batt_percent = power_get_battery_percent();

    return 0;
}

uint8_t power_get_battery_percent(void)
{
    /* Read SOC register (0x04) from MAX17048
     * Value is in 1/256 % units
     */
    uint16_t soc_raw = i2c_read16(FUEL_GAUGE_ADDR, FG_REG_SOC);
    float pct = (float)soc_raw * FG_SOC_LSB_PCT;
    if (pct > 100.0f) pct = 100.0f;
    if (pct < 0.0f) pct = 0.0f;
    batt_percent = (uint8_t)pct;
    return batt_percent;
}

uint16_t power_get_battery_mv(void)
{
    /* Read VCELL register (0x02)
     * Value is in 78.125 µV per cell, 16-bit unsigned
     * Vcell_mV = raw * 78.125e-6 * 1000 = raw * 0.078125
     * But it's 12-bit >> 4, so: Vcell = (raw >> 4) * 1.25 mV
     */
    uint16_t vcell_raw = i2c_read16(FUEL_GAUGE_ADDR, FG_REG_VCELL);
    batt_mv = (uint16_t)((vcell_raw >> 4) * (uint16_t)FG_VCELL_LSB_MV);
    return batt_mv;
}

uint8_t power_is_charging(void)
{
    /* Read charge status pin from MCP73831 (active low = charging) */
    /* charging = (gpio_read(CHARGE_STAT_PIN) == 0) ? 1 : 0; */
    return charging;
}

int8_t power_get_charge_current_ma(void)
{
    /* MCP73831 charge current = PROG resistor value
     * I_reg = 1000V / R_PROG (kΩ)
     * Default R_PROG = 2kΩ → 500mA
     * Read CRATE from MAX17048 for actual current
     */
    uint16_t crate_raw = i2c_read16(FUEL_GAUGE_ADDR, FG_REG_CRATE);
    float crate_pct_hr = (float)((int16_t)crate_raw) * (1.0f / 256.0f);
    /* Convert %/hr to mA: current = (%/hr) × capacity / 100 */
    charge_current = (int8_t)(crate_pct_hr * BATTERY_CAPACITY_MAH / 100.0f);
    return charge_current;
}

uint16_t power_get_estimated_runtime_hours(void)
{
    /* Estimate remaining runtime based on current consumption */
    /* Average consumption: ~1.5 mA during active monitoring */
    float avg_current_ua = 1500.0f;
    float remaining_mah = (float)batt_percent * BATTERY_CAPACITY_MAH / 100.0f;
    if (avg_current_ua < 0.1f) return 9999;
    float hours = (remaining_mah * 1000.0f) / avg_current_ua;
    return (uint16_t)hours;
}

void power_enter_deep_sleep(void)
{
    /* Put peripherals in low-power state */
    /* display_off(); */
    /* flash_enter_powerdown(); */
    /* imu_configure_wake_on_motion(50);  // 50mg threshold */

    /* Configure nRF5340 for System OFF:
     * - Wake on GPIO (button or IMU IRQ)
     * - RAM retention enabled
     * - RTC2 running for timekeeping
     * - Network core suspended
     */
    /* nrf_pmu_task_trigger(NRF_PMU_TASK_ENTER_DEEPSLEEP); */
}

void power_wake_from_deep_sleep(void)
{
    /* Restore peripherals */
    /* flash_wakeup(); */
    /* imu_wake_from_sleep(); */
    /* display_on(); */
}

void power_set_cpu_frequency_mhz(uint32_t mhz)
{
    /* nRF5340 supports 128 MHz or 64 MHz on app core */
    if (mhz == 128) {
        /* Configure HFCLK for 128 MHz via DPLL */
        cpu_freq_mhz = 128;
    } else if (mhz == 64) {
        /* Configure for 64 MHz */
        cpu_freq_mhz = 64;
    } else if (mhz == 32) {
        cpu_freq_mhz = 32;
    }
}

uint32_t power_get_cpu_frequency_mhz(void)
{
    return cpu_freq_mhz;
}

void power_enable_peripheral(uint8_t peripheral_id)
{
    /* Enable specific peripheral power domain
     * 0=IMU, 1=Display, 2=Flash, 3=BLE, 4=Env sensors
     */
    (void)peripheral_id;
}

void power_disable_peripheral(uint8_t peripheral_id)
{
    (void)peripheral_id;
}

uint32_t power_get_current_draw_ua(void)
{
    /* In production: measure via MAX17048 CRATE */
    /* For now: estimate based on active peripherals */
    uint32_t total = 0;
    total += 500;   /* MCU active (128 MHz) */
    total += 600;   /* IMU sampling at 1 kHz */
    total += 20;    /* BLE advertising */
    /* total += 300;  // Display on (1 Hz) */
    /* total += 50;   // I²C sensors */
    return total;
}

/* ---- End of power_drv.c ---- */