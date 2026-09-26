/*
 * CasterEcho order-tracked condition analytics.
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "casterecho.h"
#include <math.h>
#include <string.h>

static float maxf(float a, float b) { return a > b ? a : b; }
static float minf(float a, float b) { return a < b ? a : b; }
static float safe_ratio(float numerator, float denominator) { return fabsf(denominator) < 1e-6f ? 0.0f : numerator / denominator; }

void capture_init(ce_capture_t *capture) {
    if (capture == NULL) return;
    memset(capture, 0, sizeof(*capture));
    capture->motion = MOTION_STOPPED;
}

bool capture_push(ce_capture_t *capture, const ce_sample_t *sample) {
    if (capture == NULL || sample == NULL) return false;
    if (capture->count == 0u) capture->started_ms = sample->timestamp_ms;
    capture->samples[capture->write_index] = *sample;
    capture->write_index = (capture->write_index + 1u) % CE_WINDOW_SAMPLES;
    if (capture->count < CE_WINDOW_SAMPLES) ++capture->count;
    capture->last_speed_mps = sample->speed_mps;
    capture->last_motion_ms = sample->timestamp_ms;
    capture->motion = sensors_classify_motion(sample, capture->count > 1u ? &capture->samples[(capture->write_index + CE_WINDOW_SAMPLES - 2u) % CE_WINDOW_SAMPLES] : NULL);
    return capture->count == CE_WINDOW_SAMPLES;
}

static const ce_sample_t *sample_at(const ce_capture_t *capture, size_t logical_index) {
    size_t start = capture->count == CE_WINDOW_SAMPLES ? capture->write_index : 0u;
    return &capture->samples[(start + logical_index) % CE_WINDOW_SAMPLES];
}

float analytics_band_energy(const ce_capture_t *capture, float low_hz, float high_hz) {
    if (capture == NULL || capture->count < 8u || low_hz < 0.0f || high_hz <= low_hz) return 0.0f;
    float total = 0.0f;
    size_t bins = 0u;
    for (size_t k=1u; k<capture->count/2u; ++k) {
        const float frequency = (float)k * (float)CE_SAMPLE_HZ / (float)capture->count;
        if (frequency < low_hz || frequency > high_hz) continue;
        float real = 0.0f, imag = 0.0f;
        for (size_t n=0u; n<capture->count; ++n) {
            const ce_sample_t *s = sample_at(capture, n);
            const float x = s->accel_z_g - 1.0f;
            const float angle = 6.28318530718f * (float)k * (float)n / (float)capture->count;
            real += x * cosf(angle); imag -= x * sinf(angle);
        }
        total += (real*real + imag*imag) / ((float)capture->count * (float)capture->count);
        ++bins;
    }
    return bins == 0u ? 0.0f : total / (float)bins;
}

static float estimate_coast_decay(const ce_capture_t *capture) {
    float first = 0.0f, last = 0.0f;
    size_t valid = 0u;
    for (size_t i=0u; i<capture->count; ++i) {
        float speed = sample_at(capture, i)->speed_mps;
        if (speed < CE_MIN_SPEED_MPS) continue;
        if (valid == 0u) first = speed;
        last = speed; ++valid;
    }
    if (valid < 2u || last >= first) return 10.0f;
    float elapsed = (float)(valid - 1u) / (float)CE_SAMPLE_HZ;
    float logarithm = logf(maxf(first / maxf(last, 0.01f), 1.001f));
    return elapsed / logarithm;
}

ce_result_t analytics_compute(const ce_capture_t *capture, const ce_calibration_t *calibration, ce_features_t *features) {
    if (capture == NULL || calibration == NULL || features == NULL || capture->count < 16u) return CE_ERR_ARGUMENT;
    memset(features, 0, sizeof(*features));
    float sum_sq=0.0f, peak=0.0f, speed_sum=0.0f, temp_sum=0.0f, chatter=0.0f;
    uint16_t revolutions=0u;
    int8_t previous_direction=0;
    for (size_t i=0u; i<capture->count; ++i) {
        const ce_sample_t *s=sample_at(capture,i);
        float radial=s->accel_z_g-1.0f;
        sum_sq += radial*radial;
        peak=maxf(peak,fabsf(radial));
        peak=maxf(peak,s->piezo_g);
        speed_sum += s->speed_mps;
        temp_sum += s->bearing_temp_c;
        if (s->hall_valid && i>0u && s->hall_direction != previous_direction) chatter += 1.0f;
        if (s->hall_valid) previous_direction=s->hall_direction;
        if (i>0u) {
            const ce_sample_t *p=sample_at(capture,i-1u);
            if (p->hall_valid && s->hall_valid && s->timestamp_ms>p->timestamp_ms && s->wheel_rpm>0.0f) {
                float turns=s->wheel_rpm*(float)(s->timestamp_ms-p->timestamp_ms)/60000.0f;
                revolutions=(uint16_t)minf((float)UINT16_MAX,(float)revolutions+turns);
            }
        }
    }
    features->vibration_rms_g=sqrtf(sum_sq/(float)capture->count);
    features->crest_factor=safe_ratio(peak,features->vibration_rms_g);
    features->peak_shock_g=peak;
    features->mean_speed_mps=speed_sum/(float)capture->count;
    features->temperature_c=temp_sum/(float)capture->count;
    float wheel_hz=maxf(features->mean_speed_mps/(calibration->wheel_circumference_mm/1000.0f),0.5f);
    features->wheel_order_energy=analytics_band_energy(capture,wheel_hz*0.75f,wheel_hz*1.25f);
    features->high_band_energy=analytics_band_energy(capture,80.0f,320.0f);
    features->coast_decay_s=estimate_coast_decay(capture);
    features->drag_index=1.0f/(1.0f+features->coast_decay_s);
    features->swivel_chatter=chatter/(float)capture->count;
    features->revolutions=revolutions;
    features->flags=analytics_flags(features,calibration);
    features->health_score=analytics_health_score(features,calibration);
    return CE_OK;
}

uint16_t analytics_flags(const ce_features_t *f, const ce_calibration_t *c) {
    if (f == NULL || c == NULL) return CE_FLAG_SENSOR_FAULT;
    uint16_t flags=0u;
    float base_rms=maxf(c->baseline_rms_g,0.05f);
    float base_order=maxf(c->baseline_wheel_order,0.0001f);
    float base_high=maxf(c->baseline_high_band,0.0001f);
    if (f->crest_factor > CE_FLAT_WARN_G && f->wheel_order_energy > base_order*2.2f) flags|=CE_FLAG_FLAT_SPOT;
    if (f->high_band_energy > base_high*3.0f || f->vibration_rms_g > base_rms*2.5f) flags|=CE_FLAG_BEARING_ROUGH;
    if (f->drag_index > maxf(CE_DRAG_WARN_INDEX,c->baseline_drag*1.8f)) flags|=CE_FLAG_DRAG;
    if (f->swivel_chatter > 0.08f) flags|=CE_FLAG_SWIVEL_CHATTER;
    if (f->temperature_c > CE_TEMP_WARN_C) flags|=CE_FLAG_OVERHEAT;
    if (c->accepted_passes < CE_BASELINE_MIN_PASSES) flags|=CE_FLAG_UNCALIBRATED;
    return flags;
}

uint16_t analytics_health_score(const ce_features_t *f, const ce_calibration_t *c) {
    if (f == NULL || c == NULL) return 0u;
    float penalty=0.0f;
    penalty += minf(30.0f,maxf(0.0f,(f->vibration_rms_g/maxf(c->baseline_rms_g,0.05f)-1.0f)*14.0f));
    penalty += minf(25.0f,maxf(0.0f,(f->high_band_energy/maxf(c->baseline_high_band,0.0001f)-1.0f)*8.0f));
    penalty += minf(25.0f,f->drag_index*35.0f);
    penalty += minf(10.0f,f->swivel_chatter*80.0f);
    if (f->temperature_c>CE_TEMP_WARN_C) penalty+=10.0f;
    return (uint16_t)maxf(0.0f,100.0f-penalty);
}

void analytics_update_baseline(ce_calibration_t *c, const ce_features_t *f) {
    if (c == NULL || f == NULL || f->flags & (CE_FLAG_SENSOR_FAULT|CE_FLAG_OVERHEAT)) return;
    float alpha=c->accepted_passes<CE_BASELINE_MIN_PASSES ? 1.0f/(float)(c->accepted_passes+1u) : 0.02f;
    c->baseline_rms_g += alpha*(f->vibration_rms_g-c->baseline_rms_g);
    c->baseline_wheel_order += alpha*(f->wheel_order_energy-c->baseline_wheel_order);
    c->baseline_high_band += alpha*(f->high_band_energy-c->baseline_high_band);
    c->baseline_drag += alpha*(f->drag_index-c->baseline_drag);
    ++c->accepted_passes;
    c->crc32=ce_crc32(c,sizeof(*c)-sizeof(c->crc32));
}
