/*
 * safety.h — Hardware safety monitoring
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_SAFETY_H
#define LITHOCORE_SAFETY_H

#include <stdint.h>

#define SAFETY_OK             0
#define SAFETY_OVP            -1   /* over-voltage (> 4.5 V) */
#define SAFETY_UVP            -2   /* under-voltage (< 1.5 V) */
#define SAFETY_OVERTEMP       -3   /* temperature > 60 °C */
#define SAFETY_REVERSE_POLARITY -4
#define SAFETY_HARDWARE_FAULT -5   /* OVP comparator triggered */

int  safety_init(void);
int  safety_check(uint16_t *voltage_mv, uint16_t *temp_dc);
int  safety_read_voltage(uint16_t *voltage_mv);
int  safety_read_temp(uint16_t *temp_dc);
uint8_t safety_check_reverse(void);
uint8_t safety_check_ovp_fault(void);
void safety_clear_fault(void);

#endif /* LITHOCORE_SAFETY_H */