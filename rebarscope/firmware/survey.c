/*
 * survey.c — Survey state machine + measurement orchestration + report hashing
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "board.h"
#include "registers.h"
#include "survey.h"
#include "drivers/ads1220.h"
#include "drivers/ad5940.h"
#include "drivers/eddy.h"
#include "drivers/encoder.h"
#include "drivers/sha256.h"
#include "drivers/ble_uart.h"
#include <string.h>
#include <math.h>

/* ---- Reference electrode constants ---- */
#define RE_CU_CUSO4   0
#define RE_AG_AGCL    1

static survey_config_t cfg;
static survey_state_t  state = SURVEY_IDLE;
static uint32_t state_t0_ms = 0;

/* ---- Ring buffer of recent survey points (in-RAM cache) ---- */
#define SURVEY_BUF_N 32
static survey_point_t ring[SURVEY_BUF_N];
static uint8_t ring_head = 0;

/* ---- SHA-256 hash chain state for tamper-evident log ---- */
static sha256_ctx_t hash_ctx;
static uint8_t last_hash[32];

/* ---- Calibration (loaded from flash at boot) ---- */
static cal_data_t cal;

void survey_init(const cal_data_t *c)
{
    if (c) cal = *c;
    else {
        cal.hcp_zero_mv = CAL_HCP_ZERO_MV;
        cal.agagcl_offset_mv = CAL_AGAGCL_OFFSET_MV;
        cal.wenner_alpha_mm = CAL_WENNER_ALPHA_MM;
        cal.wenner_k = CAL_WENNER_K;
        cal.eddy_t0_us = CAL_EDDY_T0_US;
        cal.lpr_area_cm2 = CAL_LPR_AREA_CM2;
        cal.lpr_k = CAL_LPR_K;
        cal.lpr_b_active_mv = CAL_LPR_B_ACTIVE_MV;
        cal.lpr_b_passive_mv = CAL_LPR_B_PASSIVE_MV;
    }
    state = SURVEY_IDLE;
    sha256_init(&hash_ctx);
    memset(last_hash, 0, 32);
    memset(ring, 0, sizeof(ring));
    ring_head = 0;
}

void survey_set_config(const survey_config_t *c)
{
    cfg = *c;
}

survey_state_t survey_get_state(void) { return state; }

static float apply_re_offset(float mv)
{
    if (cfg.ref_electrode == RE_AG_AGCL)
        return mv + cal.agagcl_offset_mv;
    return mv;
}

static uint8_t classify_hcp(float mv)
{
    /* ASTM C876 (mV vs Cu/CuSO4) */
    if (mv > -200.0f) return HCP_CLASS_NO_CORROSION;
    if (mv < -350.0f) return HCP_CLASS_ACTIVE_CORROSION;
    return HCP_CLASS_UNCERTAIN;
}

static uint8_t classify_resistivity(float rho_ohm_m)
{
    /* Resistivity classification (kΩ·cm converted from Ω·m: x100) */
    float rho_kohm_cm = rho_ohm_m * 100.0f;
    if (rho_kohm_cm >= 200.0f) return RESISTIVITY_VERY_HIGH;
    if (rho_kohm_cm >= 100.0f) return RESISTIVITY_HIGH;
    if (rho_kohm_cm >= 50.0f)  return RESISTIVITY_MODERATE;
    return RESISTIVITY_LOW;
}

