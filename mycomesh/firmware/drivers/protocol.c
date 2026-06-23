/*
 * protocol.c — USB/BLE binary protocol for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements a binary command/response protocol over USB CDC for
 * configuration, data download, and real-time monitoring.  The protocol
 * uses a simple frame format:
 *
 *   [SYNC 0xA5] [CMD 1B] [LEN 2B LE] [PAYLOAD N B] [CRC8 1B]
 *
 * Responses follow the same format with SYNC 0x5A for response frames.
 */

#include "protocol.h"
#include "registers.h"
#include <string.h>

/* ===================================================================== */
/*  Protocol constants                                                    */
/* ===================================================================== */

#define PROTO_SYNC_CMD    0xA5
#define PROTO_SYNC_RESP   0x5A
#define PROTO_MAX_PAYLOAD 512

/* CRC-8 polynomial 0x07 (CRC-8/SMBUS). */
static uint8_t proto_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* ===================================================================== */
/*  USB CDC emulation (simplified)                                        */
/* ===================================================================== */

/* In a full implementation, this would use the STM32 USB CDC driver
   to communicate over USB.  Here we provide a simplified interface
   using UART4 (debug port) as the transport, which allows testing
   the protocol without a full USB stack. */

static uint8_t  g_rx_buffer[PROTO_MAX_PAYLOAD + 8];
static uint16_t g_rx_pos = 0;
static uint8_t  g_rx_state = 0;  /* 0=idle, 1=got sync, 2=got cmd, 3=got len */

static uint8_t usb_rx_byte(void)
{
    while (!(UART4->ISR & USART_ISR_RXNE));
    return (uint8_t)UART4->RDR;
}

static void usb_tx_byte(uint8_t b)
{
    while (!(UART4->ISR & USART_ISR_TXE));
    UART4->TDR = b;
}

static void usb_tx_buffer(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        usb_tx_byte(data[i]);
    }
}

/* ===================================================================== */
/*  Response sender                                                       */
/* ===================================================================== */

static void send_response(uint8_t cmd, const uint8_t *payload, uint16_t len)
{
    uint8_t header[4];
    header[0] = PROTO_SYNC_RESP;
    header[1] = cmd;
    header[2] = (uint8_t)(len & 0xFF);
    header[3] = (uint8_t)(len >> 8);

    /* CRC over header[1..3] + payload. */
    uint8_t crc = proto_crc8(&header[1], 3);
    crc = proto_crc8(payload);  /* simplified: would be cumulative */

    usb_tx_buffer(header, 4);
    if (len > 0) usb_tx_buffer(payload, len);
    usb_tx_byte(crc);
}

static void send_ack(uint8_t cmd)
{
    send_response(cmd, NULL, 0);
}

static void send_nack(uint8_t cmd, uint8_t error)
{
    send_response(cmd, &error, 1);
}

/* ===================================================================== */
/*  Command handlers                                                      */
/* ===================================================================== */

/* External functions from main.c */
extern void myco_usb_connect_callback(void);
extern void myco_usb_disconnect_callback(void);
extern void myco_apply_config(uint8_t duty_cycle, uint8_t mains_hz, uint8_t region);

/* Forward declarations for drivers we need. */
extern uint16_t dsp_get_noise_mad(uint8_t channel);
extern uint8_t  classifier_label(uint8_t class_label);

static void handle_get_status(void)
{
    /* Build a 16-byte status response. */
    uint8_t status[16];
    memset(status, 0, sizeof(status));

    /* Bytes 0-3: uptime (would be from RTC/tick). */
    /* Bytes 4-5: battery voltage (would be from power_read_battery_mv). */
    status[4] = 0x0C;  /* 3000 mV low byte (placeholder) */
    status[5] = 0x0B;  /* 3000 mV high byte */

    /* Byte 6: current state. */
    /* Byte 7: duty cycle %. */
    status[7] = 5;

    /* Bytes 8-9: channels active. */
    status[8] = ADS1298_TOTAL_CHANS;

    /* Bytes 10-13: epoch class label + spike count. */
    status[10] = CLASS_IDLE;

    /* Bytes 14-15: sample rate. */
    status[14] = (uint8_t)(ADS1298_SAMPLE_RATE & 0xFF);
    status[15] = (uint8_t)(ADS1298_SAMPLE_RATE >> 8);

    send_response(PROTO_CMD_GET_STATUS, status, sizeof(status));
}

