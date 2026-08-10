/**
 * @file    display.h
 * @brief   TideBand — Sharp LS013B7DH03 transflective LCD driver API.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef TIDEBAND_DISPLAY_H
#define TIDEBAND_DISPLAY_H

#include <stdint.h>
#include "doppler.h"
#include "depth.h"
#include "attitude.h"

/* ---- Display modes ---- */
typedef enum {
    DISP_MODE_SURFACE = 0,   /* Pre/post dive: status, battery, time */
    DISP_MODE_DIVE,          /* Active dive: current rose + depth */
    DISP_MODE_CALIBRATION,   /* Calibration mode */
    DISP_MODE_ERROR,         /* Error display */
} display_mode_t;

/* ---- Public API ---- */

/** Initialize Sharp LCD over SPI4. */
void display_init(void);

/** Clear the display (all white). */
void display_clear(void);

/** Render the current dive screen with live data. */
void display_render_dive(const doppler_result_t *doppler,
                         const depth_data_t *depth,
                         const attitude_t *att,
                         float battery_pct,
                         uint32_t dive_time_s);

/** Render the surface (pre-dive) screen. */
void display_render_surface(float battery_pct, uint16_t dive_count,
                             float surface_temp_c);

/** Render error message. */
void display_render_error(const char *msg);

/** Set display mode. */
void display_set_mode(display_mode_t mode);

/** Turn display on/off. */
void display_on(void);
void display_off(void);

#endif /* TIDEBAND_DISPLAY_H */