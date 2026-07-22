/*
 * survey.h — Survey state machine + calibration data interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef REBARSCOPE_SURVEY_H
#define REBARSCOPE_SURVEY_H

#include <stdint.h>

/* ---- ASTM C876 classification ---- */
#define HCP_CLASS_NO_CORROSION   0
#define HCP_CLASS_UNCERTAIN       1
#define HCP_CLASS_ACTIVE_CORROSION 2

/* ---- Resistivity classification ---- */
#define RESISTIVITY_LOW        0   /* < 50 kΩ·cm */
#define RESISTIVITY_MODERATE   1   /* 50–100   */
#define RESISTIVITY_HIGH       2   /* 100–200  */
#define RESISTIVITY_VERY_HIGH  3   /* ≥ 200    */

typedef struct {
    uint16_t grid_res_mm;      /* target grid resolution */
    uint8_t  ref_electrode;    /* 0 = Cu/CuSO4, 1 = Ag/AgCl */
    uint8_t  modality_mask;    /* bit0=HCP, bit1=ρ, bit2=cover, bit3=LPR */
    uint8_t  enable_hcp;       /* convenience booleans derived from mask */
    uint8_t  enable_resistivity;
    uint8_t  enable_cover;
} survey_config_t;

typedef struct {
    float    x_mm;
    float    y_mm;
    float    heading_deg;
    float    hcp_mv;            /* mV vs Cu/CuSO4 (after offset) */
    uint8_t  hcp_class;
    float    rho_ohm_m;
    uint8_t  rho_class;
    float    cover_mm;
    float    rebar_diam_mm;
    uint32_t timestamp_ms;
    uint8_t  hash[32];
} survey_point_t;

typedef enum {
    SURVEY_IDLE = 0,
    SURVEY_SCANNING,
    SURVEY_LPR,
    SURVEY_STOP
} survey_state_t;

typedef struct {
    float hcp_zero_mv;
    float agagcl_offset_mv;
    float wenner_alpha_mm;
    float wenner_k;
    float eddy_t0_us;
    float lpr_area_cm2;
    float lpr_k;
    float lpr_b_active_mv;
    float lpr_b_passive_mv;
} cal_data_t;

#define MODALITY_HCP         (1u << 0)
#define MODALITY_RESISTIVITY (1u << 1)
#define MODALITY_COVER       (1u << 2)
#define MODALITY_LPR         (1u << 3)

void survey_init(const cal_data_t *c);
void survey_set_config(const survey_config_t *c);
survey_state_t survey_get_state(void);
void survey_start(void);
void survey_stop(void);
survey_point_t survey_trigger_point(void);
const survey_point_t *survey_get_last(void);
float survey_run_lpr(float ecorr_mv, float *out_rp_kohm, float *out_i_corr_ua_cm2);
void survey_handle_ble_rx(const uint8_t *data, uint8_t len);

#endif /* REBARSCOPE_SURVEY_H */