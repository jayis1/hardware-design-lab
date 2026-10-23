/* SPDX-License-Identifier: MIT
 * QuadraAlign load, inertial, calibration, and leveling estimator
 * Author: jayis1
 */
#include "sensors.h"
#include "registers.h"
#include <math.h>
#include <string.h>

#define PI_F 3.14159265358979323846f
#define FILTER_TAPS 8U
#define STABILITY_WINDOW 20U

typedef struct {
    float samples[QA_NUM_FEET][FILTER_TAPS];
    float imu_energy[STABILITY_WINDOW];
    uint8_t sample_index;
    uint8_t energy_index;
    uint32_t sequence;
    bool filled;
} estimator_state_t;

static estimator_state_t state;
static int32_t simulated_count;

static float clampf(float value, float low, float high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static float mean(const float *values, uint8_t count) {
    float sum = 0.0f;
    for (uint8_t i = 0; i < count; ++i) sum += values[i];
    return count ? sum / (float)count : 0.0f;
}

static float trimmed_mean(float *values, uint8_t count) {
    for (uint8_t i = 1; i < count; ++i) {
        float key = values[i];
        int j = (int)i - 1;
        while (j >= 0 && values[j] > key) {
            values[j + 1] = values[j];
            --j;
        }
        values[j + 1] = key;
    }
    if (count < 3U) return mean(values, count);
    return mean(&values[1], (uint8_t)(count - 2U));
}

static bool ads1234_wait_ready(void) {
    uint32_t started = board_millis();
    while (board_gpio_read(PIN_ADC_DOUT_A)) {
        if ((board_millis() - started) >= ADS1234_TIMEOUT_MS) return false;
        board_delay_ms(1U);
    }
    return true;
}

static int32_t ads1234_read_channel(uint8_t channel) {
#ifdef QUADRALIGN_EMBEDDED
    uint32_t value = 0U;
    (void)channel;
    for (uint8_t bit = 0; bit < 24U; ++bit) {
        board_gpio_write(PIN_ADC_SCK, true);
        value = (value << 1U) | (board_gpio_read(PIN_ADC_DOUT_A) ? 1U : 0U);
        board_gpio_write(PIN_ADC_SCK, false);
    }
    if ((value & 0x800000U) != 0U) value |= 0xFF000000U;
    return (int32_t)value;
#else
    /* Deterministic plant model makes host self-tests repeatable. */
    simulated_count += 97;
    return 420000 + (int32_t)channel * 35000 + (simulated_count % 1200) - 600;
#endif
}

bool sensors_init(void) {
    uint8_t identity = 0U;
    uint8_t reset = LIS2DW12_SOFT_RESET;
    uint8_t config = (uint8_t)(LIS2DW12_ODR_200HZ | LIS2DW12_MODE_HIGH_PERF);
    if (board_i2c_write(LIS2DW12_ADDRESS, LIS2DW12_CTRL2, &reset, 1U) != 0) return false;
    board_delay_ms(2U);
    if (board_i2c_read(LIS2DW12_ADDRESS, LIS2DW12_WHO_AM_I, &identity, 1U) != 0) return false;
#ifndef QUADRALIGN_EMBEDDED
    identity = LIS2DW12_WHO_AM_I_VALUE;
#endif
    if (identity != LIS2DW12_WHO_AM_I_VALUE) return false;
    if (board_i2c_write(LIS2DW12_ADDRESS, LIS2DW12_CTRL1, &config, 1U) != 0) return false;
    config = (uint8_t)(LIS2DW12_BDU | LIS2DW12_IF_ADD_INC);
    return board_i2c_write(LIS2DW12_ADDRESS, LIS2DW12_CTRL2, &config, 1U) == 0;
}

bool sensors_read_load(load_frame_t *frame, const qa_config_t *config) {
    if (!frame || !config || !ads1234_wait_ready()) return false;
    frame->valid = true;
    for (uint8_t channel = 0; channel < QA_NUM_FEET; ++channel) {
        int32_t raw = ads1234_read_channel(channel);
        frame->raw[channel] = raw;
        if (config->cal[channel].counts_per_kg <= 1.0f) {
            frame->valid = false;
            frame->kg[channel] = 0.0f;
            continue;
        }
        float corrected = (float)raw - config->cal[channel].offset_counts;
        frame->kg[channel] = clampf(corrected / config->cal[channel].counts_per_kg, -5.0f, QA_MAX_LOAD_KG * 1.2f);
    }
    return frame->valid;
}

static int16_t read_i16(const uint8_t *data) {
    return (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8U));
}

