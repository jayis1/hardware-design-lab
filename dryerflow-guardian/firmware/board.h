/*
 * board.h — DryerFlow Guardian board definition
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef DRYERFLOW_GUARDIAN_BOARD_H
#define DRYERFLOW_GUARDIAN_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#define DFG_AUTHOR_NAME "jayis1"
#define DFG_DEVICE_NAME "DryerFlow Guardian"
#define DFG_FIRMWARE_VERSION "1.0.0"

#define DFG_SAMPLE_RATE_HZ          2U
#define DFG_HISTORY_CAPACITY        128U
#define DFG_LOG_CAPACITY            256U
#define DFG_ALERT_TEXT_LENGTH       96U
#define DFG_DEVICE_ID_LENGTH        24U
#define DFG_BLE_PACKET_LENGTH       256U
#define DFG_BASELINE_CYCLES         2U
#define DFG_MINUTES_PER_HOUR        60U
#define DFG_SECONDS_PER_MINUTE      60U
#define DFG_BACKUP_BATTERY_MAH      600U
#define DFG_MAX_SERVICE_LOADS       30U

#define DFG_I2C_ADDR_SDP31          0x21U
#define DFG_I2C_ADDR_SHT41          0x44U
#define DFG_I2C_ADDR_SGP41          0x59U
#define DFG_I2C_ADDR_TMP117         0x48U
#define DFG_I2C_ADDR_ADS1115        0x4AU

#define DFG_GPIO_LED_STATUS         2U
#define DFG_GPIO_REED_RUNSTATE      4U
#define DFG_GPIO_USB_SENSE          6U
#define DFG_GPIO_BATTERY_THERM      7U
#define DFG_GPIO_CO_ENABLE          9U
#define DFG_GPIO_MIC_BCLK           12U
#define DFG_GPIO_MIC_WS             13U
#define DFG_GPIO_MIC_SD             14U
#define DFG_GPIO_I2C_SDA            17U
#define DFG_GPIO_I2C_SCL            18U

#define DFG_PRESSURE_FILTER_ALPHA   0.22f
#define DFG_FLOW_FILTER_ALPHA       0.18f
#define DFG_THERMAL_FILTER_ALPHA    0.15f
#define DFG_HUMIDITY_FILTER_ALPHA   0.12f
#define DFG_ACOUSTIC_FILTER_ALPHA   0.24f
#define DFG_GAS_FILTER_ALPHA        0.10f

#define DFG_VENT_DIAMETER_MM        100.0f
#define DFG_VENT_AREA_M2            0.00785f
#define DFG_STANDARD_AIR_DENSITY    1.204f
#define DFG_SAFE_CO_PPM             9.0f
#define DFG_CRITICAL_CO_PPM         35.0f
#define DFG_MAX_EXHAUST_TEMP_C      92.0f
#define DFG_RECOMMENDED_SERVICE_VRI 62.0f
#define DFG_CRITICAL_SERVICE_VRI    80.0f

typedef enum {
    DFG_RUN_IDLE = 0,
    DFG_RUN_STARTING,
    DFG_RUN_ACTIVE,
    DFG_RUN_DRYDOWN,
    DFG_RUN_COMPLETE
} dfg_run_state_t;

typedef enum {
    DFG_ALERT_NONE               = 0,
    DFG_ALERT_FLOW_RESTRICTED    = 1 << 0,
    DFG_ALERT_DRYING_SLOW        = 1 << 1,
    DFG_ALERT_BACKDRAFT_RISK     = 1 << 2,
    DFG_ALERT_OVERHEAT           = 1 << 3,
    DFG_ALERT_SERVICE_SOON       = 1 << 4,
    DFG_ALERT_SERVICE_NOW        = 1 << 5,
    DFG_ALERT_SENSOR_FAULT       = 1 << 6
} dfg_alert_bits_t;

typedef struct {
    float pressure_pa;
    float static_pressure_pa;
    float flow_cfm;
    float exhaust_temp_c;
    float ambient_temp_c;
    float humidity_rh;
    float duct_skin_temp_c;
    float turbulence_score;
    float blower_energy;
    float voc_index;
    float nox_index;
    float co_ppm;
    float battery_pct;
    float battery_temp_c;
    dfg_run_state_t run_state;
    uint32_t alerts;
    uint32_t sequence;
} dfg_sensor_frame_t;

typedef struct {
    float baseline_pressure_pa;
    float baseline_flow_cfm;
    float baseline_dryness_minutes;
    float baseline_turbulence;
    float baseline_exhaust_temp_c;
    float baseline_humidity_peak;
    bool valid;
} dfg_baseline_t;

typedef struct {
    float vent_resistance_index;
    float cycle_efficiency_score;
    float backdraft_suspicion_score;
    float lint_growth_rate;
    float service_horizon_loads;
    float dryness_transition_minutes;
    float confidence;
} dfg_health_metrics_t;

typedef struct {
    uint32_t cycle_id;
    float avg_flow_cfm;
    float peak_pressure_pa;
    float peak_temp_c;
    float humidity_peak_rh;
    float max_co_ppm;
    float vent_resistance_index;
    float cycle_efficiency_score;
    float service_horizon_loads;
    uint32_t alerts;
} dfg_cycle_record_t;

#endif
