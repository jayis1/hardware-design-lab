/*
 * lorawan.h - LoRaWAN class-A MAC header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef LORAWAN_H
#define LORAWAN_H
#include <stdint.h>
#include "board.h"

int lorawan_init(lorawan_region_t region);
void lorawan_set_keys(const uint8_t *join_eui, const uint8_t *app_key);
int lorawan_join(void);
int lorawan_send(const uint8_t *payload, int len, int confirmed);

#endif /* LORAWAN_H */