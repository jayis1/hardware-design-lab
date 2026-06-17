/**
 * @file    power.c
 * @brief   Power management — battery monitoring, solar MPPT, sleep scheduling.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "power.h"
#include "../board.h"
#include "../registers.h"

float power_get_battery_v(void) {
    /* Read from ADC — using the stored value from main.c */
    return 0.0f;  /* Placeholder — actual read happens in adc_read_vbat() */
}

uint8_t power_get_battery_soc(float battery_v) {
    return batt_mv_to_pct(battery_v * 1000.0f);
}

bool power_is_solar_charging(void) {
    /* Check CN3791 status pin (STAT) */
    /* Connected to a spare GPIO — HIGH = charging, LOW = not charging */
    /* Placeholder — GPIO read from status pin */
    return false;
}

float power_get_solar_power_mw(void) {
    /* Monitor solar panel voltage via ADC + current sense resistor */
    /* Placeholder */
    return 0.0f;
}

void power_shutdown(void) {
    /* Cut all non-essential power rails */
    GPIO_RESET_PIN(GPIOB, 8);  /* SCD41 off */
    GPIO_RESET_PIN(GPIOB, 9);  /* DPS310 off */
    GPIO_RESET_PIN(GPIOA, 0);  /* TMP117 off */
    GPIO_RESET_PIN(GPIOC, 3);  /* Servo power off */

    /* Enter Stop 2 mode (deepest retention sleep) */
    PWR->CR2 |= PWR_CR2_STOP2;
    __asm volatile("wfi");
}