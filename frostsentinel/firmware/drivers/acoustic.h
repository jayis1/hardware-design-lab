/*
 * drivers/acoustic.h — Acoustic emission ice-nucleation detection header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef FROSTSENTINEL_ACOUSTIC_H
#define FROSTSENTINEL_ACOUSTIC_H

#include <stdint.h>

/* AE status codes (also stored in g_sys.ae_status) */
#define AE_STATUS_IDLE        0
#define AE_STATUS_ARMED       1
#define AE_STATUS_NUCLEATION  2

/* Initialize the ADC and state for the acoustic emission channel. */
void acoustic_init(void);

/*
 * Capture one 40 ms AE burst, run FFT, update baseline, detect events.
 * Called only when leaf wetness > threshold and T_wet ≤ +1 °C.
 * Returns AE_STATUS_IDLE, AE_STATUS_ARMED, or AE_STATUS_NUCLEATION.
 */
uint8_t acoustic_check(void);

/* Reset nucleation state (called at the start of each new frost night). */
void acoustic_reset(void);

/* Accessors */
uint32_t acoustic_get_cumulative_energy(void);
uint32_t acoustic_get_last_band_energy(void);
uint8_t  acoustic_is_nucleation_detected(void);
uint32_t acoustic_get_nucleation_time(void);

#endif /* FROSTSENTINEL_ACOUSTIC_H */