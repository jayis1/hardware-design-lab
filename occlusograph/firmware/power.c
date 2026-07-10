/*
 * power.c — Power management for Occlusograph.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 *
 * Manages the nRF5340 power modes, the LTC4124 inductive charge controller,
 * and the battery state-of-charge estimation. The device has three power
 * states:
 *
 *   ACTIVE  — sensors + BLE running, ~2.1 mA. Used during streaming.
 *   IDLE    — sensors off, BLE connected, ~0.4 mA. Quick wake on event.
 *   SLEEP   — deep sleep, only RTC + IMU-interrupt wake, ~12 µA. Overnight
 *             between scheduled bruxism scans.
 *
 * State-of-charge is estimated by coulomb counting (integrating net current
 * in/out of the battery), corrected by a voltage-based open-circuit lookup
 * table every 5 minutes for drift correction.
 */

#include "power.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- Platform shim ---- */
extern void hal_sys_off(void);
extern void hal_sys_wake(void);
extern void hal_rtc_set_alarm(uint32_t seconds);
extern int  hal_twi_write(uint8_t bus, uint8_t addr, const uint8_t *w, uint32_t len);
extern int  hal_twi_read(uint8_t bus, uint8_t addr, uint8_t reg, uint8_t *r, uint32_t len);
extern uint16_t hal_adc_vbat_mv(void);
extern int32_t hal_chip_temp_mc(void);

/* ---- Module state ---- */
typedef enum { PWR_ACTIVE, PWR_IDLE, PWR_SLEEP } pwr_state_t;
static pwr_state_t g_state = PWR_ACTIVE;
static int32_t g_coulomb_count = 0;   /* µAh accumulated */
static uint32_t g_last_ocv_time = 0;
static uint8_t g_soc_pct = 100;

/* ---- Open-circuit voltage → SoC lookup (LiPo, 15 mAh) ---- */
static const uint16_t ocv_table[11][2] = {
    { 2700,  0 }, { 3200,  5 }, { 3500, 15 }, { 3600, 25 }, { 3700, 40 },
    { 3780, 55 }, { 3850, 70 }, { 3950, 85 }, { 4050, 95 }, { 4150, 99 },
    { 4200, 100 }
};

static uint8_t ocv_to_soc(uint16_t mv)
{
    for (int i = 0; i < 10; i++) {
        if (mv < ocv_table[i+1][0]) {
            uint16_t mv0 = ocv_table[i][0], mv1 = ocv_table[i+1][0];
            uint8_t s0 = (uint8_t)ocv_table[i][1], s1 = (uint8_t)ocv_table[i+1][1];
            if (mv1 == mv0) return s0;
            int frac = (int)(mv - mv0) * 100 / (int)(mv1 - mv0);
            return (uint8_t)(s0 + ((s1 - s0) * frac) / 100);
        }
    }
    return 100;
}

/* ---- Public API ---- */

int power_init(void)
{
    /* Enable charge controller: set CHG_ENABLE high to allow inductive
     * charging when a field is present. */
    uint8_t cfg = 0x01;  /* enable charging */
    hal_twi_write(0, LTC4124_ADDR, &cfg, 1);
    g_state = PWR_ACTIVE;
    return 0;
}

uint16_t power_battery_mv(void)
{
    return hal_adc_vbat_mv();
}

uint8_t power_battery_pct(void)
{
    /* Coulomb-count correction: integrate current every call. This is
     * approximate — in the real deployment we'd use the nRF5340's SAADC
     * bias-current measurement. */
    if (g_state == PWR_ACTIVE) {
        g_coulomb_count -= 21;   /* -2.1 mA per 1 s tick = -21 µAh per 10 s */
    } else if (g_state == PWR_IDLE) {
        g_coulomb_count -= 4;
    } else {
        g_coulomb_count -= 0.12;  /* 12 µA */
    }

    /* Periodically correct with OCV. */
    uint32_t now = 0;  /* would be a real-time clock in deployment */
    if (now - g_last_ocv_time >= 300u) {  /* every 5 minutes */
        uint16_t mv = power_battery_mv();
        uint8_t ocv_soc = ocv_to_soc(mv);
        /* Blend: 70% coulomb, 30% OCV to correct drift. */
        int32_t cc_soc = (int32_t)g_soc_pct + (g_coulomb_count * 100) / 15; /* 15 mAh */
        cc_soc = CLAMP(cc_soc, 0, 100);
        g_soc_pct = (uint8_t)((cc_soc * 7 + ocv_soc * 3) / 10);
        g_coulomb_count = 0;
        g_last_ocv_time = now;
    }
    return g_soc_pct;
}

bool power_is_charging(void)
{
    uint8_t st;
    if (hal_twi_read(0, LTC4124_ADDR, LTC_REG_STATUS, &st, 1)) return false;
    return (st & LTC_STATUS_CHARGING) != 0;
}

void power_sleep(void)
{
    g_state = PWR_SLEEP;
    /* Set IMU to wake-on-motion mode (low-power tap detect). */
    uint8_t wake = 0x08;  /* wake-on-motion enable */
    hal_twi_write(1, IMU_I2C_ADDR, &wake, 1);
    hal_sys_off();
}

void power_wake(void)
{
    hal_sys_wake();
    g_state = PWR_ACTIVE;
    /* Restore IMU to full-performance mode. */
    uint8_t on = ICM_VAL_PWR_MGMT0_ON;
    hal_twi_write(1, IMU_I2C_ADDR, &on, 1);
}

void power_set_wake_timer(uint32_t seconds)
{
    hal_rtc_set_alarm(seconds);
}

int32_t power_chip_temp_mc(void)
{
    return hal_chip_temp_mc();
}