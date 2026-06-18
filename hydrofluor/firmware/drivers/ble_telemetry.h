/*
 * ble_telemetry.h — HydroFluor BLE 5 (BM71) GATT telemetry server
 * Author: jayis1
 * License: MIT
 */
#ifndef BLE_TELEMETRY_H
#define BLE_TELEMETRY_H

#include "board.h"
#include "fluorometry.h"
#include <stdint.h>

/* Initialize USART1 + BM71 module, advertise HydroFluor service. */
void ble_init(void);

/* Notify connected clients of a new sample record. */
void ble_notify_sample(const sample_record_t *rec);

/* Push a calibration result notification. */
void ble_notify_calib(uint8_t analyte, int16_t fit_residual);

/* Process incoming commands from the BLE link (non-blocking poll).
 * Returns a bitmask of commands received this cycle. */
#define BLE_CMD_START         0x01
#define BLE_CMD_STOP          0x02
#define BLE_CMD_PUMP_ON       0x04
#define BLE_CMD_PUMP_OFF      0x08
#define BLE_CMD_CALIBRATE     0x10
#define BLE_CMD_SET_PERIOD    0x20
uint16_t ble_poll(uint16_t *param_period_ms_out);

/* Check if a central is currently connected. */
uint8_t ble_connected(void);

/* Write the device-info characteristic (fw version, serial, cal date). */
void ble_set_device_info(const char *fw, const char *serial, uint32_t cal_date);

#endif /* BLE_TELEMETRY_H */