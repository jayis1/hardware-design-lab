/*
 * SplintSense symbolic register map for simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef SPLINTSENSE_REGISTERS_H
#define SPLINTSENSE_REGISTERS_H

#define REG_PWR_STATUS         0x0001u
#define REG_PWR_BATTERY_MV     0x0002u
#define REG_PWR_BATTERY_PCT    0x0003u
#define REG_ENV_TEMP_C_X100    0x0010u
#define REG_ENV_HUMIDITY_X100  0x0011u
#define REG_ENV_VOC_INDEX      0x0012u
#define REG_ENV_IMPACT_X100    0x0013u
#define REG_ENV_STEPS          0x0014u
#define REG_PRESSURE_BASE      0x0100u
#define REG_MOISTURE_BASE      0x0200u
#define REG_RSI_X100           0x0300u
#define REG_FIT_X100           0x0301u
#define REG_ODOR_X100          0x0302u
#define REG_COMPLIANCE_X100    0x0303u
#define REG_ALERT_LEVEL        0x0304u

#define STATUS_CHARGING_MASK   0x01u
#define STATUS_LOW_BATT_MASK   0x02u
#define STATUS_ALERT_MASK      0x04u

#endif
