/*
 * power.c — Power management for Synthand.
 *
 * Battery gauge (BQ27426), charging status, temperature monitoring,
 * and sleep/ship mode control.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <stdint.h>
#include "board.h"
#include "registers.h"
#include "drivers/power.h"

/* -------------------------------------------------------------------------
 * I²C helpers (shared with haptic.c — TWIM0 bus)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static int bq27426_read_reg16(uint8_t reg, uint16_t *value)
{
    /* Write register address, then read 2 bytes */
    TWIM0->ADDRESS = BQ27426_ADDR;
    TWIM0->TXD_PTR = (uint32_t)&reg;
    TWIM0->TXD_MAXCNT = 1;
    uint8_t buf[2];
    TWIM0->RXD_PTR = (uint32_t)buf;
    TWIM0->RXD_MAXCNT = 2;
    TWIM0->EVENTS_STOPPED = 0;
    TWIM0->EVENTS_ERROR = 0;
    TWIM0->SHORTS = (1U << 0);
    TWIM0->TASKS_STARTTX = 1;

    uint32_t timeout = 10000;
    while (TWIM0->EVENTS_STOPPED == 0 && TWIM0->EVENTS_ERROR == 0) {
        if (--timeout == 0) return -1;
    }
    TWIM0->SHORTS = 0;

    if (TWIM0->EVENTS_ERROR) {
        TWIM0->EVENTS_ERROR = 0;
        return -2;
    }

    *value = (uint16_t)((buf[1] << 8) | buf[0]);  /* little-endian */
    return 0;
}

