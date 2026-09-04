/*
 * sash-sentinel/firmware/registers.h
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef SASH_SENTINEL_REGISTERS_H
#define SASH_SENTINEL_REGISTERS_H

#include <stdint.h>

#define REG_SYS_STATUS           0x0000u
#define REG_ENV_STATUS           0x0010u
#define REG_THERMAL_STATUS       0x0020u
#define REG_LATCH_STATUS         0x0030u
#define REG_AIRFLOW_STATUS       0x0040u
#define REG_POWER_STATUS         0x0050u
#define REG_ALERT_STATUS         0x0060u
#define REG_TELEMETRY_TX         0x0100u
#define REG_TELEMETRY_RX         0x0200u
#define REG_EVENT_COUNTER        0x0300u
#define REG_SENSOR_FAULT_BITMAP  0x0310u

#define SYS_STATUS_BOOTED        (1u << 0)
#define SYS_STATUS_CONFIGURED    (1u << 1)
#define SYS_STATUS_WIFI_READY    (1u << 2)
#define SYS_STATUS_BLE_READY     (1u << 3)
#define SYS_STATUS_STORAGE_OK    (1u << 4)

#define ALERT_STATUS_INFO        (1u << 0)
#define ALERT_STATUS_WARNING     (1u << 1)
#define ALERT_STATUS_CRITICAL    (1u << 2)

#define SENSOR_FAULT_ENV         (1u << 0)
#define SENSOR_FAULT_THERMAL     (1u << 1)
#define SENSOR_FAULT_LATCH       (1u << 2)
#define SENSOR_FAULT_AIRFLOW     (1u << 3)
#define SENSOR_FAULT_POWER       (1u << 4)

extern uint32_t g_register_bank[256];

static inline void reg_write_u32(uint16_t address, uint32_t value) {
    g_register_bank[address / 4u] = value;
}

static inline uint32_t reg_read_u32(uint16_t address) {
    return g_register_bank[address / 4u];
}

#endif
