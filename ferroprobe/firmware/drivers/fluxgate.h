/*
 * fluxgate.h — Fluxgate excitation driver interface
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FERROPROBE_FLUXGATE_H
#define FERROPROBE_FLUXGATE_H

#include <stdint.h>

/* Initialize TIM1 for center-aligned PWM at 15.625 kHz.
 * Configures complementary outputs CH1/CH1N and CH2/CH2N for the
 * H-bridge driver that feeds the fluxgate excitation coils.
 * All three axis drive coils are energized in parallel from the
 * same excitation source for phase coherence.
 */
void fluxgate_init(void);

/* Enable excitation output (starts H-bridge switching) */
void fluxgate_enable(void);

/* Disable excitation output (stops H-bridge, tristates outputs) */
void fluxgate_disable(void);

/* Set excitation amplitude as a fraction of full scale (0.0 – 1.0).
 * Lower amplitude reduces power consumption at the cost of sensitivity.
 */
void fluxgate_set_amplitude(float fraction);

/* Get the current excitation frequency in Hz (always 15625) */
uint32_t fluxgate_get_frequency(void);

/* Get the 2f0 reference phase (0-359 degrees) for lock-in sync */
uint16_t fluxgate_get_2f0_phase(void);

/* H-bridge driver enable/disable (TPS28225 gate driver) */
void fluxgate_hbridge_enable(void);
void fluxgate_hbridge_disable(void);

#endif /* FERROPROBE_FLUXGATE_H */