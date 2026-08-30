/*
 * CrisperCue register map
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef CRISPERCUE_REGISTERS_H
#define CRISPERCUE_REGISTERS_H

#define REG_PWR_STATUS                0x0001
#define REG_PWR_BATTERY_MV            0x0002
#define REG_PWR_BATTERY_PERCENT       0x0003
#define REG_PWR_CURRENT_MA            0x0004
#define REG_GAS_CO2_X10               0x0010
#define REG_GAS_ETHYLENE_X100         0x0011
#define REG_GAS_VOC_X10               0x0012
#define REG_GAS_OXYGEN_X100           0x0013
#define REG_GAS_HUMIDITY_X100         0x0014
#define REG_GAS_PURGE_X100            0x0015
#define REG_MASS_TRAY_G               0x0020
#define REG_MASS_LOSS_G_X10           0x0021
#define REG_MASS_MOISTURE_X100        0x0022
#define REG_MASS_USAGE_X100           0x0023
#define REG_OPT_COLOR_X100            0x0030
#define REG_OPT_CHLORO_X100           0x0031
#define REG_OPT_BRUISE_X100           0x0032
#define REG_OPT_MOLD_X100             0x0033
#define REG_OPT_GLOSS_X100            0x0034
#define REG_TH_AIR_C_X100             0x0040
#define REG_TH_PRODUCE_C_X100         0x0041
#define REG_TH_DEW_MARGIN_X100        0x0042
#define REG_TH_COMPRESSOR_X100        0x0043
#define REG_TH_OPEN_MIN_X10           0x0044
#define REG_INFER_FRESHNESS_X100      0x0050
#define REG_INFER_SPOILAGE_X100       0x0051
#define REG_INFER_RECIPE_X100         0x0052
#define REG_INFER_VENT_X100           0x0053
#define REG_INFER_VALUE_CENTS         0x0054
#define REG_ALERT_LEVEL               0x0055

#endif
