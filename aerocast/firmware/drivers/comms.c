/*
 * comms.c — AeroCast ESP32-C6 AT-command bridge (Wi-Fi 6 + BLE 5.3)
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * The STM32 talks to an ESP32-C6 module over USART2 at 2 Mbps using a
 * line-oriented AT command protocol. This file implements:
 *   - Module boot + AT sanity check
 *   - Wi-Fi join (credentials provisioned over BLE)
 *   - MQTT publish/subscribe via the ESP-AT MQTT extension
 *   - BLE GATT service advertising (0xFE5A) for the companion app
 *   - NTP time sync
 *   - A small char-by-char RX buffer consumed by main()'s command loop
 *
 * All blocking waits have timeouts; on failure we fall back to a
 * degraded mode (BLE-only) and keep logging to eMMC.
 */
#ifndef AEROCAST_COMMS_C
#define AEROCAST_COMMS_C
#endif

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "board.h"
#include "registers.h"
#include "comms.h"

#define ESP_RX_BUF_SZ  512
static char    g_rx_buf[ESP_RX_BUF_SZ];
static uint16_t g_rx_head = 0;
static uint8_t  g_wifi_joined = 0;
static uint8_t  g_mqtt_connected = 0;
static uint8_t  g_ble_advertising = 0;

/* ---- Low-level char I/O ---- */
static void esp_send(const char *s) { uart_puts(ESP_UART, s); }
static void esp_send_line(const char *s) { uart_puts(ESP_UART, s); uart_putc(ESP_UART, '\r'); uart_putc(ESP_UART, '\n'); }

static int esp_wait_for(const char *expect, uint32_t timeout_ms)
{
    uint32_t start = millis();
    uint16_t match = 0;
    size_t elen = strlen(expect);
    while ((millis() - start) < timeout_ms) {
        uint8_t c;
        if (uart_getc_nonblocking(ESP_UART, &c)) {
            if (c == (uint8_t)expect[match]) {
                match++;
                if (match == elen) return 1;
            } else {
                match = 0;
            }
        }
    }
    return 0;
}

/* ---- Public API ---- */
void comms_init(void)
{
    uart_init(ESP_UART, 2000000u);
    /* Reset pulse was already done in gpio_board_init(); give it time */
    delay_ms(300);

    /* AT sanity */
    esp_send_line("AT");
    if (!esp_wait_for("OK", 2000u)) {
        /* Module not responsive — operate in offline/BLE-only mode */
        return;
    }
    /* Reset to known state */
    esp_send_line("AT+RST");
    esp_wait_for("ready", 3000u);
    esp_send_line("AT");
    esp_wait_for("OK", 1000u);

    /* Configure BLE (GATT service 0xFE5A) */
    esp_send_line("AT+BLEINIT=2");           /* role: server */
    esp_wait_for("OK", 1000u);
    esp_send_line("AT+BLEGATTSSRVCRE");
    esp_wait_for("OK", 1000u);
    esp_send_line("AT+BLEGATTSSRVSTART");
    esp_wait_for("OK", 1000u);
    esp_send_line("AT+BLEADVSTART");
    if (esp_wait_for("OK", 1000u)) g_ble_advertising = 1;

    /* Try to join Wi-Fi if creds were previously provisioned (stored
     * in ESP NVS). We attempt AT+CIPSTA? to see if already connected. */
    esp_send_line("AT+CIPSTATUS");
    if (esp_wait_for("STATUS:2", 500u) || esp_wait_for("STATUS:3", 500u)) {
        g_wifi_joined = 1;
    }
}

int comms_wifi_join(const char *ssid, const char *pass)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, pass);
    esp_send_line(cmd);
    if (esp_wait_for("WIFI GOT IP", 15000u)) {
        g_wifi_joined = 1;
        return 0;
    }
    return -1;
}

int comms_mqtt_connect(const char *broker, uint16_t port)
{
    if (!g_wifi_joined) return -1;
    char cmd[160];
    /* ESP-AT MQTT extension */
    snprintf(cmd, sizeof(cmd), "AT+MQTTUSERCFG=0,1,\"aerocast\",\"\",\"\",0,0,\"%s\"", broker);
    esp_send_line(cmd);
    if (!esp_wait_for("OK", 2000u)) return -1;
    snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%u,1", broker, port);
    esp_send_line(cmd);
    if (esp_wait_for("+MQTTCONNECTED", 5000u)) {
        g_mqtt_connected = 1;
        return 0;
    }
    return -1;
}

int comms_mqtt_publish(const char *topic, const char *payload)
{
    if (!g_mqtt_connected) return -1;
    char cmd[256];
    size_t plen = strlen(payload);
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"%s\",\"%s\",1,0", topic, payload);
    esp_send_line(cmd);
    return esp_wait_for("OK", 2000u) ? 0 : -1;
}

int comms_ntp_sync(void)
{
    if (!g_wifi_joined) return -1;
    esp_send_line("AT+CIPSNTPCFG=1,0,\"pool.ntp.org\"");
    return esp_wait_for("OK", 3000u) ? 0 : -1;
}

/* Called every 1 s from main loop to drain ESP UART into our RX ring. */
void comms_poll(void)
{
    uint8_t c;
    while (uart_getc_nonblocking(ESP_UART, &c)) {
        if (g_rx_head < ESP_RX_BUF_SZ - 1u) {
            g_rx_buf[g_rx_head++] = (char)c;
        } else {
            /* overflow → drop oldest */
            memmove(g_rx_buf, g_rx_buf + 1, ESP_RX_BUF_SZ - 1u);
            g_rx_buf[ESP_RX_BUF_SZ - 1u] = (char)c;
        }
    }
}

/* main() calls this to pull chars one at a time from the bridge. */
int comms_getc(uint8_t *c)
{
    static uint16_t tail = 0;
    if (tail == g_rx_head) {
        /* reset on drain */
        g_rx_head = 0; tail = 0;
        return 0;
    }
    *c = (uint8_t)g_rx_buf[tail++];
    return 1;
}

void comms_send_line(const char *s)
{
    uart_puts(ESP_UART, s);
    uart_putc(ESP_UART, '\r');
    uart_putc(ESP_UART, '\n');
}

uint8_t comms_wifi_connected(void)   { return g_wifi_joined; }
uint8_t comms_mqtt_connected_get(void) { return g_mqtt_connected; }
uint8_t comms_ble_advertising(void)  { return g_ble_advertising; }

/* Begin an OTA calibration upload session. The companion app streams
 * the new calib.bin over BLE; the firmware buffers it and asks
 * storage.c to commit. This function just flips a flag and notifies
 * the app via a BLE notify. */
static uint8_t g_ota_calib_active = 0;
void comms_begin_ota_calib(void)
{
    g_ota_calib_active = 1;
    comms_send_line("+AEROCAST: OTA_CALIB_BEGIN");
}

uint8_t comms_ota_calib_active(void) { return g_ota_calib_active; }

void comms_ota_calib_finish(void)
{
    g_ota_calib_active = 0;
    comms_send_line("+AEROCAST: OTA_CALIB_DONE");
}