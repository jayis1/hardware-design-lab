/**
 * @file    ble.h
 * @brief   SonicSight — BLE 5.3 interface (NRF52840 module).
 *          GATT service definitions, data streaming, command parsing.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef BLE_H
#define BLE_H

#include <stdint.h>
#include "board.h"

/** BLE command opcodes received from companion app */
#define BLE_CMD_START_SCAN    (0x01U)  /**< Begin full acquisition cycle */
#define BLE_CMD_STOP_SCAN     (0x02U)  /**< Abort current scan */
#define BLE_CMD_CALIBRATE     (0x03U)  /**< Run calibration routine */
#define BLE_CMD_SET_GAIN      (0x04U)  /**< Set VGA gain (dB) */
#define BLE_CMD_SET_FILTER    (0x05U)  /**< Set filter cutoff (Hz) */
#define BLE_CMD_SET_LAMBDA    (0x06U)  /**< Set Tikhonov lambda */
#define BLE_CMD_SET_GRID      (0x07U)  /**< Set reconstruction grid size */
#define BLE_CMD_GET_STATUS    (0x08U)  /**< Request device status */
#define BLE_CMD_GET_VERSION   (0x09U)  /**< Request firmware version */
#define BLE_CMD_RESET         (0x0AU)  /**< Soft reset device */
#define BLE_CMD_SET_SENSORS   (0x0BU)  /**< Set active sensor count */
#define BLE_CMD_SEND_GEOMETRY (0x0CU)  /**< Send sensor positions from app */

/** BLE context */
typedef struct {
    uint8_t  connected;                 /**< 1 if BLE link is active */
    uint8_t  cmd_pending;               /**< 1 if a command is pending */
    uint8_t  cmd;                       /**< Most recent command opcode */
    float    cmd_param_f;               /**< Float parameter for command */
    int32_t  cmd_param_i;               /**< Integer parameter for command */
    float    custom_lambda;             /**< User-set lambda (0 = use default) */
    uint32_t notification_seq;          /**< Notification sequence counter */
    uint8_t  tx_pending;                /**< 1 if a TX operation is in progress */
    uint32_t last_activity_ms;          /**< Last BLE activity timestamp (ms) */
} ble_ctx_t;

/**
 * @brief Initialise BLE subsystem (reset NRF52, configure HCI UART).
 * @param ctx BLE context.
 */
void ble_init(ble_ctx_t *ctx);

/**
 * @brief Process incoming BLE commands (called from idle loop).
 * @param ctx BLE context (updated with parsed commands).
 */
void ble_process_commands(ble_ctx_t *ctx);

/**
 * @brief Stop any active BLE notifications.
 * @param ctx BLE context.
 */
void ble_stop_notifications(ble_ctx_t *ctx);

/**
 * @brief Send device status notification.
 * @param ctx          BLE context.
 * @param scan_count   Cumulative scan count.
 * @param temperature  Current temperature (°C).
 * @param vel_min      Minimum velocity in image (m/s).
 * @param vel_max      Maximum velocity in image (m/s).
 * @param error_code   Current error code (0 = OK).
 */
void ble_send_status(ble_ctx_t *ctx, uint32_t scan_count,
                      float temperature, float vel_min, float vel_max,
                      int32_t error_code);

/**
 * @brief Send compressed tomogram image.
 * @param ctx       BLE context.
 * @param image     Slowness image buffer (row-major).
 * @param grid_size Grid dimension.
 */
void ble_send_tomogram(ble_ctx_t *ctx, const float *image, uint32_t grid_size);

/**
 * @brief Send raw time-of-flight matrix.
 * @param ctx       BLE context.
 * @param tof_matrix ToF matrix (ToF_MAX_SENSORS × ToF_MAX_SENSORS).
 * @param num_rays  Number of valid ray paths.
 */
void ble_send_tof_raw(ble_ctx_t *ctx, const float (*tof_matrix)[TOMO_MAX_SENSORS],
                       uint32_t num_rays);

/**
 * @brief Send per-channel gain and SNR report.
 * @param ctx        BLE context.
 * @param snr_db     Array of per-channel SNR values (dB).
 * @param gain_db    Array of per-channel gain values (dB).
 * @param num_ch     Number of ADC channels.
 */
void ble_send_gain_report(ble_ctx_t *ctx, const float *snr_db,
                           const float *gain_db, uint8_t num_ch);

#endif /* BLE_H */