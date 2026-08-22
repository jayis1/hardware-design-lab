/*
 * Canopy Sentinel protocol and register map
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#ifndef CANOPY_SENTINEL_REGISTERS_H
#define CANOPY_SENTINEL_REGISTERS_H

#define CS_REG_DEVICE_INFO          0x0001
#define CS_REG_POWER_STATUS         0x0002
#define CS_REG_ACTIVE_CROP          0x0003
#define CS_REG_SCAN_COMMAND         0x0004
#define CS_REG_SCAN_RESULT          0x0005
#define CS_REG_LEAF_CAL_GAIN        0x0006
#define CS_REG_SPORE_THRESHOLD      0x0007
#define CS_REG_AIRFLOW_CAL_GAIN     0x0008
#define CS_REG_STORAGE_COUNT        0x0009
#define CS_REG_FW_VERSION           0x000A
#define CS_REG_TIMEBASE             0x000B
#define CS_REG_FACTORY_RESET        0x000C

#define CS_PKT_HELLO                0x10
#define CS_PKT_DEVICE_STATUS        0x11
#define CS_PKT_SCAN_SUMMARY         0x12
#define CS_PKT_SCAN_THERMAL         0x13
#define CS_PKT_SCAN_COMPONENTS      0x14
#define CS_PKT_SESSION_EXPORT       0x15
#define CS_PKT_ACK                  0x16
#define CS_PKT_ERROR                0x17
#define CS_PKT_SET_PROFILE          0x18
#define CS_PKT_SET_NOTES            0x19
#define CS_PKT_LOG_COUNT            0x1A

#define CS_STATUS_OK                0x00000000u
#define CS_STATUS_LOW_BATTERY       0x00000001u
#define CS_STATUS_CHARGING          0x00000002u
#define CS_STATUS_CLIP_OPEN         0x00000004u
#define CS_STATUS_SPORE_DIRTY       0x00000008u
#define CS_STATUS_STORAGE_FULL      0x00000010u
#define CS_STATUS_SENSOR_WARN       0x00000020u

#define CS_EVT_BOOT                 0x30
#define CS_EVT_SCAN_START           0x31
#define CS_EVT_SCAN_COMPLETE        0x32
#define CS_EVT_PROFILE_CHANGED      0x33
#define CS_EVT_EXPORT_COMPLETE      0x34
#define CS_EVT_LOW_BATTERY          0x35

#define CS_ERR_NONE                 0x0000
#define CS_ERR_INVALID_PACKET       0x0001
#define CS_ERR_BAD_LENGTH           0x0002
#define CS_ERR_BUSY                 0x0003
#define CS_ERR_STORAGE              0x0004
#define CS_ERR_SENSOR               0x0005
#define CS_ERR_LOW_POWER            0x0006

#endif
