/*
 * Pantry Warden shelf driver interface
 * Author: jayis1
 */

#ifndef PANTRY_WARDEN_SHELF_H
#define PANTRY_WARDEN_SHELF_H

#include "../board.h"
#include "gas.h"

typedef struct {
    float baseline_mass_kg;
    float baseline_gap_mm;
    float baseline_freshness_pct;
} pw_shelf_driver_t;

void shelf_init(pw_shelf_driver_t *driver, pw_shelf_frame_t *frame);
void shelf_sample(pw_shelf_driver_t *driver,
                  pw_shelf_frame_t *frame,
                  const pw_gas_frame_t *gas,
                  pw_mode_t mode,
                  unsigned tick);
float shelf_mass_delta(const pw_shelf_driver_t *driver, const pw_shelf_frame_t *frame);

#endif
