/*
 * sx1262.h - Semtech SX1262 LoRa radio driver header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef SX1262_H
#define SX1262_H
#include <stdint.h>

int  sx1262_init(void);
void sx1262_set_frequency(uint32_t hz);
void sx1262_set_tx_power(int8_t dbm);
void sx1262_set_lora_params(uint8_t sf, uint8_t bw, uint8_t cr);
int  sx1262_send(const uint8_t *data, int len, int timeout_ms);
void sx1262_receive(uint32_t timeout_ms);
void sx1262_sleep(void);

#endif /* SX1262_H */