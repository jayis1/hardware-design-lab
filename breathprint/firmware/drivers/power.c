/*
 * power.c — Battery monitoring, sleep modes, and charger control
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#include "power.h"
#include "adc_drv.h"
#include "../board.h"

/* ========================================================================
 * Power state
 * ======================================================================== */

static uint8_t last_battery_pct = 100;
static uint8_t last_charging = 0;
static float last_battery_voltage = 4.2f;

/* ========================================================================
 * Initialize power management
 * ========================================================================
 */

void power_init(void) {
    /* Enable PWR clock already done in board_init */

    /* Configure PWR: VOS0 (high performance) for 312 MHz */
    PWR_CR3 |= PWR_CR3_SDEN;  /* Enable SD (supply detector) */

    /* Initial battery reading */
    last_battery_pct = adc_read_battery_pct();
    last_battery_voltage = adc_read_battery_voltage();
    last_charging = power_is_charging();
}

/* ========================================================================
 * Read battery percentage
 * ======================================================================== */

uint8_t power_read_battery(void) {
    last_battery_pct = adc_read_battery_pct();
    last_battery_voltage = adc_read_battery_voltage();
    return last_battery_pct;
}

/* ========================================================================
 * Check if charging (MCP73831 STAT pin)
 *   STAT = LOW → charging
 *   STAT = HIGH → charge complete / no battery
 * ========================================================================
 */

uint8_t power_is_charging(void) {
    /* STAT pin is active-low during charging */
    uint8_t stat = gpio_read(CHRG_STAT_PORT, CHRG_STAT_PIN);
    last_charging = !stat;  /* Low = charging */
    return last_charging;
}

/* ========================================================================
 * Get battery voltage
 * ======================================================================== */

float power_get_battery_voltage(void) {
    return last_battery_voltage;
}

/* ========================================================================
 * Check if battery is critically low
 * ======================================================================== */

uint8_t power_is_critical(void) {
    return (last_battery_pct < 5) ? 1 : 0;
}

/* ========================================================================
 * Enter low-power sleep mode
 * ========================================================================
 */

void power_enter_sleep(void) {
    /* Disable peripherals that aren't needed during sleep */
    sensor_array_heaters_off();
    gpio_clear(SENS_PWR_PORT, SENS_PWR_PIN);
    display_off();

    /* Configure SysTick to wake up periodically */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
    __asm volatile ("dsb");
    __asm volatile ("wfi");
}

/* ========================================================================
 * Enter stop mode (deepest low-power with RAM retention)
 * ========================================================================
 */

void power_enter_stop(void) {
    /* Turn off all non-essential peripherals */
    sensor_array_heaters_off();
    gpio_clear(SENS_PWR_PORT, SENS_PWR_PIN);
    display_off();
    gpio_clear(PUMP_PORT, PUMP_PWR_PIN);

    /* Enter Stop mode */
    PWR_CR1 |= PWR_CR1_LPDS;  /* Low-power deep sleep */
    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    __asm volatile ("dsb");
    __asm volatile ("wfi");

    /* Woken up — reconfigure system */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
    clock_init();
    display_on();
}

/* ========================================================================
 * Enter standby mode (lowest power, RAM lost)
 * ========================================================================
 */

void power_enter_standby(void) {
    /* This would use PWR_CR1_PDDS + SLEEPDEEP for lowest power mode */
    /* For now, just use stop mode */
    power_enter_stop();
}

/* ========================================================================
 * Power budget estimation
 * Returns estimated remaining time in minutes
 * ========================================================================
 */

uint32_t power_estimate_remaining_min(void) {
    /* Based on current consumption patterns:
     * - Idle with BLE: ~8 mA
     * - 500 mAh battery
     * - Remaining = battery_pct/100 * 500 / 8 * 60
     */
    float mAh_remaining = (float)last_battery_pct / 100.0f * 500.0f;
    float hours = mAh_remaining / 8.0f;
    return (uint32_t)(hours * 60.0f);
}

/* ========================================================================
 * Estimate samples remaining
 * ======================================================================== */

uint32_t power_estimate_samples_remaining(void) {
    /* Each sample uses ~0.94 mWh
     * Battery: 500 mAh × 3.7V = 1850 mWh
     * Remaining energy = battery_pct/100 * 1850
     * Samples = remaining / 0.94
     */
    float remaining_mwh = (float)last_battery_pct / 100.0f * 1850.0f;
    return (uint32_t)(remaining_mwh / 0.94f);
}