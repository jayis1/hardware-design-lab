/*
 * wifi_mqtt.h — WiFi + MQTT Client Header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef FERMENTIQ_WIFI_MQTT_H
#define FERMENTIQ_WIFI_MQTT_H

#include "board.h"

/* API */
int wifi_mqtt_init(void);
int wifi_mqtt_publish(const fermentiq_state_t *state);
int wifi_mqtt_subscribe(const char *topic);

#endif /* FERMENTIQ_WIFI_MQTT_H */