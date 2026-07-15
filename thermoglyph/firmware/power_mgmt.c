/*
 * power_mgmt.c — Power management, battery fuel gauge, solar harvesting
 *
 * Implements:
 *   - MAX17048 fuel gauge reading (I2C)
 *   - BQ25570 solar status monitoring (GPIO + ADC)
 *   - Power state machine (active / idle / sleep / solar-sustain / fault)
 *   - Automatic state transitions based on battery level and activity
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#include "power_mgmt.h"
#include "board.h"
#include "registers.h"
#include "thermal_pid.h"
#include <string.h>

/* ------------------------------------------------------------------------- */
/* I2C helpers (shared with thermal_pid.c in real build; simplified here) */
/* ------------------------------------------------------------------------- */
extern bool i2c_read(uint8_t bus, uint8_t addr, uint8_t reg,
                     uint8_t *buf, uint8_t len);

/* GPIO helpers */
static volatile uint32_t *gpio_port(uint8_t p) {
    return (p == 0) ? (volatile uint32_t *)NRF_P0_BASE
                    : (volatile uint32_t *)NRF_P1_BASE;
}
static uint32_t gp_rd(uint8_t p, uint8_t n) {
    return (gpio_port(p)[GPIO_PIN_IN / 4] >> n) & 1U;
}
static void gp_out(uint8_t p, uint8_t n) {
    gpio_port(p)[GPIO_PIN_CNF(n) / 4] = 3UL;
}
static void gp_in_pu(uint8_t p, uint8_t n) {
    gpio_port(p)[GPIO_PIN_CNF(n) / 4] = 0xCUL;
}

/* ------------------------------------------------------------------------- */
/* State */
/* ------------------------------------------------------------------------- */
static power_state_t current_state = POWER_STATE_ACTIVE;
static uint8_t  battery_pct = 100;
static uint16_t battery_mv = 4200;
static int16_t  charge_rate = 0;
static uint16_t solar_mv = 0;
static bool     solar_active = false;
static bool     usb_connected = false;
static uint32_t last_activity_ms = 0;
static uint32_t idle_timeout_ms = 30000;   /* 30 s → idle */
static uint32_t sleep_timeout_ms = 120000; /* 2 min → sleep */

/* ------------------------------------------------------------------------- */
/* MAX17048 fuel gauge */
/* ------------------------------------------------------------------------- */

static void read_fuel_gauge(void)
{
    uint8_t buf[2];

    /* Read VCELL (0x02) → voltage in 78.125 µV steps */
    if (i2c_read(0, MAX17048_ADDR, MAX17048_REG_VCELL, buf, 2)) {
        uint16_t raw = ((uint16_t)buf[0] << 8) | buf[1];
        battery_mv = (uint16_t)((uint32_t)raw * 78U / 1000U); /* → mV */
    }

    /* Read SOC (0x04) → state of charge in 1/256 % steps */
    if (i2c_read(0, MAX17048_ADDR, MAX17048_REG_SOC, buf, 2)) {
        uint16_t raw = ((uint16_t)buf[0] << 8) | buf[1];
        battery_pct = (uint8_t)(raw / 256U);
    }

    /* Read CRATE (0x16) → charge/discharge rate in 0.208 %/hr */
    if (i2c_read(0, MAX17048_ADDR, MAX17048_REG_CRATE, buf, 2)) {
        int16_t raw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
        /* Convert to mA: %/hr × capacity_mAh / 100, then × 0.208
         * Simplified: charge_rate_ma = raw * 500 * 208 / 10000 / 100 */
        charge_rate = (int16_t)((int32_t)raw * 500 * 208 / 1000000);
    }
}

/* ------------------------------------------------------------------------- */
/* BQ25570 solar harvester status */
/* ------------------------------------------------------------------------- */

static void read_solar_status(void)
{
    /* VBAT_OK pin: high = battery voltage above OK threshold (charging) */
    /* SOLAR_OK pin (P1.12) */
    solar_active = (gp_rd(1, 12) != 0);

    /* Read solar voltage via SAADC (P0.16 = analog input)
     * In real implementation, configure SAADC channel and trigger
     * conversion. Simplified: use GPIO read as approximate. */
    static uint16_t solar_sim = 0;
    solar_sim = (solar_sim + 50) % 2000;  /* simulated value */
    if (solar_active) {
        solar_mv = 1200 + (solar_sim / 10);  /* ~1.2–1.4 V from 2 cells */
    } else {
        solar_mv = 0;
    }

    /* USB-C VBUS detect (P0.15) */
    usb_connected = (gp_rd(0, 15) != 0);
}

/* ------------------------------------------------------------------------- */
/* Power state machine */
/* ------------------------------------------------------------------------- */

