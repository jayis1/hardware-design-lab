/* StitchScope phase-domain signal processing
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "stitchscope.h"
#include <limits.h>
#include <string.h>

static uint32_t absolute32(int32_t value) {
    if (value == INT32_MIN) {
        return (uint32_t)INT32_MAX + 1u;
    }
    return (uint32_t)(value < 0 ? -value : value);
}

static uint32_t integer_sqrt(uint64_t value) {
    uint64_t result = 0u;
    uint64_t bit = (uint64_t)1u << 62;
    while (bit > value) {
        bit >>= 2;
    }
    while (bit != 0u) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return result > UINT32_MAX ? UINT32_MAX : (uint32_t)result;
}

static int32_t tof_centroid(const sensor_frame_t *frame, uint32_t *confidence) {
    uint64_t weighted = 0u;
    uint32_t total = 0u;
    for (size_t i = 0u; i < 9u; ++i) {
        if (frame->tof_confidence[i] >= 80u && frame->tof_mm[i] < 300u) {
            /* Horizontal zone position plus depth creates a texture coordinate. */
            uint32_t coordinate = (uint32_t)(i % 3u) * 10000u + frame->tof_mm[i] * 100u;
            weighted += (uint64_t)coordinate * frame->tof_confidence[i];
            total += frame->tof_confidence[i];
        }
    }
    *confidence = total;
    return total == 0u ? -1 : (int32_t)(weighted / total);
}

static void clear_cycle(cycle_accumulator_t *acc, uint32_t start_us) {
    uint32_t previous = acc->start_us;
    uint32_t index = acc->cycle_index;
    memset(acc, 0, sizeof(*acc));
    acc->start_us = start_us;
    acc->previous_cycle_us = previous;
    acc->cycle_index = index;
    acc->first_tof_centroid = -1;
    acc->last_tof_centroid = -1;
    acc->active = true;
}

void signal_init(cycle_accumulator_t *acc) {
    if (acc != NULL) {
        memset(acc, 0, sizeof(*acc));
        acc->first_tof_centroid = -1;
        acc->last_tof_centroid = -1;
    }
}

static uint16_t phase_bin(const cycle_accumulator_t *acc, uint32_t timestamp_us) {
    uint32_t elapsed = timestamp_us - acc->start_us;
    uint32_t estimated = 40000u;
    if (acc->previous_cycle_us > 0u && acc->start_us > acc->previous_cycle_us) {
        estimated = acc->start_us - acc->previous_cycle_us;
    }
    if (estimated < MIN_CYCLE_US) {
        estimated = MIN_CYCLE_US;
    }
    if (estimated > MAX_CYCLE_US) {
        estimated = MAX_CYCLE_US;
    }
    uint32_t bin = (uint32_t)(((uint64_t)elapsed * PHASE_BIN_COUNT) / estimated);
    return (uint16_t)(bin >= PHASE_BIN_COUNT ? PHASE_BIN_COUNT - 1u : bin);
}

bool signal_ingest(cycle_accumulator_t *acc, const sensor_frame_t *frame,
                   bool cycle_boundary, stitch_features_t *completed) {
    if (acc == NULL || frame == NULL || completed == NULL) {
        return false;
    }
    bool produced = false;
    if (cycle_boundary) {
        if (acc->active && frame->timestamp_us - acc->start_us >= MIN_CYCLE_US) {
            signal_finalize(acc, completed, frame->timestamp_us);
            produced = true;
            ++acc->cycle_index;
        }
        clear_cycle(acc, frame->timestamp_us);
    }
    if (!acc->active) {
        return produced;
    }

    uint16_t bin = phase_bin(acc, frame->timestamp_us);
    if (acc->bin_counts[bin] < UINT16_MAX) {
        acc->force_bins[bin] += frame->force_un;
        int32_t dynamic_z = (int32_t)frame->accel_mg[2] - 1000;
        acc->accel_bins[bin] += dynamic_z;
        ++acc->bin_counts[bin];
    }

    /* ToF sampling is slower than the force stream. Duplicate timestamps are
     * harmless because centroid confidence is averaged per cycle. */
    uint32_t confidence = 0u;
    int32_t centroid = tof_centroid(frame, &confidence);
    if (centroid >= 0 && confidence > 0u) {
        if (acc->first_tof_centroid < 0) {
            acc->first_tof_centroid = centroid;
        }
        acc->last_tof_centroid = centroid;
        acc->tof_confidence_sum += confidence;
        if (acc->tof_frames < UINT16_MAX) {
            ++acc->tof_frames;
        }
    }
    return produced;
}