bool sensors_read_imu(imu_frame_t *frame) {
    if (!frame) return false;
    uint8_t bytes[6] = {0};
    if (board_i2c_read(LIS2DW12_ADDRESS, LIS2DW12_OUT_X_L, bytes, sizeof bytes) != 0) return false;
#ifndef QUADRALIGN_EMBEDDED
    bytes[4] = 0x00U; bytes[5] = 0x10U;
#endif
    frame->ax_g = (float)(read_i16(&bytes[0]) >> 2) * LIS2DW12_SAMPLE_MG_LSB / 1000.0f;
    frame->ay_g = (float)(read_i16(&bytes[2]) >> 2) * LIS2DW12_SAMPLE_MG_LSB / 1000.0f;
    frame->az_g = (float)(read_i16(&bytes[4]) >> 2) * LIS2DW12_SAMPLE_MG_LSB / 1000.0f;
    float magnitude_xy = sqrtf(frame->ay_g * frame->ay_g + frame->az_g * frame->az_g);
    frame->pitch_deg = atan2f(-frame->ax_g, magnitude_xy) * 180.0f / PI_F;
    frame->roll_deg = atan2f(frame->ay_g, frame->az_g) * 180.0f / PI_F;
    float magnitude = sqrtf(frame->ax_g * frame->ax_g + frame->ay_g * frame->ay_g + frame->az_g * frame->az_g);
    frame->vibration_rms_g = fabsf(magnitude - 1.0f);
    frame->valid = true;
    return true;
}

void sensors_tare(qa_config_t *config, uint16_t samples) {
    if (!config || samples == 0U) return;
    int64_t sums[QA_NUM_FEET] = {0};
    for (uint16_t sample = 0; sample < samples; ++sample) {
        if (!ads1234_wait_ready()) continue;
        for (uint8_t channel = 0; channel < QA_NUM_FEET; ++channel) sums[channel] += ads1234_read_channel(channel);
    }
    for (uint8_t channel = 0; channel < QA_NUM_FEET; ++channel)
        config->cal[channel].offset_counts = (float)sums[channel] / (float)samples;
}

void sensors_calibrate_channel(qa_config_t *config, uint8_t channel, int32_t zero, int32_t loaded, float reference_kg) {
    if (!config || channel >= QA_NUM_FEET || reference_kg < 0.1f || loaded <= zero) return;
    config->cal[channel].offset_counts = (float)zero;
    config->cal[channel].counts_per_kg = (float)(loaded - zero) / reference_kg;
}

void estimator_init(void) { memset(&state, 0, sizeof state); }

static float moving_average(uint8_t channel, float value) {
    state.samples[channel][state.sample_index] = value;
    uint8_t count = state.filled ? FILTER_TAPS : (uint8_t)(state.sample_index + 1U);
    float work[FILTER_TAPS];
    memcpy(work, state.samples[channel], count * sizeof(float));
    return trimmed_mean(work, count);
}

static float calculate_imbalance(const float feet[QA_NUM_FEET], float total) {
    if (total < 1.0f) return 0.0f;
    float ideal = total / 4.0f;
    float largest = 0.0f;
    for (uint8_t i = 0; i < QA_NUM_FEET; ++i) {
        float deviation = fabsf(feet[i] - ideal);
        if (deviation > largest) largest = deviation;
    }
    return 100.0f * largest / ideal;
}

static void calculate_center(const float feet[QA_NUM_FEET], float total, const qa_config_t *config, float *x, float *y) {
    if (total < 1.0f) { *x = 0.0f; *y = 0.0f; return; }
    float left = feet[0] + feet[2];
    float right = feet[1] + feet[3];
    float front = feet[0] + feet[1];
    float rear = feet[2] + feet[3];
    *x = ((right - left) / total) * config->footprint_width_mm * 0.5f;
    *y = ((front - rear) / total) * config->footprint_depth_mm * 0.5f;
}

