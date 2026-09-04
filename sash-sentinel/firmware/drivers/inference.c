/*
 * sash-sentinel/firmware/drivers/inference.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "inference.h"
#include "airflow.h"
#include "env.h"
#include "latch.h"
#include "thermal.h"

#include <stdio.h>
#include <string.h>

void inference_init(void) {
}

static alert_level_t risk_level_from_score(float score) {
    if (score >= 75.0f) {
        return ALERT_CRITICAL;
    }
    if (score >= 45.0f) {
        return ALERT_WARNING;
    }
    if (score >= 20.0f) {
        return ALERT_INFO;
    }
    return ALERT_NONE;
}

risk_report_t inference_evaluate(const sample_history_t *history, const device_config_t *config) {
    risk_report_t report;
    memset(&report, 0, sizeof(report));

    const device_sample_t *latest = board_history_latest(history);
    if (latest == NULL) {
        snprintf(report.summary, sizeof(report.summary), "No samples available.");
        snprintf(report.action, sizeof(report.action), "Wait for baseline acquisition.");
        return report;
    }

    float condensation_headroom = latest->env.dew_point_c - latest->thermal.edge_cold_spot_c;
    float thermal_bridge = latest->thermal.thermal_gradient_c * 8.5f;
    float mold_index = env_compute_mold_index(history);
    float airflow_loss = airflow_energy_loss_score(history);
    float latch_score = latch_health_score(history);

    report.condensation_risk = board_clampf(condensation_headroom * 18.0f + latest->env.sill_moisture_pct * 0.55f + (latest->thermal.frost_signature ? 24.0f : 0.0f), 0.0f, 100.0f);
    report.infiltration_risk = board_clampf(airflow_loss * 0.8f + latest->airflow.acoustic_leak_score * 0.35f + latest->latch.sash_offset_mm * 8.0f, 0.0f, 100.0f);
    report.mold_risk = board_clampf(mold_index * 0.9f + (latest->env.voc_index - 20.0f) * 0.4f, 0.0f, 100.0f);
    report.latch_fault_risk = board_clampf(100.0f - latch_score + latest->latch.travel_cycles * 0.02f, 0.0f, 100.0f);
    report.rot_risk = board_clampf(report.condensation_risk * 0.45f + report.mold_risk * 0.35f + thermal_bridge * 0.2f, 0.0f, 100.0f);
    report.comfort_loss_risk = board_clampf(report.infiltration_risk * 0.6f + thermal_bridge * 0.5f, 0.0f, 100.0f);

    float highest = report.condensation_risk;
    if (report.infiltration_risk > highest) {
        highest = report.infiltration_risk;
    }
    if (report.mold_risk > highest) {
        highest = report.mold_risk;
    }
    if (report.latch_fault_risk > highest) {
        highest = report.latch_fault_risk;
    }
    if (report.rot_risk > highest) {
        highest = report.rot_risk;
    }
    if (report.comfort_loss_risk > highest) {
        highest = report.comfort_loss_risk;
    }
    report.level = risk_level_from_score(highest);

    if (highest == report.condensation_risk) {
        snprintf(report.summary, sizeof(report.summary), "Condensation risk elevated near the lower seal; dew point margin is %.1f C.", condensation_headroom);
        snprintf(report.action, sizeof(report.action), "Reduce indoor humidity, inspect weep paths, and add fresh weatherstrip to the coldest edge.");
    } else if (highest == report.infiltration_risk) {
        snprintf(report.summary, sizeof(report.summary), "Air leakage is dominating loss; sash offset and pressure pulses suggest a draft path.");
        snprintf(report.action, sizeof(report.action), "Re-seat the sash, adjust keeper alignment, and replace compressible seal tape around the latch edge.");
    } else if (highest == report.mold_risk) {
        snprintf(report.summary, sizeof(report.summary), "Moisture persistence across the cavity indicates mold-friendly conditions.");
        snprintf(report.action, sizeof(report.action), "Dry the sill, clean hidden corners, and improve ventilation before organic growth begins.");
    } else if (highest == report.latch_fault_risk) {
        snprintf(report.summary, sizeof(report.summary), "Latch compression is drifting out of tolerance after repeated travel cycles.");
        snprintf(report.action, sizeof(report.action), "Inspect the latch cam, keeper screws, and frame squareness; recalibrate after hardware service.");
    } else if (highest == report.rot_risk) {
        snprintf(report.summary, sizeof(report.summary), "Combined moisture and cold-bridge behavior is consistent with hidden material rot risk.");
        snprintf(report.action, sizeof(report.action), "Probe wood moisture behind trim, reseal paint breaks, and verify exterior flashing.");
    } else {
        snprintf(report.summary, sizeof(report.summary), "Thermal leakage is raising comfort and HVAC penalties.");
        snprintf(report.action, sizeof(report.action), "Prioritize seal replacement on this opening before peak heating or cooling season.");
    }

    if (report.condensation_risk < config->acceptable_condensation_score && report.level == ALERT_INFO) {
        report.level = ALERT_NONE;
    }

    return report;
}
