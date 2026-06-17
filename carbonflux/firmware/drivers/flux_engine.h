/**
 * @file    flux_engine.h
 * @brief   Flux engine — linear regression CO₂ flux computation.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef FLUX_ENGINE_H
#define FLUX_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** Binary log record — 32 bytes (fits LoRaWAN payload). */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;          /* 0  — Unix UTC (seconds)            */
    int32_t  co2_ppm_x1000;     /* 4  — CO₂ ppm × 1000                */
    int32_t  flux_umol_x1000;   /* 8  — Flux µmol·m⁻²·s⁻¹ × 1000     */
    int16_t  soil_temp_5cm_x100;  /* 12 — °C × 100 @ 5 cm             */
    int16_t  soil_temp_15cm_x100; /* 14 — °C × 100 @ 15 cm            */
    int16_t  soil_temp_30cm_x100; /* 16 — °C × 100 @ 30 cm            */
    uint16_t vwc_x100;          /* 18 — VWC % × 100                   */
    uint16_t pressure_x10;      /* 20 — hPa × 10                      */
    int16_t  air_temp_c_x100;   /* 22 — °C × 100                      */
    uint16_t par_umol;          /* 24 — PAR µmol·m⁻²·s⁻¹             */
    uint8_t  battery_soc;       /* 26 — Battery SOC %                 */
    uint8_t  flags;             /* 27 — Flags                          */
    uint32_t reserved;          /* 28 — Reserved                       */
} flux_record_t;

/* Flag bits */
#define FLUX_FLAG_VALID         0x01
#define FLUX_FLAG_ERROR         0x02
#define FLUX_FLAG_CALIBRATION   0x04
#define FLUX_FLAG_RAIN          0x08
#define FLUX_FLAG_LORA_FAILED   0x10

/** Running sums for linear regression. */
typedef struct {
    uint32_t sample_count;      /* Number of valid samples             */
    float    sum_x;             /* Σ time (seconds)                    */
    float    sum_y;             /* Σ CO₂ (ppm)                         */
    float    sum_xy;            /* Σ (time × CO₂)                      */
    float    sum_xx;            /* Σ (time²)                           */
    float    intercept;         /* CO₂ at t=0 (ppm)                    */
    float    slope;             /* dC/dt (ppm/s)                       */
    float    r_squared;         /* Coefficient of determination (0–1)  */
    float    flux_umol;         /* Soil CO₂ efflux (µmol·m⁻²·s⁻¹)     */
} flux_result_t;

/* Lifecycle */
void     flux_engine_reset(void);
void     flux_engine_add_sample(float time_sec, float co2_ppm,
                                float pressure, float temp_c);
void     flux_engine_compute(flux_result_t *result);

/* Status */
uint32_t flux_engine_get_count(void);
bool     flux_engine_is_active(void);
uint8_t  flux_engine_quality_score(void);

#ifdef __cplusplus
}
#endif

#endif /* FLUX_ENGINE_H */