/* ---- Measurement orchestration ---- */
static survey_point_t acquire_point(void)
{
    survey_point_t p;
    memset(&p, 0, sizeof(p));
    uint32_t t_ms = state_t0_ms;   /* caller sets */

    /* Position */
    float x = 0, y = 0, h = 0;
    encoder_get_position(&x, &y, &h);
    p.x_mm = x;
    p.y_mm = y;
    p.heading_deg = h;

    /* 1. HCP — average 4 samples for noise rejection */
    if (cfg.enable_hcp) {
        float sum = 0.0f;
        for (int i = 0; i < 4; i++) {
            int32_t raw = ads1220_read_raw();
            sum += ads1220_raw_to_mv(raw) - cal.hcp_zero_mv;
        }
        p.hcp_mv = apply_re_offset(sum / 4.0f);
        p.hcp_class = classify_hcp(p.hcp_mv);
    } else {
        p.hcp_mv = 0.0f;
        p.hcp_class = HCP_CLASS_NO_CORROSION;
    }

    /* 2. Wenner resistivity */
    if (cfg.enable_resistivity) {
        p.rho_ohm_m = ad5940_wenner_measure(cal.wenner_alpha_mm, 16) * cal.wenner_k;
        p.rho_class = classify_resistivity(p.rho_ohm_m);
    } else {
        p.rho_ohm_m = 0.0f;
        p.rho_class = RESISTIVITY_VERY_HIGH;
    }

    /* 3. Eddy cover depth + diameter */
    if (cfg.enable_cover) {
        float amp = 0.0f;
        p.cover_mm = eddy_measure_depth_mm(&amp);
        p.rebar_diam_mm = eddy_estimate_diameter_mm(amp, p.cover_mm);
    } else {
        p.cover_mm = 0.0f;
        p.rebar_diam_mm = 0.0f;
    }

    p.timestamp_ms = t_ms;

    /* Update hash chain: hash(previous_hash || point fields) */
    sha256_init(&hash_ctx);
    sha256_update(&hash_ctx, last_hash, 32);
    sha256_update(&hash_ctx, (const uint8_t *)&p, sizeof(p) - 32);
    sha256_final(&hash_ctx, last_hash);
    memcpy(p.hash, last_hash, 32);

    return p;
}

void survey_start(void)
{
    state = SURVEY_SCANNING;
    state_t0_ms = 0;
    encoder_reset_origin();
    sha256_init(&hash_ctx);
    memset(last_hash, 0, 32);
    ring_head = 0;
}

void survey_stop(void)
{
    state = SURVEY_IDLE;
}

survey_point_t survey_trigger_point(void)
{
    survey_point_t p = acquire_point();
    ring[ring_head] = p;
    ring_head = (ring_head + 1) % SURVEY_BUF_N;
    return p;
}

const survey_point_t *survey_get_last(void)
{
    if (ring_head == 0) return &ring[SURVEY_BUF_N - 1];
    return &ring[ring_head - 1];
}

/* ---- LPR sweep (corrosion rate in mm/yr) ---- */
#define LPR_SWEEP_SAMPLES 250   /* 0.1 mV/s for 50 mV → 500 s; we use 2 Hz sampling */
#define LPR_DT_S          0.5f

typedef struct {
    float e_mv;   /* potential offset from Ecorr */
    float i_ua;   /* current (µA) */
} lpr_sample_t;

static lpr_sample_t lpr_samples[LPR_SWEEP_SAMPLES];

float survey_run_lpr(float ecorr_mv, float *out_rp_kohm, float *out_i_corr_ua_cm2)
{
    /* Start sweep */
    ad5940_lpr_start(ecorr_mv);
    /* Collect samples over the sweep */
    float prev_e = ecorr_mv - 25.0f;
    float prev_i = 0.0f;
    float sum_dE = 0.0f, sum_dI = 0.0f;
    int n = 0;
    for (int i = 0; i < LPR_SWEEP_SAMPLES; i++) {
        float i_ua = ad5940_lpr_sample_ua();
        float e_mv = ecorr_mv - 25.0f + (50.0f * i) / (float)LPR_SWEEP_SAMPLES;
        lpr_samples[i].e_mv = e_mv;
        lpr_samples[i].i_ua = i_ua;
        if (i > 0) {
            float dE = e_mv - prev_e;
            float dI = i_ua - prev_i;
            sum_dE += dE; sum_dI += dI;
            n++;
        }
        prev_e = e_mv;
        prev_i = i_ua;
        /* delay LPR_DT_S — caller's scheduler handles this */
    }
    ad5940_lpr_stop();

    if (n == 0 || sum_dI == 0.0f) return 0.0f;

    float Rp = (sum_dE / n) / (sum_dI / n);   /* kΩ (mV/µA = kΩ) */
    if (out_rp_kohm) *out_rp_kohm = Rp;

    /* Stern-Geary: i_corr (µA/cm²) = B / Rp, B in mV, Rp in kΩ·cm² */
    float B = (ecorr_mv > -250.0f) ? cal.lpr_b_active_mv : cal.lpr_b_passive_mv;
    float area_cm2 = cal.lpr_area_cm2;
    /* Rp measured in kΩ; multiply by area to get kΩ·cm² */
    float i_corr = B / (Rp * area_cm2);   /* µA/cm² */
    if (out_i_corr_ua_cm2) *out_i_corr_ua_cm2 = i_corr;

    /* Corrosion penetration rate (mm/yr) = 0.00327 * i_corr * (M/n)
     * For Fe: M/n = 27.9 g/equiv */
    float v_corr = 0.00327f * i_corr * 27.9f;
    return v_corr;
}

