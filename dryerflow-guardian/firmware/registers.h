/*
 * registers.h — logical register map for DryerFlow Guardian
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef DRYERFLOW_GUARDIAN_REGISTERS_H
#define DRYERFLOW_GUARDIAN_REGISTERS_H

#define REG_DEVICE_ID_LOW              0x0000
#define REG_DEVICE_ID_HIGH             0x0001
#define REG_FW_VERSION_MAJOR           0x0002
#define REG_FW_VERSION_MINOR           0x0003
#define REG_PRESSURE_PA_X10            0x0010
#define REG_STATIC_PRESSURE_PA_X10     0x0011
#define REG_FLOW_CFM_X10               0x0012
#define REG_EXHAUST_TEMP_C_X10         0x0013
#define REG_AMBIENT_TEMP_C_X10         0x0014
#define REG_HUMIDITY_RH_X10            0x0015
#define REG_DUCT_SKIN_TEMP_C_X10       0x0016
#define REG_TURBULENCE_X100            0x0017
#define REG_BLOWER_ENERGY_X100         0x0018
#define REG_VOC_INDEX_X10              0x0019
#define REG_NOX_INDEX_X10              0x001A
#define REG_CO_PPM_X10                 0x001B
#define REG_BATTERY_PERCENT_X10        0x001C
#define REG_BATTERY_TEMP_C_X10         0x001D
#define REG_RUN_STATE                  0x001E
#define REG_ALERT_FLAGS                0x001F
#define REG_VRI_X10                    0x0020
#define REG_CES_X10                    0x0021
#define REG_BSS_X10                    0x0022
#define REG_LGR_X100                   0x0023
#define REG_SERVICE_HORIZON_X10        0x0024
#define REG_DRYNESS_MINUTES_X10        0x0025
#define REG_CONFIDENCE_X10             0x0026
#define REG_BASELINE_PRESSURE_X10      0x0030
#define REG_BASELINE_FLOW_X10          0x0031
#define REG_BASELINE_DRYNESS_X10       0x0032
#define REG_BASELINE_TURBULENCE_X100   0x0033
#define REG_FACTORY_RESET              0x00F0
#define REG_ENTER_BOOTLOADER           0x00F1
#define REG_STORE_BASELINE             0x00F2
#define REG_SELF_TEST                  0x00F3

#endif
