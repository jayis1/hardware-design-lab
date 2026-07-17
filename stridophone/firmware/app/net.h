/*
 * net.h — push events / spectrogram frames to the app via the ESP32 bridge.
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_NET_H
#define STRIDOPHONE_NET_H

#include <stdint.h>

void net_init(void);

/* True if Wi-Fi is associated (mirror of esp_wifi_up). */
int  net_wifi_up(void);

/* Push up to max_events queued events to the app. Returns count sent. */
int  net_push_pending(int max_events);

/* Push a compact spectrogram row (32 bins, log-scaled) to the app. */
int  net_push_spectrogram(const float *mag, int nbins);

#endif