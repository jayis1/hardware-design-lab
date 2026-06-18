/*
 * misc.h - Blower PWM + ESP32 bridge helper prototypes
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef MISC_H
#define MISC_H
#include <stdint.h>

void blower_set_pwm(uint8_t pct);
int  esp32_bridge_recv(uint8_t *buf, int maxlen, int timeout_ms);
int  esp32_bridge_send(const uint8_t *buf, int len);
void esp32_ota_start(void);

#endif /* MISC_H */