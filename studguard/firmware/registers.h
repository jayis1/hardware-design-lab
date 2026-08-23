/*
 * registers.h — StudGuard simulated hardware register map
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef STUDGUARD_REGISTERS_H
#define STUDGUARD_REGISTERS_H

#include <stdint.h>

typedef struct {
    uint32_t CTRL;
    uint32_t STATUS;
    uint32_t PERIOD;
    uint32_t DUTY;
    uint32_t GAIN;
} sg_piezo_regs_t;

typedef struct {
    uint32_t CTRL;
    uint32_t STATUS;
    uint32_t SAMPLE_RATE;
    uint32_t FIFO_LEVEL;
    int16_t  FIFO[512];
} sg_adc_regs_t;

typedef struct {
    uint32_t CTRL;
    uint32_t STATUS;
    uint32_t CHANNEL_SELECT;
    uint32_t RAW[4];
} sg_capsense_regs_t;

typedef struct {
    uint32_t CTRL;
    uint32_t STATUS;
    uint32_t TIMESTAMP_LO;
    uint32_t TIMESTAMP_HI;
} sg_uwb_regs_t;

typedef struct {
    uint32_t CTRL;
    uint32_t STATUS;
    uint32_t TX_POWER;
    uint32_t RX_LEVEL;
} sg_ble_regs_t;

typedef struct {
    uint32_t CTRL;
    uint32_t STATUS;
    uint32_t SOC_MILLIPCT;
    uint32_t VBAT_MV;
    uint32_t CURRENT_MA;
} sg_power_regs_t;

extern sg_piezo_regs_t   SG_PIEZO;
extern sg_adc_regs_t     SG_ADC;
extern sg_capsense_regs_t SG_CAP;
extern sg_uwb_regs_t     SG_UWB;
extern sg_ble_regs_t     SG_BLE;
extern sg_power_regs_t   SG_PWR;

#endif
