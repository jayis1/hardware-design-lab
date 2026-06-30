/*
 * drivers/power.h — Power management driver interface
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef POWER_H
#define POWER_H

#include <stdint.h>
#include "../board.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t battery_mv;
    uint16_t solar_mv;
    uint8_t  charge_pct;       /* 0-100% */
    uint8_t  charging;         /* 1 if solar is charging battery */
    int8_t  temp_c;            /* Battery temperature (°C) */
    power_state_t state;
} power_status_t;

int   power_init(void);
int   power_read_status(power_status_t *out);
int   power_set_state(power_state_t state);
power_state_t power_get_state(void);
uint16_t power_read_battery_mv(void);
uint16_t power_read_solar_mv(void);
uint8_t  power_get_charge_pct(void);
int   power_enter_stop_mode(void);
int   power_wakeup(void);

/* MAX17055 fuel gauge */
int   fuel_gauge_init(void);
int   fuel_gauge_read_soc(uint8_t *percent);
int   fuel_gauge_read_voltage(uint16_t *mv);
int   fuel_gauge_read_temp(int8_t *temp_c);

/* MAX77654 PMIC */
int   pmic_init(void);
int   pmic_set_charge_current(uint32_t ma);
int   pmic_get_charge_status(uint8_t *charging);

#ifdef __cplusplus
}
#endif

#endif /* POWER_H */