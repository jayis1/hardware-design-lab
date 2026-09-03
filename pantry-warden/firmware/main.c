/*
         * Pantry Warden firmware simulation
         * Author: jayis1
         * Copyright (C) 2026 jayis1. All rights reserved.
         */

        #include <stdio.h>

        #include "board.h"
        #include "registers.h"
        #include "drivers/acoustic.h"
        #include "drivers/comms.h"
        #include "drivers/gas.h"
        #include "drivers/inference.h"
        #include "drivers/logger.h"
        #include "drivers/power.h"
        #include "drivers/shelf.h"

        static pw_mode_t mode_for_tick(unsigned tick)
        {
            if (tick >= 12U && tick <= 18U) {
                return PW_MODE_QUIET;
            }
            if (tick >= 37U && tick <= 45U) {
                return PW_MODE_NIGHT_SWEEP;
            }
            if (tick >= 46U) {
                return PW_MODE_CLEANOUT;
            }
            return PW_MODE_AUTO;
        }

        static void print_register_snapshot(unsigned tick,
                                            const pw_gas_frame_t *gas,
                                            const pw_shelf_frame_t *shelf,
                                            const pw_acoustic_frame_t *acoustic,
                                            const pw_power_frame_t *power,
                                            const pw_inference_t *inf)
        {
            printf("REG[0x%04X]=%u REG[0x%04X]=%.2f REG[0x%04X]=%.2f REG[0x%04X]=%.1f ",
                   PW_REG_TICK_COUNT,
                   tick,
                   PW_REG_TEMP_C,
                   gas->temp_c,
                   PW_REG_HUMIDITY_PCT,
                   gas->humidity_pct,
                   PW_REG_VOC_INDEX,
                   gas->voc_index);
            printf("REG[0x%04X]=%.2f REG[0x%04X]=%.1f REG[0x%04X]=%.1f REG[0x%04X]=%.1f\n",
                   PW_REG_TOTAL_MASS_KG,
                   shelf->total_mass_kg,
                   PW_REG_WINGBEAT_SCORE,
                   acoustic->wingbeat_score,
                   PW_REG_BATTERY_PCT,
                   power->battery_pct,
                   PW_REG_HEALTH_SCORE,
                   inf->shelf_health_score);
        }

        int main(void)
        {
            pw_gas_driver_t gas_driver;
            pw_shelf_driver_t shelf_driver;
            pw_acoustic_driver_t acoustic_driver;
            pw_power_driver_t power_driver;
            pw_gas_frame_t gas;
            pw_shelf_frame_t shelf;
            pw_acoustic_frame_t acoustic;
            pw_power_frame_t power;
            pw_inference_t inf;
            pw_logger_t logger;
            char frame[256];

            gas_init(&gas_driver, &gas);
            shelf_init(&shelf_driver, &shelf);
            acoustic_init(&acoustic_driver, &acoustic);
            power_init(&power_driver, &power);
            inference_init(&inf);
            logger_init(&logger);

            puts("Pantry Warden firmware simulation by jayis1");
            puts("============================================================");

            for (unsigned tick = 0; tick < PW_SAMPLE_COUNT; ++tick) {
                const pw_mode_t mode = mode_for_tick(tick);

                gas_sample(&gas_driver, &gas, mode, tick);
                shelf_sample(&shelf_driver, &shelf, &gas, mode, tick);
                acoustic_sample(&acoustic_driver, &acoustic, &shelf, mode, tick);
                power_step(&power_driver, &power, &gas, mode, tick);
                inference_evaluate(&inf,
                                   &gas,
                                   &shelf,
                                   &acoustic,
                                   &power,
                                   shelf_mass_delta(&shelf_driver, &shelf),
                                   mode);

                comms_format_frame(frame, sizeof(frame), tick, &gas, &shelf, &acoustic, &power, &inf, mode);
                puts(frame);
                printf("ACTION: %s\n", inf.action);
                print_register_snapshot(tick, &gas, &shelf, &acoustic, &power, &inf);
                logger_push(&logger, tick, &inf);
            }

            logger_print(&logger);
            return 0;
        }
