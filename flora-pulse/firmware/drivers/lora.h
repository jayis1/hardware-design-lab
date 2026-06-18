/*
 * lora.h — SX1276 LoRa transceiver driver interface
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FLORAPULSE_LORA_H
#define FLORAPULSE_LORA_H

#include <stdint.h>

/* Initialize SX1276 LoRa transceiver via SPI2.
 * Configures: 868 MHz, SF12, BW 125 kHz, CR 4/5, 14 dBm, sync word 0x34.
 */
void lora_init(void);

/* Send a LoRa packet. Blocks until TX complete (or timeout).
 * Returns 0 on success, non-zero on error.
 */
uint8_t lora_send(const uint8_t *data, uint8_t len);

/* Start continuous receive mode.
 * DIO0 interrupt will fire on RX_DONE.
 */
void lora_rx_start(void);

/* Read received packet data into buffer.
 * Returns number of bytes received, or 0 if nothing available.
 */
uint8_t lora_rx_read(uint8_t *buf, uint8_t maxlen);

/* Get RSSI of last received packet (dBm, negative) */
int16_t lora_get_rssi(void);

/* Get SNR of last received packet (dB) */
float lora_get_snr(void);

/* Enter sleep mode (low power, preserves config) */
void lora_sleep(void);

/* Wake up into standby mode */
void lora_wakeup(void);

/* Check if a TX or RX interrupt is pending (DIO0) */
uint8_t lora_irq_pending(void);

/* Clear interrupt flags */
void lora_clear_irq(void);

#endif /* FLORAPULSE_LORA_H */