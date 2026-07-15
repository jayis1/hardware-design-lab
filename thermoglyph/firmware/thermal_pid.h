/*
 * thermal_pid.h — Thermal PID control loop and safety system
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#ifndef THERMAL_PID_H
#define THERMAL_PID_H

#include <stdint.h>
#include <stdbool.h>
#include "glyph_engine.h"

/* Initialize thermal PID system, sensors, PWM drivers */
void thermal_init(void);

/* Run one PID iteration (called at 200 Hz from thermal_task).
 * Reads skin temps from TMP117, computes PID, drives TLC59711 + row select.
 * Returns false if a safety fault is active (THERM_FAULT asserted). */
bool thermal_pid_step(const thermal_frame_t *target);

/* Read current skin temperatures (all 12 rows) in °C × 100 */
void thermal_read_skin_temps(int16_t temps[12]);

/* Get average and max skin temperature (for telemetry) */
void thermal_get_stats(int16_t *avg_c100, int16_t *max_c100);

/* Check if hardware fault is active */
bool thermal_fault_active(void);

/* Clear fault (requires fault condition to be gone) */
bool thermal_clear_fault(void);

/* Emergency stop: immediately disable all TEC power */
void thermal_emergency_stop(void);

/* Kick the hardware watchdog (must be called every 50 ms) */
void thermal_watchdog_kick(void);

#endif /* THERMAL_PID_H */