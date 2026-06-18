/*
 * pms5003.h - PMS5003 particulate sensor header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef PMS5003_H
#define PMS5003_H
#include <stdint.h>

typedef struct {
    uint16_t pm1_0, pm2_5, pm10;            /* µg/m³ (atmospheric) */
    uint16_t pm1_0_factory, pm2_5_factory, pm10_factory;
    uint16_t particles_03, particles_05, particles_1;
    uint16_t particles_25, particles_5, particles_10;  /* #/0.1L */
} pms_data_t;

int pms5003_init(void);
int pms5003_read(pms_data_t *out);
int pms5003_sleep(void);
int pms5003_wakeup(void);

#endif /* PMS5003_H */