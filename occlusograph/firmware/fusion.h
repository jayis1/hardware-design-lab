/*
 * fusion.h — Force-fusion interface for Occlusograph.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef OCCLUSOGRAPH_FUSION_H
#define OCCLUSOGRAPH_FUSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "sensors.h"

/* Initialize the fusion filter state (zeros all per-element state). */
void fusion_init(void);

/* Process one raw sensor frame into a calibrated force frame.
 * Combines the high-bandwidth piezo signal with the low-bandwidth
 * capacitive signal via a per-element complementary filter, producing
 * a full-bandwidth (DC-2 kHz) force estimate in millinewtons.
 */
void fusion_update(const sensor_frame_t *raw, force_frame_t *out);

/* Configure filter cutoffs (Hz). Defaults: piezo HP 0.1 Hz, cap LP 50 Hz. */
void fusion_set_cutoffs(float piezo_hp_hz, float cap_lp_hz);

/* Apply per-element force calibration gains (scaling from raw to mN). */
void fusion_set_gains(const float piezo_gain[PIEZO_NUM_ELEMENTS],
                      const float cap_gain[CAP_NUM_ELEMENTS]);

#ifdef __cplusplus
}
#endif

#endif /* OCCLUSOGRAPH_FUSION_H */