/*
 * lora_drv.h — SX1262 LoRa driver interface (LoRaWAN-lite uplink/downlink).
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef LORA_DRV_H
#define LORA_DRV_H

#include <stdint.h>

int lora_init(uint32_t freq_hz, uint8_t sf, uint32_t bw_hz, int8_t tx_power_dbm);
int lora_send(const uint8_t *payload, uint8_t len);
int lora_receive(uint8_t *buf, uint8_t max_len, uint32_t timeout_ms);

#endif /* LORA_DRV_H */