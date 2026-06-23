/*
 * protocol.h — USB/BLE binary protocol header for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef MYCOMESH_PROTOCOL_H
#define MYCOMESH_PROTOCOL_H

#include "board.h"
#include <stdint.h>

/* Initialize the USB CDC protocol handler. */
void protocol_init(void);

/* Poll for incoming USB commands (call from main loop). */
void protocol_poll(void);

/* Send a status response over USB. */
void protocol_send_status(const epoch_summary_t *epoch);

/* Send calibration data over USB (raw electrode signals). */
void protocol_send_calibration(int16_t *samples, uint16_t count);

/* Send environmental data over USB. */
void protocol_send_env(const env_data_t *env);

/* Send spike event data over USB. */
void protocol_send_spikes(const spike_event_t *spikes, uint16_t count);

/* Notify main application of USB connect/disconnect. */
void myco_usb_connect_callback(void);
void myco_usb_disconnect_callback(void);
void myco_apply_config(uint8_t duty_cycle, uint8_t mains_hz, uint8_t region);

#endif /* MYCOMESH_PROTOCOL_H */