/*
 * Pantry Warden inference interface
 * Author: jayis1
 */

#ifndef PANTRY_WARDEN_INFERENCE_H
#define PANTRY_WARDEN_INFERENCE_H

#include "../board.h"

void inference_init(pw_inference_t *inf);
void inference_evaluate(pw_inference_t *inf,
                        const pw_gas_frame_t *gas,
                        const pw_shelf_frame_t *shelf,
                        const pw_acoustic_frame_t *acoustic,
                        const pw_power_frame_t *power,
                        float mass_delta_kg,
                        pw_mode_t mode);
const char *inference_state_name(pw_state_t state);

#endif
