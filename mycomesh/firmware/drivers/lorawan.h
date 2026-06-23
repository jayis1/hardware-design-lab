/*
 * lorawan.h — SX1262 LoRaWAN driver header for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef MYCOMESH_LORAWAN_H
#define MYCOMESH_LORAWAN_H

#include <stdint.h>

/* Initialize the SX1262 LoRa transceiver and LoRaWAN MAC layer. */
int lorawan_init(uint8_t region);

/* Send a LoRaWAN uplink on the specified port. */
int lorawan_send(uint8_t port, const uint8_t *payload, uint8_t len);

/* Set the LoRaWAN region (0=EU868, 1=US915). */
void lorawan_set_region(uint8_t region);

/* Check if the node has joined the LoRaWAN network. */
uint8_t lorawan_is_joined(void);

/* Set device EUI, app EUI, and app key (for OTAA join). */
void lorawan_set_keys(const uint8_t *dev_eui, const uint8_t *app_eui,
                      const uint8_t *app_key);

/* Initiate OTAA join procedure. */
int lorawan_join(void);

/* Process any pending LoRaWAN downlink messages. */
void lorawan_process_downlink(void);

/* Get last RSSI and SNR. */
int16_t lorawan_last_rssi(void);
int8_t  lorawan_last_snr(void);

#endif /* MYCOMESH_LORAWAN_H */