static void handle_start_acq(const uint8_t *payload, uint16_t len)
{
    /* Payload: 4-byte config (sample_rate, gain, filters, reserved). */
    if (len < 4) {
        send_nack(PROTO_CMD_START_ACQ, 0x01);  /* too short */
        return;
    }
    /* Would configure ADS1298 with the provided parameters. */
    send_ack(PROTO_CMD_START_ACQ);
}

static void handle_stop_acq(void)
{
    /* Would stop ADS1298 acquisition. */
    send_ack(PROTO_CMD_STOP_ACQ);
}

static void handle_set_config(const uint8_t *payload, uint16_t len)
{
    if (len < 3) {
        send_nack(PROTO_CMD_SET_CONFIG, 0x01);
        return;
    }
    /* Payload: [duty_cycle, mains_hz, region]. */
    myco_apply_config(payload[0], payload[1], payload[2]);
    send_ack(PROTO_CMD_SET_CONFIG);
}

static void handle_get_env(void)
{
    /* Build a 12-byte environmental data response. */
    uint8_t env_data[12];
    memset(env_data, 0, sizeof(env_data));

    /* Bytes 0-1: soil moisture × 10. */
    env_data[0] = 0x5E;  /* 350 = 35.0% (placeholder) */
    env_data[1] = 0x01;

    /* Bytes 2-3: soil temperature × 10. */
    env_data[2] = 0xF6;  /* 220 = 22.0°C (placeholder) */
    env_data[3] = 0x00;

    /* Bytes 4-5: CO₂ ppm. */
    env_data[4] = 0x90;  /* 400 ppm baseline */
    env_data[5] = 0x01;

    /* Bytes 6-7: humidity %. */
    env_data[6] = 50;

    /* Bytes 8-11: timestamp. */
    env_data[8] = 0;

    send_response(PROTO_CMD_GET_ENV, env_data, sizeof(env_data));
}

static void handle_calibrate(const uint8_t *payload, uint16_t len)
{
    if (len < 1) {
        send_nack(PROTO_CMD_CALIBRATE, 0x01);
        return;
    }
    /* Payload: calibration type (0=offset, 1=electrode impedance). */
    uint8_t result[4] = {0, 0, 0, 0};
    send_response(PROTO_CMD_CALIBRATE, result, sizeof(result));
}

static void handle_dfu_enter(void)
{
    /* Would trigger USB DFU mode for firmware update. */
    send_ack(PROTO_CMD_DFU_ENTER);
    /* In production: enter DFU bootloader. */
}

static void handle_get_log_list(void)
{
    /* Return a list of log files.  In this simplified version, we
       return a single file entry. */
    uint8_t list[16];
    memset(list, 0, sizeof(list));
    list[0] = 1;  /* 1 file */
    /* File 0: name = "MYCO_001.BIN", size = 0 (placeholder). */
    memcpy(&list[1], "MYCO_001.BIN", 12);
    send_response(PROTO_CMD_GET_LOG_LIST, list, sizeof(list));
}

static void handle_download_log(const uint8_t *payload, uint16_t len)
{
    if (len < 4) {
        send_nack(PROTO_CMD_DOWNLOAD_LOG, 0x01);
        return;
    }
    /* Payload: 4-byte file ID. */
    /* Would stream the file over USB in chunks. */
    /* For now, send a small placeholder chunk. */
    uint8_t chunk[64];
    memset(chunk, 0, sizeof(chunk));
    chunk[0] = 0x4D;  /* 'M' */
    chunk[1] = 0x43;  /* 'C' */
    chunk[2] = 0x4D;  /* 'M' */
    chunk[3] = 0x53;  /* 'S' */
    send_response(PROTO_CMD_DOWNLOAD_LOG, chunk, sizeof(chunk));
}

/* ===================================================================== */
/*  Public API                                                            */
/* ===================================================================== */

void protocol_init(void)
{
    g_rx_pos = 0;
    g_rx_state = 0;
}

