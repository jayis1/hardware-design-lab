/*
 * drivers/sx1276.h — SX1276 LoRa transceiver driver
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
#ifndef HIVEVOX_SX1276_H
#define HIVEVOX_SX1276_H

#include <stdint.h>

int    sx1276_init(void);
int    sx1276_tx(const uint8_t *payload, uint8_t len);
int    sx1276_rx(uint8_t *buf, uint8_t max_len, uint32_t timeout_ms);
void   sx1276_sleep(void);
int8_t sx1276_last_rssi(void);
void   sx1276_set_sf(uint8_t sf);

void   delay_ms(uint32_t ms);

#endif /* HIVEVOX_SX1276_H */