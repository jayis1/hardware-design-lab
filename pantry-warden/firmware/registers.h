/*
 * Pantry Warden register map sketch
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef PANTRY_WARDEN_REGISTERS_H
#define PANTRY_WARDEN_REGISTERS_H

#define PW_REG_DEVICE_ID           0x0000u
#define PW_REG_FW_VERSION          0x0004u
#define PW_REG_STATUS_FLAGS        0x0008u
#define PW_REG_MODE                0x000Cu
#define PW_REG_TICK_COUNT          0x0010u

#define PW_REG_TEMP_C              0x0100u
#define PW_REG_HUMIDITY_PCT        0x0104u
#define PW_REG_CO2_PPM             0x0108u
#define PW_REG_VOC_INDEX           0x010Cu
#define PW_REG_ETHANOL_PPM         0x0110u
#define PW_REG_STALE_AIR_INDEX     0x0114u
#define PW_REG_FAN_DUTY            0x0118u
#define PW_REG_DEW_MARGIN          0x011Cu

#define PW_REG_TOTAL_MASS_KG       0x0200u
#define PW_REG_LEFT_MASS_KG        0x0204u
#define PW_REG_RIGHT_MASS_KG       0x0208u
#define PW_REG_FRONT_GAP_MM        0x020Cu
#define PW_REG_OPTICAL_FRESHNESS   0x0210u
#define PW_REG_MOISTURE_STRIP      0x0214u
#define PW_REG_PACKAGE_TILT_DEG    0x0218u
#define PW_REG_DISTURBANCE_SCORE   0x021Cu

#define PW_REG_WINGBEAT_SCORE      0x0300u
#define PW_REG_CHEW_SCORE          0x0304u
#define PW_REG_STRUCT_ENERGY       0x0308u
#define PW_REG_AIR_ENERGY          0x030Cu
#define PW_REG_TRANSIENT_COUNT     0x0310u

#define PW_REG_BATTERY_PCT         0x0400u
#define PW_REG_BUS_VOLTAGE         0x0404u
#define PW_REG_CURRENT_MA          0x0408u
#define PW_REG_HOURS_LEFT          0x040Cu

#define PW_REG_STATE               0x0500u
#define PW_REG_HEALTH_SCORE        0x0504u
#define PW_REG_SPOILAGE_CONF       0x0508u
#define PW_REG_PEST_CONF           0x050Cu
#define PW_REG_CONDENSE_RISK       0x0510u
#define PW_REG_RESTOCK_CONF        0x0514u
#define PW_REG_ACTION_CODE         0x0518u

#define PW_FLAG_LOW_BATTERY        (1u << 0)
#define PW_FLAG_SERVICE_DOOR_OPEN  (1u << 1)
#define PW_FLAG_EVENT_PENDING      (1u << 2)
#define PW_FLAG_WIFI_LINKED        (1u << 3)
#define PW_FLAG_BLE_PAIRED         (1u << 4)

#endif
