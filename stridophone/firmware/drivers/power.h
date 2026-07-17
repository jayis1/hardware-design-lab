/*
 * power.h — BQ25895 charger / fuel gauge + power management.
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_POWER_H
#define STRIDOPHONE_POWER_H

#include <stdint.h>

void power_init(void);

/* Read battery state-of-charge (0..100) and voltage (mV). */
int  power_soc(void);
int  power_voltage_mv(void);

/* True if the charger is actively charging. */
uint8_t power_is_charging(void);

/* JEITA gating: disable charging if temp out of [0,45] C. */
void power_jeita_check(float temp_c);

#endif