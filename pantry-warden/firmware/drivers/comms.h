/*
 * Pantry Warden comms formatter interface
 * Author: jayis1
 */

#ifndef PANTRY_WARDEN_COMMS_H
#define PANTRY_WARDEN_COMMS_H

#include <stddef.h>

#include "../board.h"

void comms_format_frame(char *buffer,
                        size_t buffer_size,
                        unsigned tick,
                        const pw_gas_frame_t *gas,
                        const pw_shelf_frame_t *shelf,
                        const pw_acoustic_frame_t *acoustic,
                        const pw_power_frame_t *power,
                        const pw_inference_t *inf,
                        pw_mode_t mode);

#endif
