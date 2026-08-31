/*
 * Threshold Veil firmware simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include <stdio.h>

#include "board.h"
#include "drivers/acoustic.h"
#include "drivers/comms.h"
#include "drivers/environment.h"
#include "drivers/inference.h"
#include "drivers/logger.h"
#include "drivers/power.h"
#include "drivers/seal.h"

static tv_mode_t mode_for_tick(unsigned tick)
{
    if (tick >= 10 && tick <= 14) {
        return TV_MODE_SHELTER;
    }
    if (tick >= 18 && tick <= 24) {
        return TV_MODE_QUIET;
    }
    if (tick >= 30 && tick <= 34) {
        return TV_MODE_OPEN_FLOW;
    }
    return TV_MODE_AUTO;
}

static void print_register_snapshot(const tv_env_frame_t *env,
                                    const tv_acoustic_frame_t *ac,
                                    const tv_seal_frame_t *seal,
                                    const tv_power_frame_t *power,
                                    const tv_inference_t *inf)
{
    printf("REG env.temp=%.2f env.rh=%.2f env.voc=%.1f env.pm25=%.1f env.pressure=%.2f ",
           env->indoor_temp_c,
           env->indoor_humidity_pct,
           env->indoor_voc_index,
           env->corridor_pm25_ugm3,
           env->pressure_pa);
    printf("ac.mid=%.2f seal.kpa=%.2f batt=%.1f state=%s ingress=%.2f\n",
           ac->mid_band_db,
           seal->gasket_pressure_kpa,
           power->battery_pct,
           inference_state_name(inf->state),
           inf->ingress_score);
}

int main(void)
{
    tv_env_frame_t env;
    tv_acoustic_frame_t acoustic;
    tv_seal_frame_t seal;
    tv_power_frame_t power;
    tv_inference_t inf;
    tv_logger_t logger;
    char frame[256];

    env_init(&env);
    acoustic_init(&acoustic);
    seal_init(&seal);
    power_init(&power);
    inference_init(&inf);
    logger_init(&logger);

    puts("Threshold Veil firmware simulation by jayis1");
    puts("------------------------------------------------------------");

    for (unsigned tick = 0; tick < 40; ++tick) {
        const tv_mode_t mode = mode_for_tick(tick);

        env_sample(&env, mode, tick);
        acoustic_sample(&acoustic, &env, mode, tick);
        inference_evaluate(&inf, &env, &acoustic, &seal, mode);
        seal_apply(&seal, &inf, &env, mode);
        power_step(&power, &seal, &env, mode);

        comms_format_frame(frame, sizeof(frame), &env, &acoustic, &seal, &power, &inf, mode);
        puts(frame);
        print_register_snapshot(&env, &acoustic, &seal, &power, &inf);

        logger_push(&logger, tick, &inf, &env, &power);
    }

    logger_print(&logger);
    return 0;
}
