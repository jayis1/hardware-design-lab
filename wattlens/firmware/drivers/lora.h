/*
 * lora.h — SX1262 LoRa radio driver and mesh framing
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef WATTLENS_LORA_H
#define WATTLENS_LORA_H

#include "board.h"

void lora_init(void);
void lora_send_summary(const power_metrics_t *m, const nilm_result_t *n);
void lora_send_event(const pq_event_t *evt);
void lora_poll_downlink(device_ctx_t *ctx);
int  lora_join_mesh(uint16_t node_id);
bool lora_is_joined(void);

#endif /* WATTLENS_LORA_H */