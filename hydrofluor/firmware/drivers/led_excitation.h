/*
 * led_excitation.h — HydroFluor excitation LED driver
 * Author: jayis1
 * License: MIT
 *
 * TIM1 PWM-based constant-current pulse driver for the 6 excitation LEDs.
 * Each excitation cycle: select wavelength → pulse for N µs → trigger ADC
 * synchronous sample → off → dark sample.
 */
#ifndef LED_EXCITATION_H
#define LED_EXCITATION_H

#include "board.h"

/* Initialize TIM1 PWM for the six excitation channels. */
void led_excitation_init(void);

/* Configure a single excitation channel's pulse/current/averages. */
void led_excitation_config(ex_wavelength_t wl, uint16_t pulse_us,
                           uint16_t current_ma, uint8_t averages);

/* Get the current config for a channel. */
const led_config_t *led_excitation_get(ex_wavelength_t wl);

/* Fire a single excitation pulse on the given wavelength and return the
 * timestamp (in µs since boot) at which the pulse midpoint occurred — used by
 * the photodiode driver to synchronously sample the detector. */
uint32_t led_excitation_fire(ex_wavelength_t wl);

/* Arm synchronous ADC trigger: configures TIM1 CC2 to assert DRDY-window
 * exactly at the pulse midpoint. Returns the expected sample-ready delay. */
uint32_t led_excitation_arm_sync(ex_wavelength_t wl);

/* Convenience: enumerate the wavelength table (for UI). */
const char *led_excitation_name(ex_wavelength_t wl);
uint16_t    led_excitation_wavelength_nm(ex_wavelength_t wl);

#endif /* LED_EXCITATION_H */