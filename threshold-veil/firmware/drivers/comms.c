/*
 * Threshold Veil communications driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "comms.h"

#include <stdio.h>

#include "inference.h"

static const char *mode_name(tv_mode_t mode)
{
    switch (mode) {
        case TV_MODE_AUTO:
            return "AUTO";
        case TV_MODE_QUIET:
            return "QUIET";
        case TV_MODE_SHELTER:
            return "SHELTER";
        case TV_MODE_OPEN_FLOW:
            return "OPEN_FLOW";
        default:
            return "UNKNOWN";
    }
}

static const char *louver_name(tv_louver_t louver)
{
    switch (louver) {
        case TV_LOUVER_SAMPLE:
            return "SAMPLE";
        case TV_LOUVER_SEAL:
            return "SEAL";
        case TV_LOUVER_EQUALIZE:
            return "EQUALIZE";
        default:
            return "UNKNOWN";
    }
}

void comms_format_frame(char *buffer,
                        size_t length,
                        const tv_env_frame_t *env,
                        const tv_acoustic_frame_t *ac,
                        const tv_seal_frame_t *seal,
                        const tv_power_frame_t *power,
                        const tv_inference_t *inf,
                        tv_mode_t mode)
{
    snprintf(buffer,
             length,
             "tick=%u mode=%s state=%s pressure=%.2fPa pm25=%.1f voc_delta=%.1f ingress=%.2f seal=%.2fkPa louver=%s noise=%.2f batt=%.1f%% note=\"%s\"",
             env->tick,
             mode_name(mode),
             inference_state_name(inf->state),
             env->pressure_pa,
             env->corridor_pm25_ugm3,
             env->corridor_voc_index - env->indoor_voc_index,
             inf->ingress_score,
             seal->gasket_pressure_kpa,
             louver_name(seal->louver),
             ac->transient_score,
             power->battery_pct,
             inf->recommendation);
}
