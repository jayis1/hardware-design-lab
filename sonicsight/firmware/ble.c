/**
 * @file    ble.c
 * @brief   SonicSight — BLE 5.3 interface implementation.
 *          Communicates with NRF52840 module via HCI UART and SPI data channel.
 *          Implements GATT notification streaming for tomogram data.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include <string.h>
#include <stdio.h>
#include "ble.h"
#include "board.h"
#include "registers.h"

/* ========================================================================== */
/*  Initialisation                                                            */
/* ========================================================================== */

void ble_init(ble_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(ble_ctx_t));

    /* Release NRF52840 from reset */
    HAL_GPIO_WritePin(BLE_RESET_PORT, BLE_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(100); /* Wait for NRF52 boot */

    /* Initialise UART for HCI communication */
    /* MX_USART4_UART_Init() — assumed to be called in main */

    /* Send AT-style initialisation commands to NRF52 */
    const char *init_cmds[] = {
        "AT+BLEINIT=2\r\n",         /* Initialise as peripheral */
        "AT+GAPDEVNAME=SonicSight\r\n",
        "AT+BLEADDR=0\r\n",         /* Use public address */
        "AT+BLEADVDAT=\"SonicSight\"\r\n",
        "AT+GAPROLE=0\r\n",         /* Peripheral role */
        "AT+GATTADDSVC=0xEF44E4A4\r\n", /* Custom service UUID */
        "AT+GATTADDCHAR=0xEF44E4A5,0x12,4\r\n", /* Tomogram char (notify) */
        "AT+GATTADDCHAR=0xEF44E4A6,0x12,10\r\n", /* Status char (notify) */
        "AT+GATTADDCHAR=0xEF44E4A7,0x0A,4\r\n",  /* Command char (write) */
        "AT+GATTADDCHAR=0xEF44E4A8,0x12,20\r\n", /* ToF raw char (notify) */
        "AT+GATTADDCHAR=0xEF44E4A9,0x12,10\r\n", /* Gain report char (notify) */
        "AT+BLEADVSTART\r\n",
        NULL
    };

    for (uint8_t i = 0; init_cmds[i] != NULL; i++) {
        HAL_UART_Transmit(&huart4, (uint8_t *)init_cmds[i],
                          strlen(init_cmds[i]), 100);
        HAL_Delay(50);
    }

    ctx->last_activity_ms = 0;
}

/* ========================================================================== */
/*  Command Parsing — Called from idle loop                                   */
/* ========================================================================== */

void ble_process_commands(ble_ctx_t *ctx)
{
    uint8_t rx_byte;
    static uint8_t cmd_buf[64];
    static uint8_t cmd_idx = 0;

    /* Read available bytes from UART (non-blocking) */
    while (HAL_UART_Receive(&huart4, &rx_byte, 1, 1) == HAL_OK) {
        cmd_buf[cmd_idx++] = rx_byte;
        ctx->last_activity_ms = g_sys_tick_ms;

        if (rx_byte == '\n' || cmd_idx >= sizeof(cmd_buf)) {
            /* Parse command */
            cmd_buf[cmd_idx] = '\0';

            if (strstr((char *)cmd_buf, "CONNECT")) {
                ctx->connected = 1;
            } else if (strstr((char *)cmd_buf, "DISCONNECT")) {
                ctx->connected = 0;
            } else if (strstr((char *)cmd_buf, "CMD=START")) {
                ctx->cmd = BLE_CMD_START_SCAN;
                ctx->cmd_pending = 1;
            } else if (strstr((char *)cmd_buf, "CMD=STOP")) {
                ctx->cmd = BLE_CMD_STOP_SCAN;
                ctx->cmd_pending = 1;
            } else if (strstr((char *)cmd_buf, "CMD=CAL")) {
                ctx->cmd = BLE_CMD_CALIBRATE;
                ctx->cmd_pending = 1;
            } else if (strstr((char *)cmd_buf, "CMD=GAIN")) {
                ctx->cmd = BLE_CMD_SET_GAIN;
                sscanf((char *)cmd_buf, "CMD=GAIN:%f", &ctx->cmd_param_f);
                ctx->cmd_pending = 1;
            } else if (strstr((char *)cmd_buf, "CMD=LAMBDA")) {
                ctx->cmd = BLE_CMD_SET_LAMBDA;
                sscanf((char *)cmd_buf, "CMD=LAMBDA:%f", &ctx->cmd_param_f);
                ctx->custom_lambda = ctx->cmd_param_f;
                ctx->cmd_pending = 1;
            } else if (strstr((char *)cmd_buf, "CMD=STATUS")) {
                ctx->cmd = BLE_CMD_GET_STATUS;
                ctx->cmd_pending = 1;
            }

            cmd_idx = 0;
        }
    }
}

