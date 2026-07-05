/*
 * tdc.h — Time-to-digital conversion helpers
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_TDC_H
#define MUONSCAPE_DRV_TDC_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Convert a TDC bin value to nanoseconds using the calibrated bin width. */
float tdc_bin_to_ns(uint16_t bin);

/* Calibrate the TDC bin width by measuring a known delay (e.g. from
 * the GPS 1 PPS divided down). Stores the result internally. */
muon_status_t tdc_calibrate(uint32_t reference_ns);

/* Get the current calibrated bin width in picoseconds. */
uint32_t tdc_get_bin_ps(void);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_TDC_H */