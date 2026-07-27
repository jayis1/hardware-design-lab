/*
 * dcir.h — DC Internal Resistance measurement
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_DCIR_H
#define LITHOCORE_DCIR_H

#include <stdint.h>

/* DCIR is measured by applying a 2 A discharge pulse for 100 ms and
 * measuring the voltage drop:
 *
 *   DCIR = ΔV / ΔI = (V_before - V_during) / 2 A
 *
 * The pulse is delivered from the supercapacitor through a MOSFET switch
 * (TIM1_CH1 PWM gate). The voltage is measured before, during (at 90 ms
 * into the pulse to avoid the inductive spike), and after (relaxation).
 *
 * The 100 ms × 2 A pulse = 0.056 mAh — negligible cell impact.
 */

#define DCIR_OK         0
#define DCIR_ERROR_PULSE -1
#define DCIR_ERROR_ADC   -2

int dcir_measure(uint16_t *dcir_mohm);

#endif /* LITHOCORE_DCIR_H */