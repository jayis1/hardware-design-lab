/*
 * sapflow.h — Sap-flow heat-pulse measurement driver interface
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FLORAPULSE_SAPFLOW_H
#define FLORAPULSE_SAPFLOW_H

#include <stdint.h>

/* Initialize the sap-flow subsystem:
 *  - TIM1 PWM for heater control (one-shot mode)
 *  - ADS1220 ADC for thermistor measurement via SPI2
 */
void sapflow_init(void);

/* Fire a single heat pulse (4-second heater on), then sample
 * thermistors at 10 Hz for 60 seconds post-pulse.
 * Stores temperature curves in internal buffer.
 * Blocking call — returns when measurement cycle complete (~64 s).
 */
void sapflow_measure(void);

/* Get the computed heat pulse velocity (cm/h).
 * Returns 0 if no measurement has been completed.
 * Uses the T-max method: vmax = sqrt(k / (pi * tmax)) where
 * tmax is the time for the downstream sensor to reach max temperature.
 */
float sapflow_get_velocity(void);

/* Get temperature readings from the last measurement cycle.
 * upper: thermistor above heater (downstream in sap flow)
 * lower: thermistor below heater (upstream, reference)
 */
void sapflow_get_temps(float *upper, float *lower);

/* Get the baseline temperature (pre-pulse) */
float sapflow_get_baseline_temp(void);

/* Get the peak temperature rise (°C) at the downstream sensor */
float sapflow_get_delta_t(void);

/* Get the time-to-peak (seconds) from heat pulse to max temperature */
float sapflow_get_tmax(void);

/* Check if a measurement cycle is currently running */
uint8_t sapflow_is_measuring(void);

/* Trigger heater pulse from main loop (non-blocking version).
 * Measurement results available after cycle completes.
 */
void sapflow_trigger_async(void);

/* Poll function for async measurement — call from main loop.
 * Returns 1 when measurement complete, 0 otherwise.
 */
uint8_t sapflow_poll(void);

#endif /* FLORAPULSE_SAPFLOW_H */