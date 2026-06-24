/*
 * power.h — Battery, charging, and power management
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef WATTLENS_POWER_H
#define WATTLENS_POWER_H

#include "board.h"

void power_mgmt_init(void);
void power_mgmt_poll(device_ctx_t *ctx);
uint8_t power_mgmt_battery_pct(void);
bool power_mgmt_is_charging(void);
void power_mgmt_enable_charge(bool en);
void power_mgmt_shutdown(void);

#endif /* WATTLENS_POWER_H */