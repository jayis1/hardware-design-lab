/*
 * piezo.h — ADS131M04 ADC driver & droplet event extractor
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef RAINFORGE_PIEZO_H
#define RAINFORGE_PIEZO_H

#include <stdint.h>
#include "board.h"

/* ---- Raw droplet event from a single impact ---- */
typedef struct {
    int32_t peak_uv[ADC_CHANNELS];     /* peak amplitude per channel, µV */
    int32_t energy_uv2us[ADC_CHANNELS];/* integrated V²·dt, (µV)²·µs */
    uint16_t width_us;                  /* pulse full-width half-max */
    int16_t  asymmetry_q15;             /* +peak/-peak ratio in Q0.15 */
    uint16_t decay_tau_us;              /* ringing decay constant */
    uint16_t flags;                     /* bit0: charge positive, bit1: saturated */
    uint32_t timestamp_ms;              /* g_board.uptime_ms at event */
} droplet_raw_t;

/* ---- Extracted physical droplet features ---- */
typedef struct {
    float diameter_mm;
    float velocity_ms;
    float charge_pc;
    float kinetic_energy_uj;
    uint32_t timestamp_ms;
} droplet_feature_t;

/* ---- Calibration coefficients (stored in FRAM) ---- */
typedef struct {
    float diameter_a;      /* D = a * peak + b * width + c * decay + d */
    float diameter_b;
    float diameter_c;
    float diameter_d;
    float velocity_a;      /* v = a / width + b */
    float velocity_b;
    float charge_a;        /* q = a * asymmetry + b */
    float charge_b;
    float energy_scale;    /* E_kinetic = scale * sum(energy_uv2us) */
    uint32_t magic;        /* 0x52434C42 = "RCLB" → calibration valid */
} calibration_t;

#define CALIB_MAGIC 0x52434C42U

/* ---- API ---- */
void     piezo_init(void);
void     piezo_power_on(void);
void     piezo_power_off(void);
uint8_t  piezo_capture(droplet_raw_t *out);   /* 1 if event captured, 0 if none */
void     piezo_extract(const droplet_raw_t *raw, droplet_feature_t *feat,
                       const calibration_t *cal);
uint8_t  piezo_is_busy(void);

/* Direct ADC access for diagnostics */
int      piezo_read_adc_frame(int32_t samples[ADC_CHANNELS]);
void     piezo_reset_adc(void);

#endif /* RAINFORGE_PIEZO_H */