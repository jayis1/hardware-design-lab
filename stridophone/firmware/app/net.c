/*
 * net.c — app-facing network bridge over the ESP32-C6 co-processor.
 *
 * Author : jayis1
 * License: MIT
 *
 * The ESP32-C6 runs an HTTP/WebSocket server (firmware on the ESP side, not
 * shown here) and pulls events from the STM32 over the framed UART protocol.
 * This module formats events as JSON and ships them via esp_coex_send().
 */
#include "net.h"
#include "../drivers/esp_coex.h"
#include "events.h"
#include "../board.h"
#include <stdio.h>
#include <string.h>

static char g_tx_buf[512];

void net_init(void) {
    esp_coex_init();
    esp_coex_boot();
}

int net_wifi_up(void) {
    return esp_wifi_up();
}

int net_push_pending(int max_events) {
    int n = events_pop_to_json(g_tx_buf, sizeof(g_tx_buf), max_events);
    if (n > 0) {
        esp_coex_send(0x10, (const uint8_t *)g_tx_buf,
                      (int)strlen(g_tx_buf));
    }
    return n;
}

int net_push_spectrogram(const float *mag, int nbins) {
    /* Pack 32 log-scaled bins into uint8 (0..255). */
    if (nbins > 32) nbins = 32;
    uint8_t buf[33];
    buf[0] = 0x12;   /* subtype: spectrogram row */
    for (int i = 0; i < nbins; i++) {
        float v = 20.0f * log10f(mag[i] + 1e-6f);  /* dB */
        if (v < 0.0f)    v = 0.0f;
        if (v > 120.0f)  v = 120.0f;
        buf[i+1] = (uint8_t)(v * 2.125f);
    }
    return esp_coex_send(0x12, buf, nbins + 1);
}