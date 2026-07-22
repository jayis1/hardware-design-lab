/*
 * drivers/fuel_gauge.h — MAX17048 fuel gauge interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef REBARSCOPE_FUEL_GAUGE_H
#define REBARSCOPE_FUEL_GAUGE_H

#include <stdint.h>

void  fuel_gauge_init(void);
uint8_t fuel_gauge_get_percent(void);
float fuel_gauge_get_voltage(void);

#endif /* REBARSCOPE_FUEL_GAUGE_H */