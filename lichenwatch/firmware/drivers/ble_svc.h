/*
 * ble_svc.h — BLE GATT service for walk-by download (nRF52840 link over UART).
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef BLE_SVC_H
#define BLE_SVC_H

#include <stdint.h>

typedef struct {
    float    fv_fm;
    float    lndvi;
    float    chl_index;
    float    wetness;
    uint8_t  class_state;
    uint8_t  conf;
    uint16_t battery_mv;
    uint16_t seq;
    uint32_t uptime_s;
} ble_status_t;

typedef enum {
    BLE_EVENT_CONNECT    = 1,
    BLE_EVENT_DISCONNECT = 2,
    BLE_EVENT_READ_STATUS = 3,
    BLE_EVENT_BULK_REQ    = 4,
    BLE_EVENT_CONFIG      = 5,
} ble_event_type_t;

typedef struct {
    ble_event_type_t type;
    uint8_t  data[16];
    uint8_t  len;
} ble_event_t;

int ble_init(void);
int ble_advertise_start(const char *node_id);
int ble_stop_advertising(void);
int ble_poll_event(ble_event_t *evt, uint32_t timeout_ms);
int ble_send_status(const ble_status_t *st);
int ble_send_bulk_record(const void *rec);
int ble_send_bulk_done(void);

#endif /* BLE_SVC_H */