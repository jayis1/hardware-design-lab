/*
 * lora_link.h — LoRa SX1262 driver and protocol layer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#ifndef LORA_LINK_H
#define LORA_LINK_H

#include <stdint.h>
#include <stdbool.h>
#include "glyph_engine.h"

/* Initialize SX1262 LoRa radio */
bool lora_init(void);

/* Send a glyph command to a buddy device via LoRa */
bool lora_send_glyph(uint8_t buddy_id, const glyph_cmd_t *cmd);

/* Broadcast SOS alert to all nearby ThermoGlyph devices */
bool lora_send_sos(uint8_t my_id, int16_t lat, int16_t lon);

/* Send heartbeat / position beacon */
bool lora_send_heartbeat(uint8_t my_id, int16_t lat, int16_t lon);

/* Process LoRa events (called from comm_task).
 * Returns true if a glyph command was received. */
bool lora_process(glyph_cmd_t *out_cmd, uint8_t *out_sender_id);

/* Get last RSSI of received packet */
int8_t lora_get_last_rssi(void);

/* Enter CAD (Channel Activity Detection) mode for low-power listen */
void lora_enter_cad(void);

/* Sleep the radio (wake on preamble detect via DIO1) */
void lora_sleep(void);

/* Wake radio from sleep */
void lora_wake(void);

/* Check if radio is currently transmitting */
bool lora_is_tx_busy(void);

#endif /* LORA_LINK_H */