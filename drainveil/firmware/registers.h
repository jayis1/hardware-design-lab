/*
 * DrainVeil register map
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef DRAINVEIL_REGISTERS_H
#define DRAINVEIL_REGISTERS_H

#define REG_PWR_STATUS              0x0001
#define REG_PWR_BATTERY_MV          0x0002
#define REG_PWR_BATTERY_PERCENT     0x0003
#define REG_PWR_EST_DAYS            0x0004
#define REG_PWR_RAIL_NOISE          0x0005

#define REG_FLOW_VELOCITY_X100      0x0100
#define REG_FLOW_REFLECT_X100       0x0101
#define REG_FLOW_TURB_X100          0x0102
#define REG_FLOW_FILL_X100          0x0103
#define REG_FLOW_SLUG_X100          0x0104
#define REG_FLOW_LPM_X100           0x0105
#define REG_FLOW_DRAIN_TIME_X10     0x0106
#define REG_FLOW_BUBBLE_X100        0x0107

#define REG_PRESSURE_KPA_X100       0x0200
#define REG_PRESSURE_PULSE_X100     0x0201
#define REG_PRESSURE_HAMMER_X100    0x0202
#define REG_PRESSURE_TRAP_X100      0x0203
#define REG_PRESSURE_VIBE_X100      0x0204
#define REG_PRESSURE_BLOCK_X100     0x0205
#define REG_PRESSURE_BRANCH_X100    0x0206

#define REG_CHEM_RH_X100            0x0300
#define REG_CHEM_COND_X100          0x0301
#define REG_CHEM_H2S_X100           0x0302
#define REG_CHEM_VOC_X100           0x0303
#define REG_CHEM_BIO_X100           0x0304
#define REG_CHEM_GREASE_X100        0x0305
#define REG_CHEM_CORROSION_X100     0x0306

#define REG_THERM_PIPE_X100         0x0400
#define REG_THERM_AMBIENT_X100      0x0401
#define REG_THERM_FREEZE_X100       0x0402
#define REG_THERM_RECOVERY_X10      0x0403
#define REG_THERM_HEATLEAK_X100     0x0404
#define REG_THERM_COLDSLUG_X100     0x0405

#define REG_INFER_CLOG_X100         0x0500
#define REG_INFER_ODOR_X100         0x0501
#define REG_INFER_FREEZE_X100       0x0502
#define REG_INFER_PRIORITY_X100     0x0503
#define REG_INFER_SERVICE_X100      0x0504
#define REG_INFER_CONFIDENCE_X100   0x0505
#define REG_INFER_EFFICIENCY_X100   0x0506
#define REG_ALERT_LEVEL             0x0507

#endif
