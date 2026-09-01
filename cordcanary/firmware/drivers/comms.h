/*
 * CordCanary telemetry formatter
 * Author: jayis1
 */

#ifndef CORDCANARY_COMMS_H
#define CORDCANARY_COMMS_H

#include "../board.h"
#include "../registers.h"

void comms_format_frame(char *buffer,
                        size_t buffer_size,
                        cc_mode_t mode,
                        const cc_thermal_frame_t *thermal,
                        const cc_current_frame_t *current,
                        const cc_strain_frame_t *strain,
                        const cc_motion_frame_t *motion,
                        const cc_power_frame_t *power,
                        const cc_inference_t *inf);

void comms_format_registers(char *buffer, size_t buffer_size, const cc_register_bank_t *bank);

#endif