void protocol_poll(void)
{
    /* Check if a byte is available on UART4 (used as USB CDC proxy). */
    if (!(UART4->ISR & USART_ISR_RXNE)) return;

    uint8_t byte = (uint8_t)UART4->RDR;

    switch (g_rx_state) {
    case 0:  /* idle — waiting for sync byte */
        if (byte == PROTO_SYNC_CMD) {
            g_rx_state = 1;
            g_rx_pos = 0;
        }
        break;

    case 1:  /* got sync — read command byte */
        g_rx_buffer[0] = byte;
        g_rx_state = 2;
        break;

    case 2:  /* got cmd — read length (2 bytes LE) */
        g_rx_buffer[1 + g_rx_pos++] = byte;
        if (g_rx_pos >= 2) {
            g_rx_state = 3;
        }
        break;

    case 3:  /* got length — read payload + CRC */
        {
            uint16_t payload_len = g_rx_buffer[1] | (g_rx_buffer[2] << 8);
            if (payload_len > PROTO_MAX_PAYLOAD) {
                /* Frame too large — reset. */
                g_rx_state = 0;
                break;
            }
            g_rx_buffer[3 + (g_rx_pos - 2)] = byte;
            g_rx_pos++;

            /* Check if we've received the full payload + CRC. */
            if (g_rx_pos >= (uint16_t)(2 + payload_len + 1)) {
                /* Verify CRC. */
                uint8_t expected_crc = proto_crc8(&g_rx_buffer[0], 3 + payload_len);
                uint8_t received_crc = g_rx_buffer[3 + payload_len];

                if (expected_crc != received_crc) {
                    send_nack(g_rx_buffer[0], 0x02);  /* CRC error */
                    g_rx_state = 0;
                    break;
                }

                /* Dispatch command. */
                uint8_t cmd = g_rx_buffer[0];
                const uint8_t *payload = &g_rx_buffer[3];
                uint16_t plen = payload_len;

                switch (cmd) {
                case PROTO_CMD_GET_STATUS:
                    handle_get_status();
                    break;
                case PROTO_CMD_START_ACQ:
                    handle_start_acq(payload, plen);
                    break;
                case PROTO_CMD_STOP_ACQ:
                    handle_stop_acq();
                    break;
                case PROTO_CMD_SET_CONFIG:
                    handle_set_config(payload, plen);
                    break;
                case PROTO_CMD_GET_ENV:
                    handle_get_env();
                    break;
                case PROTO_CMD_CALIBRATE:
                    handle_calibrate(payload, plen);
                    break;
                case PROTO_CMD_DFU_ENTER:
                    handle_dfu_enter();
                    break;
                case PROTO_CMD_GET_LOG_LIST:
                    handle_get_log_list();
                    break;
                case PROTO_CMD_DOWNLOAD_LOG:
                    handle_download_log(payload, plen);
                    break;
                default:
                    send_nack(cmd, 0x03);  /* unknown command */
                    break;
                }

                g_rx_state = 0;
            }
        }
        break;

    default:
        g_rx_state = 0;
        break;
    }
}

/* ===================================================================== */
/*  Data senders                                                          */
/* ===================================================================== */

void protocol_send_status(const epoch_summary_t *epoch)
{
    send_response(PROTO_CMD_GET_STATUS, (const uint8_t *)epoch,
                  sizeof(epoch_summary_t));
}

void protocol_send_calibration(int16_t *samples, uint16_t count)
{
    /* Send raw calibration data as a stream of 16-bit samples.
       Chunked to fit within PROTO_MAX_PAYLOAD. */
    uint16_t offset = 0;
    while (offset < count) {
        uint16_t chunk = count - offset;
        if (chunk > 250) chunk = 250;  /* limit to 250 samples (500 bytes) */

        send_response(0x10, (const uint8_t *)&samples[offset],
                      chunk * sizeof(int16_t));
        offset += chunk;
    }
}

void protocol_send_env(const env_data_t *env)
{
    send_response(PROTO_CMD_GET_ENV, (const uint8_t *)env, sizeof(env_data_t));
}

void protocol_send_spikes(const spike_event_t *spikes, uint16_t count)
{
    uint16_t offset = 0;
    while (offset < count) {
        uint16_t chunk = count - offset;
        uint16_t max_chunk = PROTO_MAX_PAYLOAD / sizeof(spike_event_t);
        if (chunk > max_chunk) chunk = max_chunk;

        send_response(0x11, (const uint8_t *)&spikes[offset],
                      chunk * sizeof(spike_event_t));
        offset += chunk;
    }
}