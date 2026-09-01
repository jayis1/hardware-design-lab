/*
 * CordCanary virtual register map
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef CORDCANARY_REGISTERS_H
#define CORDCANARY_REGISTERS_H

#include "board.h"

enum {
    CC_REG_SYSTEM_STATUS = 0x00,
    CC_REG_MODE = 0x01,
    CC_REG_THERMAL_HOTSPOT = 0x10,
    CC_REG_THERMAL_RISE_RATE = 0x11,
    CC_REG_CURRENT_RMS = 0x20,
    CC_REG_CURRENT_CREST = 0x21,
    CC_REG_CURRENT_HF_NOISE = 0x22,
    CC_REG_CURRENT_LEAKAGE = 0x23,
    CC_REG_STRAIN_BEND = 0x30,
    CC_REG_STRAIN_PULL = 0x31,
    CC_REG_STRAIN_FATIGUE = 0x32,
    CC_REG_MOTION_WOBBLE = 0x40,
    CC_REG_MOTION_VIBRATION = 0x41,
    CC_REG_POWER_BATTERY = 0x50,
    CC_REG_POWER_RUNTIME = 0x51,
    CC_REG_RISK_SCORE = 0x60,
    CC_REG_OUTLET_HEALTH = 0x61,
    CC_REG_ALERT_FLAGS = 0x62
};

typedef struct {
    float system_status;
    float mode;
    float thermal_hotspot;
    float thermal_rise_rate;
    float current_rms;
    float current_crest;
    float current_hf_noise;
    float current_leakage;
    float strain_bend;
    float strain_pull;
    float strain_fatigue;
    float motion_wobble;
    float motion_vibration;
    float power_battery;
    float power_runtime;
    float risk_score;
    float outlet_health;
    float alert_flags;
} cc_register_bank_t;

static inline void cc_registers_clear(cc_register_bank_t *bank)
{
    bank->system_status = 0.0f;
    bank->mode = 0.0f;
    bank->thermal_hotspot = 0.0f;
    bank->thermal_rise_rate = 0.0f;
    bank->current_rms = 0.0f;
    bank->current_crest = 0.0f;
    bank->current_hf_noise = 0.0f;
    bank->current_leakage = 0.0f;
    bank->strain_bend = 0.0f;
    bank->strain_pull = 0.0f;
    bank->strain_fatigue = 0.0f;
    bank->motion_wobble = 0.0f;
    bank->motion_vibration = 0.0f;
    bank->power_battery = 0.0f;
    bank->power_runtime = 0.0f;
    bank->risk_score = 0.0f;
    bank->outlet_health = 0.0f;
    bank->alert_flags = 0.0f;
}

static inline void cc_registers_capture(cc_register_bank_t *bank,
                                        cc_mode_t mode,
                                        const cc_thermal_frame_t *thermal,
                                        const cc_current_frame_t *current,
                                        const cc_strain_frame_t *strain,
                                        const cc_motion_frame_t *motion,
                                        const cc_power_frame_t *power,
                                        const cc_inference_t *inf)
{
    bank->system_status = (float) inf->state;
    bank->mode = (float) mode;
    bank->thermal_hotspot = thermal->hotspot_delta_c;
    bank->thermal_rise_rate = thermal->rise_rate_cpm;
    bank->current_rms = current->rms_current_a;
    bank->current_crest = current->crest_factor;
    bank->current_hf_noise = current->hf_noise_score;
    bank->current_leakage = current->leakage_ma;
    bank->strain_bend = strain->bend_radius_mm;
    bank->strain_pull = strain->pull_force_n;
    bank->strain_fatigue = strain->fatigue_index;
    bank->motion_wobble = motion->wobble_score;
    bank->motion_vibration = motion->vibration_rms_g;
    bank->power_battery = power->battery_pct;
    bank->power_runtime = power->estimated_runtime_h;
    bank->risk_score = inf->risk_score;
    bank->outlet_health = inf->outlet_health_score;
    bank->alert_flags = inf->urgent_unplug ? 1.0f : 0.0f;
}

#endif
