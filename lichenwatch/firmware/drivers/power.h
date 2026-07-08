/*
 * power.h — Power management interface (battery gauge, sleep, uptime).
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef POWER_H
#define POWER_H

#include <stdint.h>

int      power_init(void);
uint16_t power_read_battery_mv(void);
void     power_update_soc(void);
uint8_t  power_state_of_charge(void);
void     power_enter_stop2(void);
uint32_t power_uptime_s(void);

#endif /* POWER_H */