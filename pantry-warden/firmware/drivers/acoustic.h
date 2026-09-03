/*
 * Pantry Warden acoustic driver interface
 * Author: jayis1
 */

#ifndef PANTRY_WARDEN_ACOUSTIC_H
#define PANTRY_WARDEN_ACOUSTIC_H

#include "../board.h"
#include "shelf.h"

typedef struct {
    float baseline_airborne;
    float baseline_structure;
} pw_acoustic_driver_t;

void acoustic_init(pw_acoustic_driver_t *driver, pw_acoustic_frame_t *frame);
void acoustic_sample(pw_acoustic_driver_t *driver,
                     pw_acoustic_frame_t *frame,
                     const pw_shelf_frame_t *shelf,
                     pw_mode_t mode,
                     unsigned tick);

#endif
