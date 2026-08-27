/*
 * SealBeat register map
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef SEALBEAT_REGISTERS_H
#define SEALBEAT_REGISTERS_H

#define REG_PWR_STATUS               0x0001u
#define REG_PWR_BATTERY_MV           0x0002u
#define REG_PWR_BATTERY_PERCENT      0x0003u
#define REG_PWR_EST_DAYS             0x0004u

#define REG_ACOUSTIC_LATCH_X100      0x0100u
#define REG_ACOUSTIC_HARM_X100       0x0101u
#define REG_ACOUSTIC_VIBE_X100       0x0102u
#define REG_ACOUSTIC_BURDEN_X100     0x0103u

#define REG_DOOR_ANGLE_X100          0x0200u
#define REG_DOOR_DWELL_S             0x0201u
#define REG_DOOR_BOUNCE_X100         0x0202u
#define REG_DOOR_HINGE_X100          0x0203u
#define REG_DOOR_CYCLES              0x0204u

#define REG_SEAL_TOP_X100            0x0300u
#define REG_SEAL_LATCH_X100          0x0301u
#define REG_SEAL_BOTTOM_X100         0x0302u
#define REG_SEAL_HINGE_X100          0x0303u
#define REG_SEAL_COMP_X100           0x0304u
#define REG_SEAL_PULL_X100           0x0305u
#define REG_SEAL_GAP_X100            0x0306u
#define REG_SEAL_VECTOR_X100         0x0307u

#define REG_THERM_EDGE_X100          0x0400u
#define REG_THERM_COMP_X100          0x0401u
#define REG_THERM_REBOUND_X100       0x0402u
#define REG_THERM_TAU_X10            0x0403u
#define REG_THERM_FROST_X100         0x0404u
#define REG_THERM_SAFETY_X100        0x0405u

#define REG_INFER_SEAL_X100          0x0500u
#define REG_INFER_SAFETY_X100        0x0501u
#define REG_INFER_HINGE_X100         0x0502u
#define REG_INFER_PRIORITY_X100      0x0503u
#define REG_INFER_ENERGY_X100        0x0504u
#define REG_INFER_SERVICE_X100       0x0505u
#define REG_ALERT_LEVEL              0x05FFu

#endif
