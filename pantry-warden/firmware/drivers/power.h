/*
 * Pantry Warden power driver interface
 * Author: jayis1
 */

#ifndef PANTRY_WARDEN_POWER_H
#define PANTRY_WARDEN_POWER_H

#include "../board.h"
#include "gas.h"

typedef struct {
    float battery_capacity_mah;
    float nominal_voltage_v;
} pw_power_driver_t;

void power_init(pw_power_driver_t *driver, pw_power_frame_t *frame);
void power_step(pw_power_driver_t *driver,
                pw_power_frame_t *frame,
                const pw_gas_frame_t *gas,
                pw_mode_t mode,
                unsigned tick);

#endif
