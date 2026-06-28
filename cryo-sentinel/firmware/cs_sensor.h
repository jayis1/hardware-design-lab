/*
 * cs_sensor.h — Sensor driver interface for Cryo-Sentinel.
 *
 * Author: jayis1
 * License: MIT
 */
#ifndef CRYO_SENTINEL_CS_SENSOR_H
#define CRYO_SENTINEL_CS_SENSOR_H

#include <stdint.h>
#include <stdbool.h>

/* Raw and cooked sensor readings for one tick. */
typedef struct {
    /* Capacitive LN2 level (raw FDC code, then calibrated %). */
    uint32_t level_raw;
    float    level_pct;
    float    level_rate_ph;       /* %/hour, from rolling window */

    /* Multi-zone RTD temperatures in degC (top/mid/bottom of vapor column). */
    float    rtd_degC[3];
    float    gradient_slope;      /* degC/mm proxy from RTD deltas */

    /* IMU: tilt in deg from vertical, vibration RMS in g. */
    float    tilt_deg;
    float    vib_rms_g;

    /* Ambient T/RH/P. */
    float    ambient_degC;
    float    ambient_rh;
    float    ambient_hpa;

    /* GPIO states. */
    bool     lid_open;
    bool     enclosure_open;
    bool     mains_present;
    bool     probe_present;

    /* Per-sensor validity flags. */
    bool     level_ok;
    bool     rtd_ok[3];
    bool     imu_ok;
    bool     ambient_ok;
} cs_reading_t;

/* Initialize all sensors. Returns 0 on success, nonzero bitmask of failures. */
int cs_sensor_init(void);

/* Perform one full sensor sweep (≈ 4 ms). Fills *out. */
int cs_sensor_sample(cs_reading_t *out);

/* Median filter helper for the level raw reading. */
uint32_t cs_median_u32(uint32_t *buf, int n);

/* Median filter helper for the RTD raw code. */
uint16_t cs_median_u16(uint16_t *buf, int n);

/* Compute RTD temperature (degC) from a MAX31865 15-bit code. */
float cs_rtd_code_to_degC(uint16_t code);

/* Convert FDC2214 raw code to capacitance (pF) at the configured fDiv. */
float cs_fdc_raw_to_pf(uint32_t raw);

#endif /* CRYO_SENTINEL_CS_SENSOR_H */