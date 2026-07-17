/*
 * esp_coex.h — ESP32-C6 co-processor bridge over USART3.
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_ESP_COEX_H
#define STRIDOPHONE_ESP_COEX_H

#include <stdint.h>

void esp_coex_init(void);

/* Reset the ESP32-C6 into run mode and wait for its "hello" frame. */
int  esp_coex_boot(void);

/* Send a framed message (type byte + payload). */
int  esp_coex_send(uint8_t type, const uint8_t *payload, int len);

/* Poll for an incoming frame (non-blocking). Returns len or 0. */
int  esp_coex_recv(uint8_t *type, uint8_t *buf, int maxlen);

/* Query whether Wi-Fi is associated (set by ESP via a status frame). */
int  esp_wifi_up(void);

#endif