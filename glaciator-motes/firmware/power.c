/*
 * power.c — BQ25895 charger, fuel gauge, solar MPPT, power FSM
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Reads the BQ25895 charger over I2C to monitor solar input, battery
 * voltage, charge current and chip temperature. Implements a state machine
 * that adapts the mote's power profile to environmental conditions:
 *
 *   NORMAL     vbat > 3.2 V, solar OK or battery healthy  → full seismic
 *   DEGRADED   3.0 < vbat < 3.2 V or very cold             → reduced rate
 *   SURVIVAL   vbat < 3.0 V                                → beacon only
 *   CHARGE_ONLY solar good but battery < 3.0 V and very cold → no sampling
 *   FAULT      charger fault / no battery                   → log + retry
 */

#include "power.h"
#include "registers.h"
#include "board.h"
#include "adc_driver.h"

/* ---- I2C helpers -------------------------------------------------------- */
static uint8_t i2c_read_reg(uint8_t dev_addr, uint8_t reg)
{
    (void)dev_addr; (void)reg;
    return 0; /* real build: I2C1 read */
}
static void i2c_write_reg(uint8_t dev_addr, uint8_t reg, uint8_t val)
{
    (void)dev_addr; (void)reg; (void)val;
}

/* ---- State -------------------------------------------------------------- */
static power_state_t g_state = PWR_STATE_NORMAL;
static uint16_t g_vbat_mv = 3300;
static uint16_t g_vsol_mv = 0;
static int16_t  g_temp_c  = 20;
static uint8_t  g_solar_pct = 0;
static bool     g_charging = false;
static bool     g_charge_done = false;
static bool     g_heater_on = false;
static uint32_t g_uptime_s = 0;

/* ---- Charger configuration ---------------------------------------------- */
static void bq25895_configure(void)
{
    /* Input: 4.5 V VINDPM, 2 A IINLIM (solar panel max) */
    i2c_write_reg(BQ25895_I2C_ADDR, BQ25895_REG_INPUT_SRC,
                  BQ25895_VINDPM_4500MV | BQ25895_IINLIM_2000MA);
    /* Charge current: 2 A (LiFePO4 cell can take 3C = ~10 A but panel-limited) */
    i2c_write_reg(BQ25895_I2C_ADDR, BQ25895_REG_ICHG, BQ25895_ICHG_2000MA);
    /* Float voltage: 3.6 V for LiFePO4 */
    i2c_write_reg(BQ25895_I2C_ADDR, BQ25895_REG_VREG, BQ25895_VREG_LIFEPO4_3600);
    /* Enable charge, disable watchdog (we'll kick it manually) */
    i2c_write_reg(BQ25895_I2C_ADDR, BQ25895_REG_CHG_CTRL1, 0x00);
    /* Enable VBUS stats + battery stats in SYS_CTRL */
    i2c_write_reg(BQ25895_I2C_ADDR, BQ25895_REG_SYS_CTRL, 0x40);
}

void power_init(void)
{
    bq25895_configure();
    g_state = PWR_STATE_NORMAL;
    g_uptime_s = 0;
}

/* ---- Sensor reads ------------------------------------------------------- */
void power_update_readings(void)
{
    uint8_t stat;

    /* Battery voltage from ADC (divider 2:1, via ADS1256 AIN5) */
    g_vbat_mv = (uint16_t)adc_read_vbat_mv();

    /* Solar voltage from ADC (divider 3:1, via ADS1256 AIN6) */
    g_vsol_mv = (uint16_t)adc_read_vsol_mv();

    /* Board temperature from ADC (TMP36 on AIN4) */
    g_temp_c = (int16_t)adc_read_board_temp_c();

    /* Charger status over I2C */
    stat = i2c_read_reg(BQ25895_I2C_ADDR, BQ25895_REG_STATUS);
    if (stat & BQ25895_PG_STAT) {
        /* Power good — solar present */
        g_solar_pct = (g_vsol_mv > VSOL_GOOD_MV) ? 100 :
                      (g_vsol_mv * 100 / VSOL_GOOD_MV);
    } else {
        g_solar_pct = 0;
    }
    uint8_t chg_stat = (stat >> 4) & 0x07;
    g_charging     = (chg_stat == 0x01);
    g_charge_done  = (chg_stat == 0x03);

    /* Also read chip temp for fault detection */
    /* (omitted for brevity) */
}

uint16_t power_vbat_mv(void)   { return g_vbat_mv; }
uint16_t power_vsol_mv(void)   { return g_vsol_mv; }
int16_t  power_temp_c(void)    { return g_temp_c; }
uint8_t  power_solar_pct(void) { return g_solar_pct; }
bool     power_charging(void)  { return g_charging; }
bool     power_charge_done(void) { return g_charge_done; }
power_state_t power_state(void) { return g_state; }

void power_set_heater(bool on)
{
    g_heater_on = on;
    /* Drive HEATER_EN_PIN */
}

const char *power_state_name(power_state_t s)
{
    switch (s) {
        case PWR_STATE_NORMAL:      return "NORMAL";
        case PWR_STATE_DEGRADED:    return "DEGRADED";
        case PWR_STATE_SURVIVAL:    return "SURVIVAL";
        case PWR_STATE_CHARGE_ONLY: return "CHARGE_ONLY";
        case PWR_STATE_FAULT:       return "FAULT";
        default: return "?";
    }
}

/* ---- 1 Hz FSM ----------------------------------------------------------- */
void power_tick_1hz(void)
{
    g_uptime_s++;
    power_update_readings();

    /* Heater control: enable only when solar abundant and battery cold */
    if (g_solar_pct > 70 && g_temp_c < (TEMP_COLD_C / 10)) {
        power_set_heater(true);
    } else if (g_temp_c > (TEMP_VERYCOLD_C / 10) + 5) {
        power_set_heater(false);
    }

    /* State transitions */
    power_state_t next = g_state;

    switch (g_state) {
        case PWR_STATE_NORMAL:
            if (g_vbat_mv < VBAT_LOW_MV) {
                next = PWR_STATE_DEGRADED;
            } else if (g_vbat_mv < VBAT_CRIT_MV) {
                next = PWR_STATE_SURVIVAL;
            }
            break;
        case PWR_STATE_DEGRADED:
            if (g_vbat_mv > VBAT_OK_MV) {
                next = PWR_STATE_NORMAL;
            } else if (g_vbat_mv < VBAT_CRIT_MV) {
                next = PWR_STATE_SURVIVAL;
            }
            break;
        case PWR_STATE_SURVIVAL:
            if (g_vbat_mv > VBAT_LOW_MV && g_solar_pct > 20) {
                next = PWR_STATE_DEGRADED;
            }
            if (g_solar_pct > 50 && g_temp_c < (TEMP_VERYCOLD_C / 10)) {
                next = PWR_STATE_CHARGE_ONLY;
            }
            break;
        case PWR_STATE_CHARGE_ONLY:
            if (g_vbat_mv > VBAT_LOW_MV) {
                next = PWR_STATE_DEGRADED;
            }
            break;
        case PWR_STATE_FAULT:
            next = PWR_STATE_NORMAL;   /* auto-retry */
            break;
    }
    g_state = next;
}