static int32_t averaged(const cycle_accumulator_t *acc, size_t bin, bool force) {
    if (acc->bin_counts[bin] == 0u) {
        return 0;
    }
    int32_t total = force ? acc->force_bins[bin] : acc->accel_bins[bin];
    return total / acc->bin_counts[bin];
}

void signal_finalize(cycle_accumulator_t *acc, stitch_features_t *out, uint32_t end_us) {
    memset(out, 0, sizeof(*out));
    out->cycle_index = acc->cycle_index;
    out->start_us = acc->start_us;
    out->duration_us = end_us - acc->start_us;
    out->capability_mask = sensors_capability_mask();

    int32_t minimum_force = INT32_MAX;
    int32_t previous_force = averaged(acc, 0u, true);
    uint64_t hysteresis = 0u;
    uint64_t low_energy = 0u;
    uint64_t mid_energy = 0u;
    uint64_t high_energy = 0u;
    uint32_t strongest = 0u;
    size_t strongest_bin = 0u;

    for (size_t i = 0u; i < PHASE_BIN_COUNT; ++i) {
        int32_t force = averaged(acc, i, true);
        int32_t vibration = averaged(acc, i, false);
        if (force > out->force_peak_un) {
            out->force_peak_un = force;
        }
        if (force < minimum_force) {
            minimum_force = force;
        }
        out->force_integral_un_bins += force / (int32_t)PHASE_BIN_COUNT;
        hysteresis += absolute32(force - previous_force);
        previous_force = force;

        uint32_t magnitude = absolute32(vibration);
        if (magnitude > strongest) {
            strongest = magnitude;
            strongest_bin = i;
        }
        uint64_t energy = (uint64_t)magnitude * magnitude;
        if (i < PHASE_BIN_COUNT / 3u) {
            low_energy += energy;
        } else if (i < (PHASE_BIN_COUNT * 2u) / 3u) {
            mid_energy += energy;
        } else {
            high_energy += energy;
        }
    }

    int32_t early = averaged(acc, PHASE_BIN_COUNT / 2u, true);
    int32_t late = averaged(acc, (PHASE_BIN_COUNT * 3u) / 4u, true);
    out->force_slope_un_per_bin = (late - early) / (int32_t)(PHASE_BIN_COUNT / 4u);
    out->force_hysteresis = hysteresis > UINT32_MAX ? UINT32_MAX : (uint32_t)hysteresis;
    out->impulse_low = integer_sqrt(low_energy);
    out->impulse_mid = integer_sqrt(mid_energy);
    out->impulse_high = integer_sqrt(high_energy);
    out->impulse_phase_q15 = (uint16_t)((strongest_bin * 32767u) / PHASE_BIN_COUNT);

    if (acc->first_tof_centroid >= 0 && acc->last_tof_centroid >= 0) {
        out->feed_um = (acc->last_tof_centroid - acc->first_tof_centroid) * 10;
    }
    if (acc->tof_frames > 0u) {
        uint32_t possible = (uint32_t)acc->tof_frames * 9u * 255u;
        uint32_t q15 = (uint32_t)(((uint64_t)acc->tof_confidence_sum * 32767u) / possible);
        out->feed_confidence_q15 = (uint16_t)(q15 > 32767u ? 32767u : q15);
    }
    out->phase_jitter_q15 = acc->max_phase_error_q15;
    (void)minimum_force;
}
