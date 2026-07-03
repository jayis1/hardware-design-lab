/*
 * disdrometer.h — DSD computation, derived products, calibration
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef RAINFORGE_DISDROMETER_H
#define RAINFORGE_DISDROMETER_H

#include <stdint.h>
#include "piezo.h"

/* ---- DSD histogram (9 diameter bins) ---- */
typedef struct {
    uint32_t count[DSD_NUM_BINS];   /* droplet count per bin this interval */
    float    n_per_m3[DSD_NUM_BINS]; /* number concentration N(D) */
    float    rainfall_rate;          /* R, mm/hr */
    float    reflectivity;           /* Z, mm^6/m^3 */
    float    liquid_water_content;   /* LWC, g/m^3 */
    float    zr_a, zr_b;             /* Z = a * R^b fit */
    float    mean_diameter;          /* D_m, mm (volume-weighted mean) */
    float    total_droplets;         /* sum of all bins */
    float    median_diameter;        /* D0, mm (half of LWC is below D0) */
    int16_t  avg_charge_pc;          /* average droplet charge (pC) */
    uint16_t pos_count;              /* count of positively-charged droplets */
    uint16_t neg_count;              /* count of negatively-charged droplets */
} dsd_t;

/* ---- API ---- */
void  disdrometer_init(dsd_t *dsd);
void  disdrometer_add(dsd_t *dsd, const droplet_feature_t *feat);
void  disdrometer_compute(dsd_t *dsd, float sample_area_m2, float interval_s);
void  disdrometer_reset(dsd_t *dsd);
int   disdrometer_bin_index(float diameter_mm);
float terminal_velocity(float diameter_mm);  /* Atlas formula */

/* ---- Default calibration (rough — replaced by lab calibration) ---- */
const calibration_t *disdrometer_default_cal(void);

#endif /* RAINFORGE_DISDROMETER_H */