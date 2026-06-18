/*
 * optics.h — AeroCast optical source control API
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef AEROCAST_OPTICS_H
#define AEROCAST_OPTICS_H

#include <stdint.h>

void     optics_init(void);
void     optics_laser_on(uint16_t drive);
void     optics_laser_off(void);
void     optics_uv_on(void);
void     optics_uv_off(void);
void     optics_uv_pulse_width_us(uint16_t us);
void     optics_pmt_bias_set(uint16_t code);
uint16_t optics_pmt_bias_get(void);
uint8_t  optics_laser_is_on(void);
uint8_t  optics_uv_is_on(void);
uint8_t  optics_laser_health_check(void);

#endif /* AEROCAST_OPTICS_H */