/* ========================================================================== */
/*  Notification Management                                                   */
/* ========================================================================== */

void ble_stop_notifications(ble_ctx_t *ctx)
{
    (void)ctx;
    /* Send stop notification command to NRF52 */
    const char *stop = "AT+NOTIFYSTOP\r\n";
    HAL_UART_Transmit(&huart4, (uint8_t *)stop, strlen(stop), 100);
}

/* ========================================================================== */
/*  Status Notification                                                       */
/* ========================================================================== */

void ble_send_status(ble_ctx_t *ctx, uint32_t scan_count,
                      float temperature, float vel_min, float vel_max,
                      int32_t error_code)
{
    if (!ctx->connected) return;

    char buf[128];
    int len = snprintf(buf, sizeof(buf),
                       "AT+NOTIFY=1,%lu,%.1f,%.1f,%.1f,%ld\r\n",
                       (unsigned long)scan_count,
                       (double)temperature,
                       (double)vel_min,
                       (double)vel_max,
                       (long)error_code);
    HAL_UART_Transmit(&huart4, (uint8_t *)buf, len, 100);
    ctx->notification_seq++;
}

/* ========================================================================== */
/*  Tomogram Data Notification                                                */
/* ========================================================================== */

void ble_send_tomogram(ble_ctx_t *ctx, const float *image, uint32_t grid_size)
{
    if (!ctx->connected) return;
    if (!image) return;

    /* Compress 64×64 float image → 4096 bytes: convert to 8-bit index,
     * then send in chunks of 128 bytes per notification.
     */
    for (uint32_t chunk = 0; chunk < (grid_size * grid_size); chunk += 64) {
        char buf[256];
        int pos = snprintf(buf, sizeof(buf), "AT+NOTIFY=2,%lu:",
                          (unsigned long)chunk);

        /* 64 pixels per chunk, 2 hex chars each → 128 hex chars */
        for (uint32_t i = 0; i < 64 && (chunk + i) < (grid_size * grid_size); i++) {
            float slowness = image[chunk + i];
            float velocity = (slowness > 0.001f) ? (1.0f / slowness) : 0.0f;
            uint8_t idx = 0;
            if (velocity > 4000.0f)      idx = 255;
            else if (velocity < 500.0f)  idx = 0;
            else                         idx = (uint8_t)((velocity - 500.0f) / 13.73f);
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%02X", idx);
        }
        strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);

        HAL_UART_Transmit(&huart4, (uint8_t *)buf, strlen(buf), 200);
        ctx->notification_seq++;

        /* Rate-limit to avoid NRF52 buffer overflow */
        HAL_Delay(5);
    }
}

/* ========================================================================== */
/*  Time-of-Flight Raw Data Notification                                      */
/* ========================================================================== */

void ble_send_tof_raw(ble_ctx_t *ctx, const float (*tof_matrix)[TOMO_MAX_SENSORS],
                       uint32_t num_rays)
{
    if (!ctx->connected) return;
    (void)num_rays;

    /* Send ToF matrix in compact format: transmitter,rx,tof_us per line */
    char buf[256];
    int pos = snprintf(buf, sizeof(buf), "AT+NOTIFY=3,%u", ADC_NUM_CHANNELS);

    for (uint8_t tx = 0; tx < ADC_NUM_CHANNELS; tx++) {
        for (uint8_t rx = tx + 1; rx < ADC_NUM_CHANNELS; rx++) {
            float tof = tof_matrix[tx][rx];
            if (tof > 0.0f && tof < 10000.0f) {
                pos += snprintf(buf + pos, sizeof(buf) - pos, ",%u,%u,%.1f",
                                tx, rx, (double)tof);
                if (pos > 200) {
                    strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
                    HAL_UART_Transmit(&huart4, (uint8_t *)buf, strlen(buf), 200);
                    pos = snprintf(buf, sizeof(buf), "AT+NOTIFY=3,CONT");
                }
            }
        }
    }
    strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
    HAL_UART_Transmit(&huart4, (uint8_t *)buf, strlen(buf), 200);
}

/* ========================================================================== */
/*  Gain/SNR Report                                                           */
/* ========================================================================== */

void ble_send_gain_report(ble_ctx_t *ctx, const float *snr_db,
                           const float *gain_db, uint8_t num_ch)
{
    if (!ctx->connected) return;

    char buf[256];
    int pos = snprintf(buf, sizeof(buf), "AT+NOTIFY=4");

    for (uint8_t ch = 0; ch < num_ch; ch++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        ",%u:%.1f:%.1f", ch,
                        (double)snr_db[ch], (double)gain_db[ch]);
    }
    strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
    HAL_UART_Transmit(&huart4, (uint8_t *)buf, strlen(buf), 200);
}