/* ---- BLE command protocol ---- */

#define CMD_PING        0x01
#define CMD_GET_STATUS  0x02
#define CMD_SET_CONFIG  0x03
#define CMD_START_SURVEY 0x04
#define CMD_STOP_SURVEY 0x05
#define CMD_TRIGGER     0x06
#define CMD_LPR         0x07
#define CMD_GET_CAL     0x08
#define CMD_SET_CAL     0x09
#define CMD_GET_HASH    0x0A

#define RESP_PONG       0x81
#define RESP_STATUS     0x82
#define RESP_POINT      0x86
#define RESP_LPR        0x87
#define RESP_CAL        0x88
#define RESP_HASH       0x8A
#define RESP_ERROR      0xE0

void survey_handle_ble_rx(const uint8_t *data, uint8_t len)
{
    if (len < 1) return;
    uint8_t cmd = data[0];
    switch (cmd) {
    case CMD_PING: {
        uint8_t resp = RESP_PONG;
        ble_uart_send(&resp, 1);
        break;
    }
    case CMD_GET_STATUS: {
        uint8_t buf[8];
        buf[0] = RESP_STATUS;
        buf[1] = (uint8_t)state;
        buf[2] = (uint8_t)(ring_head);
        /* battery % + firmware version */
        extern uint8_t fuel_gauge_get_percent(void);
        buf[3] = fuel_gauge_get_percent();
        buf[4] = (uint8_t)(BOARD_FIRMWARE_VER >> 8);
        buf[5] = (uint8_t)(BOARD_FIRMWARE_VER & 0xFF);
        buf[6] = cfg.ref_electrode;
        buf[7] = cfg.modality_mask;
        ble_uart_send(buf, 8);
        break;
    }
    case CMD_SET_CONFIG: {
        if (len >= 5) {
            cfg.ref_electrode = data[1];
            cfg.modality_mask = data[2];
            cfg.grid_res_mm = (uint16_t)(data[3] | (data[4] << 8));
            uint8_t resp = 0x84;
            ble_uart_send(&resp, 1);
        }
        break;
    }
    case CMD_START_SURVEY:
        survey_start();
        break;
    case CMD_STOP_SURVEY:
        survey_stop();
        break;
    case CMD_TRIGGER: {
        survey_point_t p = survey_trigger_point();
        uint8_t buf[1 + sizeof(survey_point_t)];
        buf[0] = RESP_POINT;
        memcpy(&buf[1], &p, sizeof(survey_point_t));
        ble_uart_send(buf, 1 + (uint8_t)sizeof(survey_point_t));
        break;
    }
    case CMD_LPR: {
        float ecorr = 0.0f;
        if (len >= 5) {
            int32_t mv = data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24);
            ecorr = (float)mv;
        }
        float rp = 0, icorr = 0;
        float v = survey_run_lpr(ecorr, &rp, &icorr);
        uint8_t buf[13];
        buf[0] = RESP_LPR;
        memcpy(&buf[1], &v, 4);
        memcpy(&buf[5], &rp, 4);
        memcpy(&buf[9], &icorr, 4);
        ble_uart_send(buf, 13);
        break;
    }
    case CMD_GET_HASH: {
        uint8_t buf[33];
        buf[0] = RESP_HASH;
        memcpy(&buf[1], last_hash, 32);
        ble_uart_send(buf, 33);
        break;
    }
    default:
        ;  /* ignore */
    }
}