/*
 * hv.h — High-Voltage Pulse Generator Driver
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef LIGNOSCAN_HV_H
#define LIGNOSCAN_HV_H

#include <stdint.h>

void hv_init(void);
void hv_enable(void);
void hv_disable(void);
void hv_fire(uint32_t pulse_width_us);
int hv_is_ready(void);
uint32_t hv_measure_voltage(void);

/* Safety: HV is automatically disabled on watchdog timeout */
void hv_safety_check(void);

#endif /* LIGNOSCAN_HV_H */