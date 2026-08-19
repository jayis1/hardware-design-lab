/*
 * ble.h — nRF52840 BLE bridge driver interface
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef SPECKLEFLOW_BLE_H
#define SPECKLEFLOW_BLE_H

#include <stdint.h>

/**
 * Initialize the BLE bridge (reset nRF52840, enable RX interrupt).
 */
int ble_init(void);

/**
 * Send a 128-byte flow-map tile to the app via BLE.
 * @param tile      128 bytes of 16×8 pixel flow data
 * @param tile_idx   Tile index within the frame (0–2399)
 */
void ble_send_flow_tile(const uint8_t *tile, uint16_t tile_idx);

/**
 * Send a status update to the app.
 */
void ble_send_status(uint8_t battery_pct, uint8_t laser_on, uint8_t fps,
                     int8_t temp_c, uint32_t frame_count);

/**
 * Dequeue a 4-byte command received from the app.
 * @param cmd  4-byte buffer to receive the command
 * @return 0 on success, -1 if no command available
 */
int ble_get_command(uint8_t *cmd);

/**
 * Check if a TX transfer is in progress.
 */
int ble_is_tx_busy(void);

/**
 * USART3 RX interrupt handler (called from USART3_IRQHandler).
 */
void ble_isr_rx(void);

#endif /* SPECKLEFLOW_BLE_H */