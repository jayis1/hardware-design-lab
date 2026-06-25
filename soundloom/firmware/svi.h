/*
 * svi.h — Soil Vitality Index public interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */
#ifndef SOUNDLOOM_SVI_H
#define SOUNDLOOM_SVI_H

#include <stdint.h>
#include "env_sensors.h"
#include "board.h"

#define SVI_FLAG_PEST         0x0001
#define SVI_FLAG_COMPACTION   0x0002
#define SVI_FLAG_LOW_BATTERY  0x0004
#define SVI_FLAG_TILT         0x0008

void    svi_init(void);
uint8_t svi_compute(const float event_rates[CLS_NUM_CLASSES],
                    float spectral_diversity,
                    float noise_floor,
                    const env_data_t *env);
float   svi_trend(void);
uint16_t svi_get_flags(const float event_rates[CLS_NUM_CLASSES],
                       uint16_t battery_mv);

#endif /* SOUNDLOOM_SVI_H */