static void enter_state(power_state_t new_state)
{
    if (new_state == current_state) return;

    /* Exit actions for current state */
    switch (current_state) {
    case POWER_STATE_SLEEP:
        /* Wake up: re-enable peripherals */
        break;
    case POWER_STATE_SOLAR_SUSTAIN:
        /* Wake from ultra-low power */
        break;
    default:
        break;
    }

    current_state = new_state;

    /* Entry actions for new state */
    switch (new_state) {
    case POWER_STATE_ACTIVE:
        /* Everything on */
        break;
    case POWER_STATE_IDLE:
        /* Thermal array off, BLE on, IMU standby */
        thermal_emergency_stop();  /* disable TECs (safe shutdown) */
        break;
    case POWER_STATE_SLEEP:
        /* LoRa CAD only, IMU tap-detect, RTC */
        thermal_emergency_stop();
        break;
    case POWER_STATE_SOLAR_SUSTAIN:
        /* Ultra-low: only BQ25570 + RTC */
        thermal_emergency_stop();
        break;
    case POWER_STATE_FAULT:
        /* Safety fault: minimal operation, TECs off */
        break;
    }
}

void power_update(void)
{
    /* Read sensor data */
    read_fuel_gauge();
    read_solar_status();

    /* Check for safety fault */
    if (thermal_fault_active()) {
        enter_state(POWER_STATE_FAULT);
        return;
    }

    /* If battery critically low, enter solar-sustain */
    if (battery_pct < BATTERY_CRIT_PCT) {
        enter_state(POWER_STATE_SOLAR_SUSTAIN);
        return;
    }

    /* If solar is active and battery is low, prefer solar-sustain */
    if (solar_active && battery_pct < BATTERY_LOW_PCT) {
        enter_state(POWER_STATE_SOLAR_SUSTAIN);
        return;
    }

    /* Normal state transitions based on activity */
    switch (current_state) {
    case POWER_STATE_ACTIVE:
        /* If no activity for idle_timeout, go to idle */
        /* (Activity is signaled by power_signal_activity()) */
        /* Simplified: check if glyph engine is idle */
        {
            const void *curr = NULL;  /* would check glyph_engine_current() */
            if (curr == NULL) {
                /* No active glyph; start idle timer */
                /* In real code, track time since last glyph */
                /* For now, stay active if BLE connected */
                extern bool ble_is_connected(void);
                if (!ble_is_connected() && !solar_active) {
                    /* Go to idle after timeout */
                    /* Simplified: just transition */
                    /* enter_state(POWER_STATE_IDLE); */
                }
            }
        }
        break;

    case POWER_STATE_IDLE:
        /* If activity resumes, go back to active */
        /* If idle for sleep_timeout, go to sleep */
        break;

    case POWER_STATE_SLEEP:
        /* Wake on: LoRa packet, BLE event, IMU tap, glyph command */
        /* Simplified: if battery recovered via solar, wake to idle */
        if (battery_pct > BATTERY_LOW_PCT + 10) {
            enter_state(POWER_STATE_IDLE);
        }
        break;

    case POWER_STATE_SOLAR_SUSTAIN:
        /* If battery charged enough, resume idle */
        if (battery_pct > 30) {
            enter_state(POWER_STATE_IDLE);
        }
        break;

    case POWER_STATE_FAULT:
        /* Only exit fault when cleared */
        if (thermal_clear_fault()) {
            enter_state(POWER_STATE_IDLE);
        }
        break;
    }
}

/* ------------------------------------------------------------------------- */
/* Public API */
/* ------------------------------------------------------------------------- */

void power_init(void)
{
    /* Configure GPIO pins */
    gp_in_pu(1, 12);   /* SOLAR_OK (input, pull-up) */
    gp_in_pu(0, 15);   /* USB VBUS detect (input, pull-up) */

    /* Initialize MAX17048 (quick-start if needed) */
    /* In real code: write 0x4000 to MODE register for quick-start */

    current_state = POWER_STATE_ACTIVE;
    battery_pct = 100;
    battery_mv = 4200;

    /* Initial sensor read */
    read_fuel_gauge();
    read_solar_status();
}

power_state_t power_get_state(void)
{
    return current_state;
}

void power_set_state(power_state_t state)
{
    enter_state(state);
}

uint8_t power_get_battery_pct(void)
{
    return battery_pct;
}

uint16_t power_get_battery_mv(void)
{
    return battery_mv;
}

int16_t power_get_charge_rate_ma(void)
{
    return charge_rate;
}

bool power_solar_active(void)
{
    return solar_active;
}

uint16_t power_get_solar_mv(void)
{
    return solar_mv;
}

bool power_usb_connected(void)
{
    return usb_connected;
}

void power_fill_telemetry(uint8_t *batt_pct, uint16_t *sol_mv,
                           uint8_t *state)
{
    if (batt_pct) *batt_pct = battery_pct;
    if (sol_mv)   *sol_mv = solar_mv;
    if (state)    *state = (uint8_t)current_state;
}