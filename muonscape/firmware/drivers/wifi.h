/*
 * wifi.h — ATWINC1500 Wi-Fi driver interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_WIFI_H
#define MUONSCAPE_DRV_WIFI_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_MODE_IDLE = 0,
    WIFI_MODE_STA,
    WIFI_MODE_AP
} wifi_mode_t;

muon_status_t wifi_init(void);
muon_status_t wifi_connect(const char *ssid, const char *pass);
muon_status_t wifi_start_ap(const char *ssid, const char *pass);
muon_status_t wifi_disconnect(void);
int           wifi_is_connected(void);
int32_t       wifi_rssi_dbm(void);

/* TCP server on port WIFI_PORT. Accepts a single client at a time. */
muon_status_t wifi_tcp_listen(void);
int           wifi_tcp_client_connected(void);
int32_t       wifi_tcp_recv(uint8_t *buf, uint32_t max, uint32_t timeout_ms);
int32_t       wifi_tcp_send(const uint8_t *buf, uint32_t len);
void          wifi_tcp_close(void);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_WIFI_H */