static uint8_t choose_foot(const qa_measurement_t *m) {
    /* Raise the corner in the downhill direction; loads break ties. */
    float scores[4] = {
        -m->pitch_deg - m->roll_deg, -m->pitch_deg + m->roll_deg,
         m->pitch_deg - m->roll_deg,  m->pitch_deg + m->roll_deg
    };
    uint8_t best = 0U;
    for (uint8_t i = 1; i < 4U; ++i) if (scores[i] > scores[best]) best = i;
    if (fabsf(m->pitch_deg) + fabsf(m->roll_deg) < 0.05f) {
        for (uint8_t i = 1; i < 4U; ++i) if (m->foot_kg[i] < m->foot_kg[best]) best = i;
    }
    return best;
}

bool estimator_update(const load_frame_t *load, const imu_frame_t *imu, const qa_config_t *config, qa_measurement_t *out) {
    if (!load || !imu || !config || !out || !load->valid || !imu->valid) return false;
    memset(out, 0, sizeof *out);
    for (uint8_t i = 0; i < QA_NUM_FEET; ++i) {
        out->foot_kg[i] = moving_average(i, load->kg[i]);
        out->total_kg += out->foot_kg[i];
        if (out->foot_kg[i] > QA_MAX_LOAD_KG) out->overloaded = true;
    }
    state.sample_index = (uint8_t)((state.sample_index + 1U) % FILTER_TAPS);
    if (state.sample_index == 0U) state.filled = true;
    out->pitch_deg = imu->pitch_deg;
    out->roll_deg = imu->roll_deg;
    out->vibration_rms_g = imu->vibration_rms_g;
    out->imbalance_percent = calculate_imbalance(out->foot_kg, out->total_kg);
    calculate_center(out->foot_kg, out->total_kg, config, &out->center_x_mm, &out->center_y_mm);
    state.imu_energy[state.energy_index] = imu->vibration_rms_g;
    state.energy_index = (uint8_t)((state.energy_index + 1U) % STABILITY_WINDOW);
    float average_energy = mean(state.imu_energy, STABILITY_WINDOW);
    out->stable = state.filled && average_energy < 0.015f;
    out->recommended_foot = choose_foot(out);
    float edge = config->footprint_depth_mm > config->footprint_width_mm ? config->footprint_depth_mm : config->footprint_width_mm;
    float angle = sqrtf(out->pitch_deg * out->pitch_deg + out->roll_deg * out->roll_deg) * PI_F / 180.0f;
    out->recommended_turns = config->screw_pitch_mm > 0.01f ? tanf(angle) * edge / config->screw_pitch_mm : 0.0f;
    out->recommended_turns = clampf(out->recommended_turns, 0.0f, 8.0f);
    out->sequence = ++state.sequence;
    return true;
}

uint32_t qa_crc32(const void *data, uint32_t length) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFUL;
    for (uint32_t i = 0; i < length; ++i) {
        crc ^= bytes[i];
        for (uint8_t bit = 0; bit < 8U; ++bit) crc = (crc >> 1U) ^ (0xEDB88320UL & (uint32_t)-(int32_t)(crc & 1U));
    }
    return ~crc;
}

void config_defaults(qa_config_t *config) {
    memset(config, 0, sizeof *config);
    for (uint8_t i = 0; i < QA_NUM_FEET; ++i) {
        config->cal[i].offset_counts = 400000.0f + 35000.0f * (float)i;
        config->cal[i].counts_per_kg = 8000.0f;
        config->cal[i].temperature_coefficient = 0.0002f;
    }
    config->footprint_width_mm = 600.0f;
    config->footprint_depth_mm = 500.0f;
    config->screw_pitch_mm = 1.5f;
    config->warn_imbalance_percent = QA_IMBALANCE_WARN_PERCENT;
    config->warn_tilt_deg = QA_TILT_WARN_DEG;
}

bool config_load(qa_config_t *config) {
    if (!config || board_flash_read(QA_FLASH_CAL_ADDRESS, config, sizeof *config) != 0) return false;
    uint32_t stored = config->crc32;
    config->crc32 = 0U;
    uint32_t calculated = qa_crc32(config, sizeof *config);
    config->crc32 = stored;
    return stored == calculated;
}

bool config_save(qa_config_t *config) {
    if (!config) return false;
    config->crc32 = 0U;
    config->crc32 = qa_crc32(config, sizeof *config);
    return board_flash_write(QA_FLASH_CAL_ADDRESS, config, sizeof *config) == 0;
}
