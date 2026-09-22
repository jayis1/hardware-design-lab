/* LoomMapper bounded diagnostic and calibration utilities
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "loommapper.h"
#include <limits.h>
#include <string.h>


static int32_t clamp_i32(int64_t value)
{
    if (value > INT32_MAX) {
        return INT32_MAX;
    }
    if (value < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static uint16_t clamp_u16(uint32_t value)
{
    if (value > UINT16_MAX) {
        return UINT16_MAX;
    }
    return (uint16_t)value;
}

static uint32_t integer_sqrt(uint64_t value)
{
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
    return (uint32_t)result;
}

void lm_calibration_defaults(lm_calibration_block_t *block)
{
    if (block == NULL) {
        return;
    }
    memset(block, 0, sizeof(*block));
    for (uint8_t channel = 0u; channel < LM_CHANNELS; ++channel) {
        block->gain_permille[channel] = 1000u;
        block->noise_limit_uv[channel] = 800u;
    }
    block->valid_mask = (1u << LM_CHANNELS) - 1u;
    block->crc = lm_crc32((const uint8_t *)block,
                          sizeof(*block) - sizeof(block->crc));
}

bool lm_calibration_valid(const lm_calibration_block_t *block)
{
    if (block == NULL) {
        return false;
    }
    uint32_t expected = lm_crc32((const uint8_t *)block,
                                 sizeof(*block) - sizeof(block->crc));
    if (expected != block->crc) {
        return false;
    }
    if ((block->valid_mask & ((1u << LM_CHANNELS) - 1u)) == 0u) {
        return false;
    }
    for (uint8_t channel = 0u; channel < LM_CHANNELS; ++channel) {
        if (block->gain_permille[channel] < 500u ||
            block->gain_permille[channel] > 1500u) {
            return false;
        }
        if (block->noise_limit_uv[channel] == 0u) {
            return false;
        }
    }
    return true;
}

lm_result_t lm_calibration_set_offset(lm_calibration_block_t *block,
                                      uint8_t channel,
                                      int32_t offset_uv)
{
    if (block == NULL || channel >= LM_CHANNELS) {
        return LM_ERR_ARG;
    }
    if (offset_uv < -250000 || offset_uv > 250000) {
        return LM_ERR_ARG;
    }
    block->offset_uv[channel] = offset_uv;
    block->valid_mask |= 1u << channel;
    block->crc = lm_crc32((const uint8_t *)block,
                          sizeof(*block) - sizeof(block->crc));
    return LM_OK;
}

lm_result_t lm_calibration_set_gain(lm_calibration_block_t *block,
                                    uint8_t channel,
                                    uint16_t gain_permille)
{
    if (block == NULL || channel >= LM_CHANNELS) {
        return LM_ERR_ARG;
    }
    if (gain_permille < 500u || gain_permille > 1500u) {
        return LM_ERR_ARG;
    }
    block->gain_permille[channel] = gain_permille;
    block->valid_mask |= 1u << channel;
    block->crc = lm_crc32((const uint8_t *)block,
                          sizeof(*block) - sizeof(block->crc));
    return LM_OK;
}

int32_t lm_calibration_apply(const lm_calibration_block_t *block,
                             uint8_t channel,
                             int32_t raw_uv)
{
    if (block == NULL || channel >= LM_CHANNELS) {
        return raw_uv;
    }
    int64_t adjusted = (int64_t)raw_uv - block->offset_uv[channel];
    adjusted *= block->gain_permille[channel];
    adjusted /= 1000;
    return clamp_i32(adjusted);
}

void lm_stats_init(lm_signal_stats_t *stats)
{
    if (stats == NULL) {
        return;
    }
    stats->minimum = INT32_MAX;
    stats->maximum = INT32_MIN;
    stats->sum = 0;
    stats->sum_squares = 0u;
    stats->count = 0u;
    stats->clipped = 0u;
}

void lm_stats_push(lm_signal_stats_t *stats, int32_t sample)
{
    if (stats == NULL) {
        return;
    }
    if (sample < stats->minimum) {
        stats->minimum = sample;
    }
    if (sample > stats->maximum) {
        stats->maximum = sample;
    }
    stats->sum += sample;
    int64_t bounded = sample;
    if (bounded > 1000000) {
        bounded = 1000000;
        stats->clipped++;
    }
    if (bounded < -1000000) {
        bounded = -1000000;
        stats->clipped++;
    }
    stats->sum_squares += (uint64_t)(bounded * bounded);
    stats->count++;
}

int32_t lm_stats_mean(const lm_signal_stats_t *stats)
{
    if (stats == NULL || stats->count == 0u) {
        return 0;
    }
    return clamp_i32(stats->sum / (int64_t)stats->count);
}

uint32_t lm_stats_rms(const lm_signal_stats_t *stats)
{
    if (stats == NULL || stats->count == 0u) {
        return 0u;
    }
    uint64_t mean_square = stats->sum_squares / stats->count;
    return integer_sqrt(mean_square);
}

uint32_t lm_stats_peak_to_peak(const lm_signal_stats_t *stats)
{
    if (stats == NULL || stats->count == 0u) {
        return 0u;
    }
    int64_t delta = (int64_t)stats->maximum - stats->minimum;
    if (delta < 0) {
        return 0u;
    }
    if ((uint64_t)delta > UINT32_MAX) {
        return UINT32_MAX;
    }
    return (uint32_t)delta;
}

uint16_t lm_signal_quality(const lm_signal_stats_t *signal,
                           const lm_signal_stats_t *noise)
{
    uint32_t signal_rms = lm_stats_rms(signal);
    uint32_t noise_rms = lm_stats_rms(noise);
    if (signal_rms <= noise_rms) {
        return 0u;
    }
    if (noise_rms == 0u) {
        return 1000u;
    }
    uint32_t ratio = ((signal_rms - noise_rms) * 100u) / noise_rms;
    return ratio >= 1000u ? 1000u : (uint16_t)ratio;
}

int32_t lm_remove_baseline(int32_t sample_uv, int32_t baseline_uv)
{
    int64_t result = (int64_t)sample_uv - baseline_uv;
    return clamp_i32(result);
}

int32_t lm_low_pass_step(int32_t previous,
                         int32_t sample,
                         uint16_t alpha_permille)
{
    if (alpha_permille > 1000u) {
        alpha_permille = 1000u;
    }
    int64_t result = (int64_t)previous * (1000u - alpha_permille);
    result += (int64_t)sample * alpha_permille;
    return clamp_i32(result / 1000);
}

int32_t lm_high_pass_step(int32_t previous_output,
                          int32_t sample,
                          int32_t previous_sample,
                          uint16_t alpha_permille)
{
    if (alpha_permille > 1000u) {
        alpha_permille = 1000u;
    }
    int64_t delta = (int64_t)sample - previous_sample;
    int64_t output = (int64_t)previous_output + delta;
    output = (output * alpha_permille) / 1000;
    return clamp_i32(output);
}

uint16_t lm_variation_permille(int32_t baseline, int32_t observed)
{
    int64_t difference = (int64_t)observed - baseline;
    if (difference < 0) {
        difference = -difference;
    }
    int64_t magnitude = baseline;
    if (magnitude < 0) {
        magnitude = -magnitude;
    }
    if (magnitude < 1000) {
        magnitude = 1000;
    }
    uint64_t ratio = (uint64_t)difference * 1000u / (uint64_t)magnitude;
    return ratio > 1000u ? 1000u : (uint16_t)ratio;
}

void lm_summary_clear(lm_graph_summary_t *summary)
{
    if (summary != NULL) {
        memset(summary, 0, sizeof(*summary));
    }
}

void lm_summary_accumulate(lm_graph_summary_t *summary,
                           lm_edge_kind_t kind)
{
    if (summary == NULL) {
        return;
    }
    switch (kind) {
    case LM_EDGE_GOOD:
        summary->good++;
        break;
    case LM_EDGE_OPEN:
        summary->open++;
        break;
    case LM_EDGE_SHORT:
        summary->shorted++;
        break;
    case LM_EDGE_INTERMITTENT:
        summary->intermittent++;
        break;
    case LM_EDGE_CROSSTALK:
        summary->crosstalk++;
        break;
    default:
        summary->unknown++;
        break;
    }
}

void lm_graph_summarize(const lm_graph_t *graph,
                        lm_graph_summary_t *summary)
{
    if (graph == NULL || summary == NULL) {
        return;
    }
    lm_summary_clear(summary);
    for (uint16_t index = 0u; index < graph->edge_count; ++index) {
        lm_summary_accumulate(summary, graph->edges[index].kind);
    }
}

uint16_t lm_graph_health(const lm_graph_t *graph)
{
    if (graph == NULL || graph->edge_count == 0u) {
        return 0u;
    }
    uint32_t penalty = 0u;
    for (uint16_t index = 0u; index < graph->edge_count; ++index) {
        const lm_edge_t *edge = &graph->edges[index];
        if (edge->kind == LM_EDGE_SHORT) {
            penalty += 250u;
        } else if (edge->kind == LM_EDGE_INTERMITTENT) {
            penalty += 180u;
        } else if (edge->kind == LM_EDGE_CROSSTALK) {
            penalty += 90u;
        } else if (edge->kind == LM_EDGE_UNKNOWN) {
            penalty += 30u;
        }
    }
    uint32_t score = penalty >= 1000u ? 0u : 1000u - penalty;
    return (uint16_t)score;
}

uint16_t lm_graph_confidence(const lm_graph_t *graph)
{
    if (graph == NULL || graph->edge_count == 0u) {
        return 0u;
    }
    uint32_t total = 0u;
    for (uint16_t index = 0u; index < graph->edge_count; ++index) {
        total += graph->edges[index].confidence_permille;
    }
    return (uint16_t)(total / graph->edge_count);
}

const lm_edge_t *lm_graph_most_variable(const lm_graph_t *graph)
{
    if (graph == NULL || graph->edge_count == 0u) {
        return NULL;
    }
    const lm_edge_t *best = &graph->edges[0];
    for (uint16_t index = 1u; index < graph->edge_count; ++index) {
        if (graph->edges[index].variation_permille >
            best->variation_permille) {
            best = &graph->edges[index];
        }
    }
    return best;
}

bool lm_graph_has_path(const lm_graph_t *graph,
                       uint8_t source,
                       uint8_t sink)
{
    const lm_edge_t *edge = graph_find(graph, source, sink);
    if (edge == NULL) {
        return false;
    }
    return edge->kind == LM_EDGE_GOOD ||
           edge->kind == LM_EDGE_INTERMITTENT;
}

uint16_t lm_event_severity_max(const lm_event_log_t *log)
{
    if (log == NULL) {
        return 0u;
    }
    uint16_t maximum = 0u;
    for (uint16_t index = 0u; index < log->count; ++index) {
        lm_event_t event;
        if (event_log_get(log, index, &event) &&
            event.severity > maximum) {
            maximum = event.severity;
        }
    }
    return maximum;
}

uint32_t lm_event_recent_count(const lm_event_log_t *log,
                               uint32_t now,
                               uint32_t window_ms)
{
    if (log == NULL) {
        return 0u;
    }
    uint32_t count = 0u;
    for (uint16_t index = 0u; index < log->count; ++index) {
        lm_event_t event;
        if (!event_log_get(log, index, &event)) {
            continue;
        }
        if ((uint32_t)(now - event.timestamp_ms) <= window_ms) {
            count++;
        }
    }
    return count;
}

uint16_t lm_battery_percent(uint16_t millivolts)
{
    if (millivolts <= 3300u) {
        return 0u;
    }
    if (millivolts >= 4200u) {
        return 100u;
    }
    uint32_t scaled = ((uint32_t)millivolts - 3300u) * 100u;
    return clamp_u16(scaled / 900u);
}

bool lm_battery_allows_scan(uint16_t millivolts,
                            lm_state_t requested_state)
{
    if (requested_state == LM_FLEX_WATCH) {
        return millivolts >= 3450u;
    }
    if (requested_state == LM_SCAN || requested_state == LM_BASELINE) {
        return millivolts >= LM_BATTERY_LOW_MV;
    }
    return true;
}

uint32_t lm_estimated_runtime_minutes(uint16_t battery_mv,
                                      uint16_t capacity_mah,
                                      lm_state_t state)
{
    if (battery_mv <= 3300u || capacity_mah == 0u) {
        return 0u;
    }
    uint32_t current_ma = 12u;
    if (state == LM_SCAN || state == LM_BASELINE) {
        current_ma = 35u;
    } else if (state == LM_FLEX_WATCH) {
        current_ma = 97u;
    }
    uint32_t usable = (uint32_t)capacity_mah *
                      lm_battery_percent(battery_mv) * 8u / 1000u;
    return usable * 60u / current_ma;
}

uint16_t lm_scan_progress(uint8_t active_source,
                          uint16_t settled_scans,
                          uint16_t required_scans)
{
    if (required_scans == 0u) {
        return 1000u;
    }
    uint32_t completed = (uint32_t)settled_scans * LM_CHANNELS;
    completed += active_source;
    uint32_t required = (uint32_t)required_scans * LM_CHANNELS;
    if (completed >= required) {
        return 1000u;
    }
    return (uint16_t)(completed * 1000u / required);
}
