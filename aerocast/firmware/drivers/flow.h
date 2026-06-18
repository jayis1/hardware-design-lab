/*
 * flow.h — AeroCast sample-flow controller API
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef AEROCAST_FLOW_H
#define AEROCAST_FLOW_H

#include <stdint.h>

void     flow_init(void);
void     flow_set_target(float lpm);
float    flow_measured(void);
float    flow_target(void);
uint16_t flow_pwm(void);
void     flow_pi_step(void);
float    flow_dp_pa(void);

#endif /* AEROCAST_FLOW_H */