/*
 * drivers/optical.h — 8-wavelength LED sweep + photodiode absorbance
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef HYDRASCAN_OPTICAL_H
#define HYDRASCAN_OPTICAL_H

#include "../board.h"

/* Absorbance vector out[8] = -log10(I_sample/I_ref) per wavelength. */
hydra_err_t optical_init(void);
hydra_err_t optical_sweep(float out[OPTICAL_WAVELENGHS]);
void        optical_off(void);   /* de-energise all LEDs */

/* Wavelegth in nm of each slot, for labelling in the app. */
extern const uint16_t optical_wavelength_nm[OPTICAL_WAVELENGHS];

#endif