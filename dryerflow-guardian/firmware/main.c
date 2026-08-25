/*
 * main.c — DryerFlow Guardian firmware simulation entry point
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/acoustic.h"
#include "drivers/airflow.h"
#include "drivers/analytics.h"
#include "drivers/ble.h"
#include "drivers/gas.h"
#include "drivers/logger.h"
#include "drivers/power.h"
#include "drivers/pressure.h"
#include "drivers/thermal.h"

static void print_banner(void) {
    printf("%s firmware %s\n", DFG_DEVICE_NAME, DFG_FIRMWARE_VERSION);
    printf("Author: %s\n", DFG_AUTHOR_NAME);
    printf("Logical registers: pressure=0x%04X flow=0x%04X vri=0x%04X\n\n",
           REG_PRESSURE_PA_X10, REG_FLOW_CFM_X10, REG_VRI_X10);
}

static void print_frame(const dfg_sensor_frame_t *frame, const dfg_health_metrics_t *metrics) {
    char alert_text[DFG_ALERT_TEXT_LENGTH];
    logger_render_alerts(frame->alerts, alert_text, sizeof(alert_text));
    printf("seq=%03u run=%u pressure=%6.2fPa flow=%6.2fCFM temp=%5.2fC hum=%5.2f%% ",
           frame->sequence,
           (unsigned)frame->run_state,
           frame->pressure_pa,
           frame->flow_cfm,
           frame->exhaust_temp_c,
           frame->humidity_rh);
    printf("turb=%4.2f co=%4.2f VRI=%5.2f CES=%5.2f BSS=%5.2f SH=%5.2f alerts=%s\n",
           frame->turbulence_score,
           frame->co_ppm,
           metrics->vent_resistance_index,
           metrics->cycle_efficiency_score,
           metrics->backdraft_suspicion_score,
           metrics->service_horizon_loads,
           alert_text);
}

static void summarize_cycle(const dfg_cycle_record_t *record) {
    char alert_text[DFG_ALERT_TEXT_LENGTH];
    logger_render_alerts(record->alerts, alert_text, sizeof(alert_text));
    printf("\nCycle summary\n");
    printf("-------------\n");
    printf("cycle_id:              %u\n", record->cycle_id);
    printf("average flow:          %.2f CFM\n", record->avg_flow_cfm);
    printf("peak pressure:         %.2f Pa\n", record->peak_pressure_pa);
    printf("peak exhaust temp:     %.2f C\n", record->peak_temp_c);
    printf("peak humidity:         %.2f RH\n", record->humidity_peak_rh);
    printf("maximum CO:            %.2f ppm\n", record->max_co_ppm);
    printf("vent resistance index: %.2f\n", record->vent_resistance_index);
    printf("cycle efficiency:      %.2f\n", record->cycle_efficiency_score);
    printf("service horizon:       %.2f loads\n", record->service_horizon_loads);
    printf("alerts:                %s\n", alert_text);
}

int main(void) {
    dfg_sensor_frame_t history[96];
    dfg_sensor_frame_t current;
    dfg_baseline_t baseline;
    dfg_health_metrics_t metrics;
    dfg_cycle_record_t record;
    char packet[DFG_BLE_PACKET_LENGTH];
    const char *device_id = "DFG-2608-A01";
    const bool gas_dryer = true;
    uint32_t i;
    uint32_t history_count = 0U;

    memset(history, 0, sizeof(history));
    memset(&current, 0, sizeof(current));
    memset(&baseline, 0, sizeof(baseline));
    memset(&metrics, 0, sizeof(metrics));
    memset(&record, 0, sizeof(record));

    print_banner();

    pressure_init();
    airflow_init();
    thermal_init();
    acoustic_init();
    gas_init();
    power_init();
    logger_init();
    analytics_init(&baseline);

    for (i = 0; i < 84U; ++i) {
        memset(&current, 0, sizeof(current));
        current.sequence = i + 1U;

        power_update(&current, i);
        pressure_sample(&current, i);
        thermal_sample(&current, i);
        acoustic_sample(&current, i);
        gas_sample(&current, i, gas_dryer);
        airflow_estimate(&current, &baseline);

        history[history_count] = current;
        ++history_count;

        analytics_update(history, history_count, &baseline, &metrics, &history[history_count - 1U]);
        current = history[history_count - 1U];
        logger_push_sample(&current);

        if ((i % 8U) == 0U || current.alerts != 0U || i > 76U) {
            print_frame(&current, &metrics);
        }
    }

    logger_finalize_cycle(history, history_count, &metrics, &record);
    summarize_cycle(&record);

    ble_encode_packet(device_id, &history[history_count - 1U], &metrics, packet, sizeof(packet));
    printf("\nTelemetry packet\n");
    printf("----------------\n");
    printf("%s\n", packet);

    printf("\nRecommendation\n");
    printf("--------------\n");
    if (metrics.backdraft_suspicion_score > 45.0f) {
        printf("Investigate venting immediately: gas backdraft suspicion elevated.\n");
    } else if (metrics.vent_resistance_index > DFG_RECOMMENDED_SERVICE_VRI) {
        printf("Clean the vent path soon: restriction trend is above baseline.\n");
    } else {
        printf("Vent path remains within expected service envelope.\n");
    }

    return 0;
}
