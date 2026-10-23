/* SPDX-License-Identifier: MIT
 * QuadraAlign peripheral register map
 * Author: jayis1
 */
#ifndef QUADRALIGN_REGISTERS_H
#define QUADRALIGN_REGISTERS_H

/* ADS1234 four-channel 24-bit load-cell ADC command/configuration. */
#define ADS1234_GAIN_128          0x03U
#define ADS1234_RATE_10_SPS       0x00U
#define ADS1234_RATE_80_SPS       0x01U
#define ADS1234_SETTLE_SAMPLES    8U
#define ADS1234_TIMEOUT_MS        20U

/* LIS2DW12 low-power accelerometer. */
#define LIS2DW12_ADDRESS          0x19U
#define LIS2DW12_WHO_AM_I         0x0FU
#define LIS2DW12_WHO_AM_I_VALUE   0x44U
#define LIS2DW12_CTRL1            0x20U
#define LIS2DW12_CTRL2            0x21U
#define LIS2DW12_CTRL3            0x22U
#define LIS2DW12_CTRL4_INT1_PAD   0x23U
#define LIS2DW12_CTRL6            0x25U
#define LIS2DW12_STATUS           0x27U
#define LIS2DW12_OUT_X_L          0x28U
#define LIS2DW12_FIFO_CTRL        0x2EU
#define LIS2DW12_ODR_200HZ        0x60U
#define LIS2DW12_MODE_HIGH_PERF   0x04U
#define LIS2DW12_FS_2G            0x00U
#define LIS2DW12_BDU              0x08U
#define LIS2DW12_SOFT_RESET       0x40U
#define LIS2DW12_IF_ADD_INC       0x04U
#define LIS2DW12_SAMPLE_MG_LSB    0.244f

/* Raytac MDBT50Q (nRF52840) modem framing over UART. */
#define BLE_SOF                   0xA5U
#define BLE_TYPE_TELEMETRY        0x01U
#define BLE_TYPE_EVENT            0x02U
#define BLE_TYPE_COMMAND          0x10U
#define BLE_TYPE_ACK              0x11U
#define BLE_MAX_PAYLOAD           160U
#define BLE_CMD_TARE              0x01U
#define BLE_CMD_CAPTURE           0x02U
#define BLE_CMD_SET_LIMITS        0x03U
#define BLE_CMD_CALIBRATE         0x04U
#define BLE_CMD_SLEEP             0x05U

#endif
