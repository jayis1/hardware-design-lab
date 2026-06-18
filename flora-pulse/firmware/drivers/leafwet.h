/*
 * leafwet.h — Leaf-wetness capacitive sensor driver interface
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FLORAPULSE_LEAFWET_H
#define FLORAPULSE_LEAFWET_H

#include <stdint.h>

/* Initialize TIM3 in external clock mode 1 (input capture counter).
 * The 555 oscillator on the leaf-wetness probe feeds TIM3_ETR (PD2).
 */
void leafwet_init(void);

/* Measure leaf-wetness oscillator frequency.
 * Gates the counter for 100 ms, returns frequency in Hz.
 * Higher frequency = dry leaf, lower = wet (capacitance increases).
 */
uint16_t leafwet_measure_hz(void);

/* Convert frequency to wetness percentage (0 = bone dry, 100 = saturated).
 * Uses linear interpolation between dry (10 kHz) and wet (5 kHz).
 */
uint8_t leafwet_to_pct(uint16_t freq_hz);

/* Get last measurement (cached from leafwet_measure_hz) */
uint16_t leafwet_get_last_hz(void);

#endif /* FLORAPULSE_LEAFWET_H */