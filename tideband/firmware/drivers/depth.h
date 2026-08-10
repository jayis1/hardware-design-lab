/**
 * @file    depth.h
 * @brief   TideBand — Depth/pressure/temperature sensor driver API.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef TIDEBAND_DEPTH_H
#define TIDEBAND_DEPTH_H

#include <stdint.h>

/* ---- Measurement result ---- */
typedef struct {
    float depth_m;      /* Depth in meters (from pressure) */
    float pressure_mbar; /* Raw pressure in millibars */
    float temp_c;       /* Water temperature in °C */
    uint8_t valid;      /* 1 if data is valid */
} depth_data_t;

/* ---- Public API ---- */

/** Initialize MS5837-30BA pressure sensor over I2C1. */
void depth_init(void);

/** Read pressure and temperature, compute depth.
 *  The MS5837 takes ~10 ms for high-resolution conversion.
 *  This function blocks until data is ready. */
void depth_read(depth_data_t *data);

/** Set the surface pressure reference for depth calculation.
 *  Should be called at the start of each dive (depth = 0). */
void depth_set_surface(float pressure_mbar);

/** Quick check: is the device currently immersed (depth > threshold)? */
uint8_t depth_is_immersed(void);

#endif /* TIDEBAND_DEPTH_H */