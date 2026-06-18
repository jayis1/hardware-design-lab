/*
 * comms.h — AeroCast ESP32-C6 communications bridge API
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef AEROCAST_COMMS_H
#define AEROCAST_COMMS_H

#include <stdint.h>

void comms_init(void);
int  comms_wifi_join(const char *ssid, const char *pass);
int  comms_mqtt_connect(const char *broker, uint16_t port);
int  comms_mqtt_publish(const char *topic, const char *payload);
int  comms_ntp_sync(void);
void comms_poll(void);
int  comms_getc(uint8_t *c);
void comms_send_line(const char *s);

uint8_t comms_wifi_connected(void);
uint8_t comms_mqtt_connected_get(void);
uint8_t comms_ble_advertising(void);

void    comms_begin_ota_calib(void);
uint8_t comms_ota_calib_active(void);
void    comms_ota_calib_finish(void);

#endif /* AEROCAST_COMMS_H */