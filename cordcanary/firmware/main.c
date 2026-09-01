/*
 * CordCanary firmware simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include <stdio.h>

#include "board.h"
#include "registers.h"
#include "drivers/comms.h"
#include "drivers/current.h"
#include "drivers/inference.h"
#include "drivers/logger.h"
#include "drivers/motion.h"
#include "drivers/power.h"
#include "drivers/strain.h"
#include "drivers/thermal.h"

const char *cc_mode_name(cc_mode_t mode)
{
    switch (mode) {
    case CC_MODE_GARAGE:
        return "GARAGE";
    case CC_MODE_RV:
        return "RV";
    case CC_MODE_WORKSHOP:
        return "WORKSHOP";
    case CC_MODE_HOME:
    default:
        return "HOME";
    }
}

const char *cc_state_name(cc_state_t state)
{
    switch (state) {
    case CC_STATE_LOAD_WATCH:
        return "LOAD_WATCH";
    case CC_STATE_OUTLET_WEAR:
        return "OUTLET_WEAR";
    case CC_STATE_CORD_FATIGUE:
        return "CORD_FATIGUE";
    case CC_STATE_DAMP_LEAKAGE:
        return "DAMP_LEAKAGE";
    case CC_STATE_ARC_SUSPECT:
        return "ARC_SUSPECT";
    case CC_STATE_NOMINAL:
    default:
        return "NOMINAL";
    }
}

static cc_mode_t mode_for_tick(unsigned tick)
{
    if (tick >= 25U && tick <= 31U) {
        return CC_MODE_GARAGE;
    }
    if (tick >= 19U && tick <= 24U) {
        return CC_MODE_RV;
    }
    if (tick >= 32U) {
        return CC_MODE_WORKSHOP;
    }
    return CC_MODE_HOME;
}

static void print_detail(unsigned tick,
                         cc_mode_t mode,
                         const cc_thermal_frame_t *thermal,
                         const cc_current_frame_t *current,
                         const cc_strain_frame_t *strain,
                         const cc_motion_frame_t *motion,
                         const cc_power_frame_t *power,
                         const cc_inference_t *inf)
{
    printf("tick=%02u mode=%-9s state=%-13s risk=%.2f conf=%.2f outlet_health=%.2f\n",
           tick,
           cc_mode_name(mode),
           cc_state_name(inf->state),
           inf->risk_score,
           inf->confidence,
           inf->outlet_health_score);
    printf("  thermal ambient=%.1fC humidity=%.1f%% shell=%.1fC plug=%.1fC cord=%.1fC hotspot=%.2fC rise=%.2fC/min\n",
           thermal->ambient_c,
           thermal->humidity_pct,
           thermal->shell_temp_c,
           thermal->plug_face_temp_c,
           thermal->cord_neck_temp_c,
           thermal->hotspot_delta_c,
           thermal->rise_rate_cpm);
    printf("  current rms=%.2fA crest=%.2f noise=%.2f leak=%.2fmA power=%.0fW transients=%.2f\n",
           current->rms_current_a,
           current->crest_factor,
           current->hf_noise_score,
           current->leakage_ma,
           current->estimated_power_w,
           current->transient_density);
    printf("  strain bend=%.1fmm pull=%.1fN torsion=%.1fdeg fatigue=%.2f secure=%s\n",
           strain->bend_radius_mm,
           strain->pull_force_n,
           strain->torsion_deg,
           strain->fatigue_index,
           strain->clipped_securely ? "yes" : "no");
    printf("  motion vibration=%.3fg wobble=%.2f drops=%u moved=%s\n",
           motion->vibration_rms_g,
           motion->wobble_score,
           motion->drop_events,
           motion->recently_moved ? "yes" : "no");
    printf("  power battery=%.1f%% voltage=%.2fV runtime=%.0fh urgent=%s\n",
           power->battery_pct,
           power->battery_v,
           power->estimated_runtime_h,
           inf->urgent_unplug ? "yes" : "no");
    printf("  advisory: %s\n", inf->advisory);
    printf("  cause:    %s\n", inf->root_cause);
}

int main(void)
{
    cc_thermal_frame_t thermal;
    cc_current_frame_t current;
    cc_strain_frame_t strain;
    cc_motion_frame_t motion;
    cc_power_frame_t power;
    cc_inference_t inf;
    cc_logger_t logger;
    cc_register_bank_t regs;
    char frame_text[CC_FRAME_TEXT_MAX];
    char reg_text[CC_FRAME_TEXT_MAX];
    unsigned tick;

    thermal_init(&thermal);
    current_init(&current);
    strain_init(&strain);
    motion_init(&motion);
    power_init(&power);
    inference_init(&inf);
    logger_init(&logger);
    cc_registers_clear(&regs);

    puts("CordCanary firmware simulation by jayis1");
    puts("============================================================");

    for (tick = 0U; tick < 40U; ++tick) {
        const cc_mode_t mode = mode_for_tick(tick);
        thermal_sample(&thermal, mode, tick);
        current_sample(&current, &thermal, mode, tick);
        strain_sample(&strain, mode, tick);
        motion_sample(&motion, &strain, mode, tick);
        inference_evaluate(&inf, &thermal, &current, &strain, &motion, &power, mode, tick);
        power_step(&power, &current, &inf, tick);
        cc_registers_capture(&regs, mode, &thermal, &current, &strain, &motion, &power, &inf);
        comms_format_frame(frame_text, sizeof(frame_text), mode, &thermal, &current, &strain, &motion, &power, &inf);
        comms_format_registers(reg_text, sizeof(reg_text), &regs);
        print_detail(tick, mode, &thermal, &current, &strain, &motion, &power, &inf);
        puts(frame_text);
        puts(reg_text);
        puts("------------------------------------------------------------");
        logger_push(&logger, tick, &inf);
    }

    logger_print(&logger);
    return 0;
}
