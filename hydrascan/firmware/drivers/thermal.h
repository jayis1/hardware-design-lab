/*
 * drivers/thermal.h — TMP117 temperature sensor
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef HYDRASCAN_THERMAL_H
#define HYDRASCAN_THERMAL_H
#include "../board.h"
hydra_err_t thermal_init(void);
hydra_err_t thermal_read(float *out_celsius);
#endif