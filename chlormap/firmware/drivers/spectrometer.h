/*
 * spectrometer.h — 128-element spectral processing + 16-band binning
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef DRIVERS_SPECTROMETER_H
#define DRIVERS_SPECTROMETER_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* Spectrometer result after processing */
typedef struct {
    int16_t bands_x1000[NUM_BANDS];   /* 16-band reflectance × 1000 */
    int16_t raw_bands[NUM_BANDS];      /* raw dark-corrected values */
    int32_t elements[ARRAY_ELEMENTS];  /* dark-corrected raw elements */
} spectrometer_result_t;

/* Wavelength table for the 16 bands (nm) */
extern const uint16_t band_wavelengths[NUM_BANDS];

/* Element-to-wavelength mapping (128 elements → nm) */
extern const uint16_t element_wavelengths[ARRAY_ELEMENTS];

/* Initialize spectrometer (load calibration) */
bool spectrometer_init(void);

/* Process raw frame: dark subtraction, linearity correction, wavelength
 * mapping, Gaussian binning into 16 bands, reflectance calculation.
 * raw:    128-element raw sample (white LED or merged)
 * dark:   128-element dark frame
 * result: output processed spectrum
 */
bool spectrometer_process(const int32_t *raw, const int32_t *dark,
                          spectrometer_result_t *result);

/* Get wavelength for a band index */
uint16_t spectrometer_band_wavelength(uint8_t band_idx);

#endif /* DRIVERS_SPECTROMETER_H */