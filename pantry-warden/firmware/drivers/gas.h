/*
 * Pantry Warden gas driver interface
 * Author: jayis1
 */

#ifndef PANTRY_WARDEN_GAS_H
#define PANTRY_WARDEN_GAS_H

#include "../board.h"

typedef struct {
    float baseline_temp_c;
    float baseline_humidity_pct;
    float baseline_co2_ppm;
    float baseline_voc_index;
    float fan_bias;
} pw_gas_driver_t;

void gas_init(pw_gas_driver_t *driver, pw_gas_frame_t *frame);
void gas_sample(pw_gas_driver_t *driver,
                pw_gas_frame_t *frame,
                pw_mode_t mode,
                unsigned tick);
float gas_spoilage_lift(const pw_gas_frame_t *frame);

#endif
