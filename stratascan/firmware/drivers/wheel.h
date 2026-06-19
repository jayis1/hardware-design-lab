/*
 * wheel.h — Survey Wheel Quadrature Encoder Driver
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef STRATASCAN_WHEEL_H
#define STRATASCAN_WHEEL_H

#include <stdint.h>

int   wheel_init(void);
int32_t wheel_get_count(void);
float wheel_get_distance_m(void);
void  wheel_reset(void);
uint8_t wheel_check_trigger(void);

#endif /* STRATASCAN_WHEEL_H */