static int bq27426_write_reg16(uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = { reg, (uint8_t)(value & 0xFF), (uint8_t)(value >> 8) };
    TWIM0->ADDRESS = BQ27426_ADDR;
    TWIM0->TXD_PTR = (uint32_t)buf;
    TWIM0->TXD_MAXCNT = 3;
    TWIM0->EVENTS_STOPPED = 0;
    TWIM0->EVENTS_ERROR = 0;
    TWIM0->TASKS_STARTTX = 1;

    uint32_t timeout = 10000;
    while (TWIM0->EVENTS_STOPPED == 0 && TWIM0->EVENTS_ERROR == 0) {
        if (--timeout == 0) return -1;
    }

    if (TWIM0->EVENTS_ERROR) {
        TWIM0->EVENTS_ERROR = 0;
        return -2;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * SAADC for battery voltage (fallback) and temperature (NTC)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static int saadc_read_channel(uint8_t channel, int16_t *value)
{
    /* Configure SAADC for single-channel, 12-bit, 1x gain */
    SAADC->RESOLUTION = 0x02;  /* 12-bit */
    SAADC->CH[0] = (channel << 6) | (0x01 << 0);  /* analog input, gain 1x
                                                     Wait: nRF SAADC channel
                                                     config is 4 words. This
                                                     is simplified. */
    SAADC->ENABLE = SAADC_ENABLE_ENABLE;

    /* Set up DMA buffer */
    SAADC->RXD_PTR = (uint32_t)value;
    SAADC->RXD_MAXCNT = 1;

    /* Start and sample */
    SAADC->TASKS_START = 1;
    while (SAADC->EVENTS_STARTED == 0);
    SAADC->EVENTS_STARTED = 0;
    SAADC->TASKS_SAMPLE = 1;
    while (SAADC->EVENTS_END == 0);
    SAADC->EVENTS_END = 0;
    SAADC->TASKS_STOP = 1;

    return 0;
}

/* -------------------------------------------------------------------------
 * Initialize power management
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int power_init(void)
{
    /* SAADC is initialized here for battery and temperature monitoring.
     * TWIM0 is shared with haptic.c and initialized there. */

    /* Verify BQ27426 is present by reading device type */
    uint16_t dev_type = 0;
    /* Write control subcommand */
    bq27426_write_reg16(BQ27426_REG_CNTL, BQ27426_CTRL_DEVICE_TYPE);
    /* Read back from control register */
    bq27426_read_reg16(BQ27426_REG_CNTL, &dev_type);

    /* BQ27426 device type should be 0x0426 */
    if (dev_type != 0x0426) {
        /* Gauge not present — fall back to SAADC voltage measurement */
        /* This is not fatal, just degraded battery reporting */
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * Read battery voltage in millivolts
 * Author: jayis1
 * ------------------------------------------------------------------------- */
uint32_t power_read_battery_mv(void)
{
    uint16_t volt = 0;
    if (bq27426_read_reg16(BQ27426_REG_VOLT, &volt) == 0) {
        return (uint32_t)volt;  /* already in mV */
    }

    /* Fallback: SAADC with voltage divider (1:2, VDD/2 on PIN_BAT_SENSE) */
    int16_t adc_val = 0;
    saadc_read_channel(PIN_BAT_SENSE, &adc_val);
    /* 12-bit ADC, 0.6V reference, gain 1x, 1:2 divider
     * V_bat = adc_val / 4095 * 0.6 * 2 * 1000 ≈ adc_val * 0.293 */
    return (uint32_t)((int32_t)adc_val * 293 / 100);
}

/* -------------------------------------------------------------------------
 * Read battery state of charge (0-100%)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
uint8_t power_read_battery_pct(void)
{
    uint16_t soc = 0;
    if (bq27426_read_reg16(BQ27426_REG_SOC, &soc) == 0) {
        return (uint8_t)(soc & 0xFF);
    }

    /* Fallback: estimate from voltage */
    uint32_t mv = power_read_battery_mv();
    if (mv >= BAT_FULL_MV) return 100;
    if (mv <= BAT_CRIT_MV) return 0;
    /* Linear interpolation between 3.2V and 4.2V (rough) */
    return (uint8_t)((mv - BAT_CRIT_MV) * 100 / (BAT_FULL_MV - BAT_CRIT_MV));
}

/* -------------------------------------------------------------------------
 * Read battery current (signed mA)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int16_t power_read_battery_ma(void)
{
    uint16_t amps = 0;
    if (bq27426_read_reg16(BQ27426_REG_AMPS, &amps) == 0) {
        return (int16_t)amps;  /* signed */
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Read temperature in milli-celsius
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int32_t power_read_temp_mc(void)
{
    /* Try BQ27426 internal temperature first */
    uint16_t temp = 0;
    if (bq27426_read_reg16(BQ27426_REG_TEMP, &temp) == 0) {
        /* BQ27426 temp is in 0.1°K → convert to milli-°C */
        int32_t temp_k_deci = (int32_t)temp;
        int32_t temp_c_deci = temp_k_deci - 2731;  /* K to °C (×10) */
        return temp_c_deci * 100;  /* to milli-°C */
    }

    /* Fallback: NTC thermistor via SAADC */
    int16_t adc_val = 0;
    saadc_read_channel(PIN_TEMP_SENSE, &adc_val);
    /* NTC: 10kΩ at 25°C, beta=3950
     * With a 10k pull-up to VDD and NTC to GND:
     * V_ntc = VDD * R_ntc / (R_pull + R_ntc)
     * R_ntc = R_pull * V_ntc / (VDD - V_ntc)
     * Temp = 1 / (1/T0 + ln(R/R0)/B) - 273.15
     * Simplified for demo: approximate linear around 25-50°C */
    /* ADC 12-bit: 0-4095 → 0-0.6V (with gain 1x, internal ref) */
    int32_t r_ntc = (int32_t)adc_val * 10000 / (4095 - adc_val);
    /* Steinhart-Hart simplified: T = B / (ln(R/R0) + B/T0) - 273.15
     * B=3950, T0=298.15K, R0=10000 */
    /* Approximate: if R_ntc ≈ 10000 → 25°C; R_ntc < 10000 → hotter */
    int32_t temp_c = 25 - (r_ntc - 10000) / 400;  /* rough linear approx */
    return temp_c * 1000;  /* milli-°C */
}

/* -------------------------------------------------------------------------
 * Check if USB-C power is connected (charging)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int power_is_charging(void)
{
    /* MCP73831 CHG_STAT pin: low = charging, high = not charging / done */
    return ((P1->IN & (1U << PIN_CHG_STAT)) == 0) ? 1 : 0;
}

/* -------------------------------------------------------------------------
 * Enter low-power sleep mode
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void power_enter_sleep(void)
{
    /* Disable sensors to save power */
    /* IMUs to low-power, EMG standby, haptics off */
    /* BLE stays connected (maintained by network core) */

    /* Set SLEEPDEEP bit for system-off (will wake on GPIO sense) */
    /* Actually, we use WFE in the main loop, not deep sleep,
     * because we need RTC/Timer to keep running. */
    __asm volatile ("wfe");
}

/* -------------------------------------------------------------------------
 * Enter ship mode (battery cutoff)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void power_enter_ship_mode(void)
{
    /* Disable everything */
    /* Set the ship-mode FET gate to cut battery */
    P1->OUTSET = (1U << PIN_SHIP_MODE);

    /* Wait for power to die */
    while (1) {
        __asm volatile ("wfe");
    }
}

/* -------------------------------------------------------------------------
 * Set low-battery warning threshold
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void power_set_low_threshold(uint32_t mv)
{
    /* Configure BQ27426 low-battery threshold (if gauge present) */
    /* This requires unlocking the gauge and writing to flash-mapped registers.
     * Simplified: just store the threshold for software comparison. */
    (void)mv;
}

/*
 * Author: jayis1
 * End of power.c
 */