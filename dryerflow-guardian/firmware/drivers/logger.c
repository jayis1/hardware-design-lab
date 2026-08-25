/*
 * logger.c — cycle summaries and human-readable alerts
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "logger.h"

static dfg_sensor_frame_t g_log_ring[DFG_LOG_CAPACITY];
static uint32_t g_log_index = 0U;

void logger_init(void) {
    memset(g_log_ring, 0, sizeof(g_log_ring));
    g_log_index = 0U;
}

void logger_push_sample(const dfg_sensor_frame_t *frame) {
    if (!frame) {
        return;
    }
    g_log_ring[g_log_index % DFG_LOG_CAPACITY] = *frame;
    ++g_log_index;
}

size_t logger_render_alerts(uint32_t alerts, char *buffer, size_t buffer_size) {
    int written = 0;
    if (!buffer || buffer_size == 0U) {
        return 0U;
    }

    buffer[0] = '\0';
    if (alerts == DFG_ALERT_NONE) {
        return (size_t)snprintf(buffer, buffer_size, "none");
    }

    if (alerts & DFG_ALERT_FLOW_RESTRICTED) {
        written += snprintf(buffer + written, buffer_size - (size_t)written, "flow_restricted ");
    }
    if (alerts & DFG_ALERT_DRYING_SLOW) {
        written += snprintf(buffer + written, buffer_size - (size_t)written, "drying_slow ");
    }
    if (alerts & DFG_ALERT_BACKDRAFT_RISK) {
        written += snprintf(buffer + written, buffer_size - (size_t)written, "backdraft_risk ");
    }
    if (alerts & DFG_ALERT_OVERHEAT) {
        written += snprintf(buffer + written, buffer_size - (size_t)written, "overheat ");
    }
    if (alerts & DFG_ALERT_SERVICE_SOON) {
        written += snprintf(buffer + written, buffer_size - (size_t)written, "service_soon ");
    }
    if (alerts & DFG_ALERT_SERVICE_NOW) {
        written += snprintf(buffer + written, buffer_size - (size_t)written, "service_now ");
    }

    return (size_t)(written > 0 ? written - 1 : 0);
}

void logger_finalize_cycle(const dfg_sensor_frame_t *history, uint32_t count,
                           const dfg_health_metrics_t *metrics,
                           dfg_cycle_record_t *record) {
    uint32_t i;
    float flow_sum = 0.0f;
    float peak_pressure = 0.0f;
    float peak_temp = 0.0f;
    float peak_humidity = 0.0f;
    float peak_co = 0.0f;
    uint32_t alerts = 0U;

    if (!history || !metrics || !record || count == 0U) {
        return;
    }

    memset(record, 0, sizeof(*record));
    record->cycle_id = history[count - 1].sequence;

    for (i = 0; i < count; ++i) {
        flow_sum += history[i].flow_cfm;
        if (history[i].pressure_pa > peak_pressure) {
            peak_pressure = history[i].pressure_pa;
        }
        if (history[i].exhaust_temp_c > peak_temp) {
            peak_temp = history[i].exhaust_temp_c;
        }
        if (history[i].humidity_rh > peak_humidity) {
            peak_humidity = history[i].humidity_rh;
        }
        if (history[i].co_ppm > peak_co) {
            peak_co = history[i].co_ppm;
        }
        alerts |= history[i].alerts;
    }

    record->avg_flow_cfm = flow_sum / (float)count;
    record->peak_pressure_pa = peak_pressure;
    record->peak_temp_c = peak_temp;
    record->humidity_peak_rh = peak_humidity;
    record->max_co_ppm = peak_co;
    record->vent_resistance_index = metrics->vent_resistance_index;
    record->cycle_efficiency_score = metrics->cycle_efficiency_score;
    record->service_horizon_loads = metrics->service_horizon_loads;
    record->alerts = alerts;
}
