/**
 * @file    power.h
 * @brief   Power management interface.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef POWER_H
#define POWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

float  power_get_battery_v(void);
uint8_t power_get_battery_soc(float battery_v);
bool   power_is_solar_charging(void);
float  power_get_solar_power_mw(void);
void   power_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POWER_H */