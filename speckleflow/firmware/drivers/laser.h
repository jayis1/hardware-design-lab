/*
 * laser.h — 785 nm VCSEL driver with TEC stabilization
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef SPECKLEFLOW_LASER_H
#define SPECKLEFLOW_LASER_H

#include <stdint.h>

/**
 * Initialize the laser subsystem (DAC, TEC PWM, PID state).
 * @return 0 on success
 */
int laser_init(void);

/**
 * Enable or disable the laser with IEC 60825-1 safety checks.
 * @param on  1=enable (with 5 s ramp), 0=disable (immediate)
 * @return 0 on success, -1 if key switch off, -2 if interlock open
 */
int laser_enable(uint8_t on);

/**
 * Set target laser power as percentage of maximum (0–100).
 * @param pct  Power percentage
 */
void laser_set_power(uint8_t pct);

/**
 * Register a trigger event (resets auto-shutoff timer).
 */
void laser_trigger(void);

/**
 * 1 ms tick handler — call from main tick.
 * Handles soft-start ramp, TEC PID, and safety checks.
 */
void laser_tick(void);

/**
 * Read the laser current-sense ADC value.
 */
uint16_t laser_get_current_sense(void);

/**
 * Read the TEC thermistor ADC value.
 */
uint16_t laser_get_thermistor(void);

/**
 * Check if laser is enabled.
 */
int laser_is_enabled(void);

/**
 * Check if laser is in soft-start ramp.
 */
int laser_is_ramping(void);

/**
 * Set the TEC temperature setpoint (raw ADC value).
 */
void laser_set_tec_setpoint(uint16_t raw);

#endif /* SPECKLEFLOW_LASER_H */