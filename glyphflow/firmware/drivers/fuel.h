/*
 * fuel.h — MAX17048 LiPo fuel gauge driver (I²C)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
#ifndef GLYPHFLOW_FUEL_H
#define GLYPHFLOW_FUEL_H

#include <stdint.h>

int    fuel_init(void);
uint8_t fuel_percent(void);            /* 0..100 */
uint16_t fuel_voltage_mv(void);        /* battery terminal voltage */
int    fuel_quick_start(void);

#endif /* GLYPHFLOW_FUEL_H */