/*
 * actuator_drv.h — TactiScript actuator array driver header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef TACTISCRIPT_ACTUATOR_DRV_H
#define TACTISCRIPT_ACTUATOR_DRV_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize the actuator subsystem:
 *   - Configure SPI0 for DRV2700 chain
 *   - Initialize HV boost converter (disabled initially)
 *   - Set up demux GPIO
 *   - Configure TIMER0 for 200 Hz refresh
 */
void actuator_init(void);

/* Enable/disable the 200 Hz refresh ISR.
 * When enabled, the timer ISR calls actuator_refresh() each tick.
 */
void actuator_refresh_enable(bool enable);

/* Drive one frame (4×6 = 24 elements, values 0-255) to the actuator array.
 * Called from TIMER0 ISR at 200 Hz. Time-multiplexes 24 elements across
 * 4 subframes of 6 elements each via the demux.
 */
void actuator_refresh(const uint8_t *frame);

/* Set the HV drive voltage (0-120 V). Scales the DRV2700 VGAIN register.
 * Called when user adjusts drive intensity.
 */
void actuator_set_voltage(uint32_t millivolts);

/* Emergency shutdown: disable HV boost and all outputs immediately */
void actuator_emergency_off(void);

/* Get current HV rail status (true if HV rail in regulation) */
bool actuator_hv_ok(void);

#endif /* TACTISCRIPT_ACTUATOR_DRV_H */