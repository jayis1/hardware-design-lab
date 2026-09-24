/* FlangePrint sensing, analytics, storage, protocol, and host HAL
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "board.h"
#include "registers.h"
#include <math.h>
#include <string.h>

static uint32_t simulated_ms;
static uint32_t simulation_frame;
static fp_job_record_t records[FP_HISTORY_RECORDS];
static size_t record_head;
static size_t record_used;
static const float fp_cos8[FP_SECTORS] = {
    1.0f, 0.70710678f, 0.0f, -0.70710678f,
    -1.0f, -0.70710678f, 0.0f, 0.70710678f
};
static const float fp_sin8[FP_SECTORS] = {
    0.0f, 0.70710678f, 1.0f, 0.70710678f,
    0.0f, -0.70710678f, -1.0f, -0.70710678f
};

uint16_t fp_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFu;
    if (data == NULL) return 0u;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8u;
        for (unsigned bit = 0; bit < 8u; ++bit)
            crc = (crc & 0x8000u)
                ? (uint16_t)((crc << 1u) ^ 0x1021u)
                : (uint16_t)(crc << 1u);
    }
    return crc;
}

uint32_t fp_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFu;
    if (data == NULL) return 0u;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8u; ++bit)
            crc = (crc >> 1u) ^
                  (0xEDB88320u & (uint32_t)(-(int32_t)(crc & 1u)));
    }
    return ~crc;
}

void fp_sensors_init(void)
{
    simulation_frame = 0u;
}

static fp_result_t select_optical_branch(uint8_t sector)
{
    if (sector >= FP_SECTORS) return FP_ERR_RANGE;
    uint8_t selection = (uint8_t)(1u << sector);
    return fp_i2c_write(TCA9548_ADDRESS, &selection, 1u);
}

static float median3(float a, float b, float c)
{
    if (a > b) { float t = a; a = b; b = t; }
    if (b > c) { float t = b; b = c; c = t; }
    if (a > b) { float t = a; a = b; b = t; }
    return b;
}

static fp_result_t range_sector(uint8_t sector, float *millimeters)
{
    if (millimeters == NULL) return FP_ERR_ARGUMENT;
    fp_result_t selected = select_optical_branch(sector);
    if (selected != FP_OK) return selected;
#ifdef FP_HOST_SIM
    float phase = (float)simulation_frame * 0.04f;
    float mean = 2.4f - fminf((float)simulation_frame * 0.025f, 1.5f);
    float tilt_x = 0.65f * expf(-(float)simulation_frame / 45.0f);
    float tilt_y = -0.28f * expf(-(float)simulation_frame / 55.0f);
    float local = sector == 3u ? 0.14f : 0.0f;
    float noise1 = 0.012f * sinf(phase + (float)sector * 1.31f);
    float noise2 = 0.010f * cosf(phase * 0.8f + (float)sector);
    float noise3 = -0.008f * sinf(phase * 1.1f + (float)sector * 0.7f);
    float base = mean + tilt_x * fp_cos8[sector] +
                 tilt_y * fp_sin8[sector] + local;
    *millimeters = median3(base + noise1, base + noise2, base + noise3);
    return FP_OK;
#else
    uint8_t start[3] = {
        (uint8_t)(VL6180_SYSRANGE_START >> 8u),
        (uint8_t)VL6180_SYSRANGE_START, 0x01u
    };
    if (fp_i2c_write(VL6180X_ADDRESS, start, sizeof(start)) != FP_OK)
        return FP_ERR_IO;
    uint8_t range = 0u;
    if (fp_i2c_read(VL6180X_ADDRESS, VL6180_RESULT_RANGE_VAL,
                    &range, 1u) != FP_OK) return FP_ERR_IO;
    *millimeters = (float)range;
    return FP_OK;
#endif
}

fp_result_t fp_capture_geometry(fp_geometry_frame_t *frame,
                                const fp_calibration_t *calibration)
{
    if (frame == NULL || calibration == NULL) return FP_ERR_ARGUMENT;
    memset(frame, 0, sizeof(*frame));
    frame->timestamp_ms = fp_millis();
    frame->battery_mv = fp_read_battery_mv();
    frame->board_temperature_c = fp_read_board_temperature();
    for (uint8_t sector = 0; sector < FP_SECTORS; ++sector) {
        float raw = 0.0f;
        if (range_sector(sector, &raw) == FP_OK) {
            float corrected = (raw - calibration->zero_mm[sector]) *
                              calibration->scale[sector];
            if (corrected >= 0.0f && corrected <= FP_GAP_LIMIT_MM) {
                frame->gap_mm[sector] = corrected;
                frame->temperature_c[sector] = frame->board_temperature_c +
                                                0.15f * fp_sin8[sector];
                frame->valid_mask |= (uint8_t)(1u << sector);
            }
        }
    }
    ++simulation_frame;
    return frame->valid_mask != 0u ? FP_OK : FP_ERR_IO;
}

static uint8_t popcount8(uint8_t value)
{
    uint8_t count = 0u;
    while (value != 0u) {
        count = (uint8_t)(count + (value & 1u));
        value >>= 1u;
    }
    return count;
}

static void solve_weighted(const fp_geometry_frame_t *frame,
                           const float *weights,
                           float *mean, float *x, float *y)
{
    float sw=0.0f, sg=0.0f, sc=0.0f, ss=0.0f;
    float scc=0.0f, sss=0.0f, scs=0.0f, sgc=0.0f, sgs=0.0f;
    for (size_t i = 0; i < FP_SECTORS; ++i) {
        if ((frame->valid_mask & (uint8_t)(1u << i)) == 0u) continue;
        float w=weights[i], c=fp_cos8[i], s=fp_sin8[i], g=frame->gap_mm[i];
        sw += w; sg += w*g; sc += w*c; ss += w*s;
        scc += w*c*c; sss += w*s*s; scs += w*c*s;
        sgc += w*g*c; sgs += w*g*s;
    }
    float a[3][4] = {
        {sw, sc, ss, sg},
        {sc, scc, scs, sgc},
        {ss, scs, sss, sgs}
    };
    for (size_t pivot = 0; pivot < 3u; ++pivot) {
        size_t best = pivot;
        for (size_t row = pivot + 1u; row < 3u; ++row)
            if (fabsf(a[row][pivot]) > fabsf(a[best][pivot])) best = row;
        if (best != pivot) {
            for (size_t col = pivot; col < 4u; ++col) {
                float temporary = a[pivot][col];
                a[pivot][col] = a[best][col];
                a[best][col] = temporary;
            }
        }
        float divisor = a[pivot][pivot];
        if (fabsf(divisor) < 1.0e-6f) continue;
        for (size_t col = pivot; col < 4u; ++col)
            a[pivot][col] /= divisor;
        for (size_t row = 0; row < 3u; ++row) {
            if (row == pivot) continue;
            float factor = a[row][pivot];
            for (size_t col = pivot; col < 4u; ++col)
                a[row][col] -= factor * a[pivot][col];
        }
    }
    *mean = a[0][3];
    *x = a[1][3];
    *y = a[2][3];
}

fp_result_t fp_fit_geometry(const fp_geometry_frame_t *frame, fp_fit_t *fit)
{
    if (frame == NULL || fit == NULL) return FP_ERR_ARGUMENT;
    memset(fit, 0, sizeof(*fit));
    fit->valid_mask = frame->valid_mask;
    fit->valid_count = popcount8(frame->valid_mask);
    if (fit->valid_count < FP_MIN_FIT_CHANNELS) return FP_ERR_RANGE;
    float weights[FP_SECTORS];
    for (size_t i = 0; i < FP_SECTORS; ++i)
        weights[i] = (frame->valid_mask & (uint8_t)(1u << i)) != 0u
                   ? 1.0f : 0.0f;
    for (unsigned iteration = 0; iteration < 3u; ++iteration) {
        solve_weighted(frame, weights, &fit->mean_gap_mm,
                       &fit->tilt_x_mm, &fit->tilt_y_mm);
        for (size_t i = 0; i < FP_SECTORS; ++i) {
            if (weights[i] == 0.0f) continue;
            float estimate = fit->mean_gap_mm + fit->tilt_x_mm * fp_cos8[i] +
                             fit->tilt_y_mm * fp_sin8[i];
            float magnitude = fabsf(frame->gap_mm[i] - estimate);
            weights[i] = magnitude <= 0.25f ? 1.0f : 0.25f / magnitude;
        }
    }
    float square_sum = 0.0f;
    float largest_projected = -1000.0f;
    uint8_t best = 0u;
    for (size_t i = 0; i < FP_SECTORS; ++i) {
        if ((frame->valid_mask & (uint8_t)(1u << i)) == 0u) continue;
        float estimate = fit->mean_gap_mm + fit->tilt_x_mm * fp_cos8[i] +
                         fit->tilt_y_mm * fp_sin8[i];
        fit->residual_mm[i] = frame->gap_mm[i] - estimate;
        square_sum += fit->residual_mm[i] * fit->residual_mm[i];
        float projected = frame->gap_mm[i] + 0.35f * fit->residual_mm[i];
        if (projected > largest_projected) {
            largest_projected = projected;
            best = (uint8_t)i;
        }
    }
    fit->rms_residual_mm = sqrtf(square_sum / (float)fit->valid_count);
    fit->tilt_magnitude_mm = hypotf(fit->tilt_x_mm, fit->tilt_y_mm);
    fit->tilt_direction_deg = atan2f(fit->tilt_y_mm, fit->tilt_x_mm) *
                              57.2957795f;
    if (fit->tilt_direction_deg < 0.0f) fit->tilt_direction_deg += 360.0f;
    fit->suggested_sector = best;
    fit->recommendation_valid = fit->valid_count >= FP_MIN_FIT_CHANNELS;
    return FP_OK;
}

fp_result_t fp_capture_acoustic(fp_acoustic_frame_t *frame,
                                const fp_calibration_t *calibration,
                                fp_service_mode_t mode, bool active)
{
    if (frame == NULL || calibration == NULL) return FP_ERR_ARGUMENT;
    if (active && mode == FP_SERVICE_FORBID_EXCITATION)
        return FP_ERR_INTERLOCK;
    if (active && fp_read_battery_mv() < FP_BATTERY_EXCITE_MIN_MV)
        return FP_ERR_INTERLOCK;
    if (fp_read_board_temperature() > FP_MAX_BOARD_TEMP_C)
        return FP_ERR_INTERLOCK;
    memset(frame, 0, sizeof(*frame));
    float highest = -1.0f;
    for (size_t sector = 0; sector < FP_SECTORS; ++sector) {
        float combined = 0.0f;
        for (size_t band = 0; band < FP_ACOUSTIC_BANDS; ++band) {
            float deviation = 0.03f * (float)(sector + band + 1u);
            if (sector == 5u) deviation += 0.22f * (float)(band + 1u);
            float measured = calibration->acoustic_baseline[sector][band] *
                             (1.0f + deviation);
            frame->band_energy[sector][band] = measured;
            combined += fabsf(measured /
                calibration->acoustic_baseline[sector][band] - 1.0f);
        }
        frame->correlation[sector] = fmaxf(0.0f, 1.0f - combined * 0.4f);
        frame->valid_mask |= (uint8_t)(1u << sector);
        if (combined > highest) {
            highest = combined;
            frame->hottest_sector = (uint8_t)sector;
        }
    }
    frame->risk_score = (uint8_t)fminf(100.0f, highest * 80.0f);
    return FP_OK;
}

uint8_t fp_score_risk(const fp_fit_t *fit,
                      const fp_acoustic_frame_t *acoustic)
{
    if (fit == NULL || acoustic == NULL) return 100u;
    float score = fit->tilt_magnitude_mm * 35.0f +
                  fit->rms_residual_mm * 60.0f +
                  (float)acoustic->risk_score * 0.45f;
    return (uint8_t)fminf(100.0f, fmaxf(0.0f, score));
}

void fp_job_init(fp_job_t *job)
{
    if (job == NULL) return;
    memset(job, 0, sizeof(*job));
    job->state = FP_JOB_IDLE;
}

fp_result_t fp_job_start(fp_job_t *job, fp_service_mode_t mode,
                         uint16_t bolts)
{
    if (job == NULL || bolts < 4u || bolts > 64u || (bolts & 1u) != 0u)
        return FP_ERR_ARGUMENT;
    if (job->state != FP_JOB_IDLE && job->state != FP_JOB_COMPLETE)
        return FP_ERR_STATE;
    job->state = FP_JOB_ZEROING;
    job->service_mode = mode;
    job->bolt_count = bolts;
    job->started_ms = fp_millis();
    job->last_activity_ms = job->started_ms;
    job->completed_steps = 0u;
    job->fault_count = 0u;
    ++job->sequence;
    job->excitation_armed = mode != FP_SERVICE_FORBID_EXCITATION;
    return FP_OK;
}

fp_result_t fp_job_step(fp_job_t *job, const fp_geometry_frame_t *frame,
                        const fp_fit_t *fit)
{
    if (job == NULL || frame == NULL || fit == NULL) return FP_ERR_ARGUMENT;
    if (fit->valid_count < FP_MIN_FIT_CHANNELS) return FP_ERR_RANGE;
    job->last_activity_ms = frame->timestamp_ms;
    if (frame->board_temperature_c > FP_MAX_BOARD_TEMP_C) {
        job->state = FP_JOB_FAULT;
        return FP_ERR_INTERLOCK;
    }
    switch (job->state) {
        case FP_JOB_ZEROING:
            job->state = FP_JOB_GEOMETRY;
            break;
        case FP_JOB_GEOMETRY:
            if (++job->completed_steps >= 12u) {
                job->completed_steps = 0u;
                job->state = FP_JOB_GUIDANCE;
            }
            break;
        case FP_JOB_GUIDANCE:
            if (fit->mean_gap_mm < 1.05f &&
                fit->tilt_magnitude_mm < FP_TILT_WARN_MM)
                job->state = FP_JOB_VERIFY;
            break;
        case FP_JOB_IDLE:
        case FP_JOB_ACOUSTIC_BASELINE:
        case FP_JOB_VERIFY:
        case FP_JOB_COMPLETE:
        case FP_JOB_FAULT:
            break;
    }
    if (frame->timestamp_ms - job->started_ms > FP_JOB_TIMEOUT_MS) {
        job->state = FP_JOB_FAULT;
        return FP_ERR_TIMEOUT;
    }
    return FP_OK;
}

fp_result_t fp_record_save(const fp_job_record_t *record)
{
    if (record == NULL) return FP_ERR_ARGUMENT;
    uint32_t expected = fp_crc32((const uint8_t *)record,
                                 offsetof(fp_job_record_t, crc32));
    if (expected != record->crc32) return FP_ERR_CRC;
    records[record_head] = *record;
    record_head = (record_head + 1u) % FP_HISTORY_RECORDS;
    if (record_used < FP_HISTORY_RECORDS) ++record_used;
    return FP_OK;
}

bool fp_record_get(size_t newest_index, fp_job_record_t *record)
{
    if (record == NULL || newest_index >= record_used) return false;
    size_t position = (record_head + FP_HISTORY_RECORDS - 1u - newest_index) %
                      FP_HISTORY_RECORDS;
    *record = records[position];
    return fp_crc32((const uint8_t *)record,
                    offsetof(fp_job_record_t, crc32)) == record->crc32;
}

size_t fp_record_count(void)
{
    return record_used;
}

static void put_u16(uint8_t *buffer, size_t *offset, uint16_t value)
{
    buffer[(*offset)++] = (uint8_t)value;
    buffer[(*offset)++] = (uint8_t)(value >> 8u);
}

static void put_u32(uint8_t *buffer, size_t *offset, uint32_t value)
{
    for (unsigned i = 0; i < 4u; ++i)
        buffer[(*offset)++] = (uint8_t)(value >> (i * 8u));
}

static void put_i16(uint8_t *buffer, size_t *offset, float value, float scale)
{
    int32_t scaled = (int32_t)lroundf(value * scale);
    if (scaled > 32767) scaled = 32767;
    if (scaled < -32768) scaled = -32768;
    put_u16(buffer, offset, (uint16_t)(int16_t)scaled);
}

size_t fp_encode_status(const fp_job_t *job,
                        const fp_geometry_frame_t *frame,
                        const fp_fit_t *fit,
                        const fp_acoustic_frame_t *acoustic,
                        uint8_t *output, size_t capacity)
{
    if (job == NULL || frame == NULL || fit == NULL || acoustic == NULL ||
        output == NULL || capacity < 48u) return 0u;
    size_t offset = 0u;
    output[offset++] = 'F';
    output[offset++] = 'P';
    output[offset++] = FP_PROTOCOL_VERSION;
    output[offset++] = (uint8_t)job->state;
    put_u32(output, &offset, job->sequence);
    put_u32(output, &offset, frame->timestamp_ms);
    put_u16(output, &offset, frame->battery_mv);
    output[offset++] = frame->valid_mask;
    output[offset++] = fit->suggested_sector;
    for (size_t i = 0; i < FP_SECTORS; ++i)
        put_i16(output, &offset, frame->gap_mm[i], 1000.0f);
    put_i16(output, &offset, fit->mean_gap_mm, 1000.0f);
    put_i16(output, &offset, fit->tilt_magnitude_mm, 1000.0f);
    put_i16(output, &offset, fit->tilt_direction_deg, 10.0f);
    put_i16(output, &offset, fit->rms_residual_mm, 1000.0f);
    output[offset++] = acoustic->risk_score;
    output[offset++] = acoustic->hottest_sector;
    uint16_t crc = fp_crc16(output, offset);
    put_u16(output, &offset, crc);
    return offset;
}

fp_result_t fp_handle_command(const uint8_t *packet, size_t length,
                              fp_job_t *job)
{
    if (packet == NULL || job == NULL || length < 5u)
        return FP_ERR_ARGUMENT;
    uint16_t supplied = (uint16_t)packet[length - 2u] |
                        (uint16_t)((uint16_t)packet[length - 1u] << 8u);
    if (fp_crc16(packet, length - 2u) != supplied) return FP_ERR_CRC;
    switch (packet[0]) {
        case 0x01u:
            if (length < 7u) return FP_ERR_ARGUMENT;
            return fp_job_start(job, (fp_service_mode_t)packet[1],
                (uint16_t)packet[2] |
                (uint16_t)((uint16_t)packet[3] << 8u));
        case 0x02u:
            if (job->state != FP_JOB_GUIDANCE) return FP_ERR_STATE;
            ++job->completed_steps;
            job->last_activity_ms = fp_millis();
            return FP_OK;
        case 0x03u:
            if (job->state != FP_JOB_GUIDANCE) return FP_ERR_STATE;
            job->state = FP_JOB_VERIFY;
            return FP_OK;
        case 0x04u:
            fp_job_init(job);
            return FP_OK;
        default:
            return FP_ERR_ARGUMENT;
    }
}

uint32_t fp_millis(void) { return simulated_ms; }
void fp_platform_init(void)
{
    simulated_ms = 0u;
    record_head = 0u;
    record_used = 0u;
}
void fp_platform_advance(void) { simulated_ms += FP_GEOMETRY_PERIOD_MS; }
void fp_platform_sleep(void) { }
void fp_set_status(bool red, bool green, bool blue)
{
    (void)red;
    (void)green;
    (void)blue;
}
fp_result_t fp_i2c_write(uint8_t address, const uint8_t *data, size_t length)
{
    return address != 0u && data != NULL && length != 0u
         ? FP_OK : FP_ERR_ARGUMENT;
}
fp_result_t fp_i2c_read(uint8_t address, uint16_t reg,
                        uint8_t *data, size_t length)
{
    (void)reg;
    if (address == 0u || data == NULL || length == 0u)
        return FP_ERR_ARGUMENT;
    memset(data, 0, length);
    return FP_OK;
}
fp_result_t fp_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t length)
{
    if (tx == NULL || rx == NULL) return FP_ERR_ARGUMENT;
    memcpy(rx, tx, length);
    return FP_OK;
}
int fp_ble_receive(uint8_t *data, size_t capacity)
{
    (void)data;
    (void)capacity;
    return 0;
}
void fp_ble_notify(const uint8_t *data, size_t length)
{
    (void)data;
    (void)length;
}
uint16_t fp_read_battery_mv(void)
{
    return (uint16_t)(3890u - (simulation_frame % 40u));
}
float fp_read_board_temperature(void)
{
    return 24.0f + 0.02f * (float)(simulation_frame % 20u);
}
