/*
 * cs_radio.h — Network-core radio dispatch (BLE + LTE-M/NB-IoT).
 *
 * The app core never touches the radios directly. It posts IPC messages to
 * the network core, which owns the BLE stack and the BG770A UART.
 *
 * Author: jayis1
 * License: MIT
 */
#ifndef CRYO_SENTINEL_CS_RADIO_H
#define CRYO_SENTINEL_CS_RADIO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cs_sensor.h"
#include "cs_alarm.h"

/* Heartbeat payload sent to the cloud every 15 min. */
typedef struct {
    uint32_t          seq;            /* monotonic, mirrors FRAM seq */
    uint32_t          epoch_min;      /* time of heartbeat */
    cs_alarm_state_t  state;
    cs_reason_t       reason;
    float             level_pct;
    float             level_rate_ph;
    float             rtd_top_degC;
    float             rtd_mid_degC;
    float             rtd_bot_degC;
    float             gradient_slope;
    float             tilt_deg;
    float             vib_rms_g;
    float             ambient_degC;
    float             ambient_rh;
    float             batt_pct;
    bool              mains_present;
    bool              lid_open;
    bool              enclosure_open;
} cs_heartbeat_t;

/* Alarm payload (Tier-1 or Tier-2). */
typedef struct {
    uint32_t          seq;
    uint32_t          epoch_min;
    uint8_t           tier;           /* 1 or 2 */
    cs_alarm_state_t  state;
    cs_reason_t       reason;
    float             level_pct;      /* the offending reading */
    float             aux;            /* tilt, rate, or temp per reason */
    char              dewar_serial[24];
} cs_alarm_msg_t;

/* Initialize the IPC channel and the network core RPC. */
int cs_radio_init(void);

/* Send a heartbeat over LTE-M/NB-IoT. Returns 0 on success. */
int cs_radio_send_heartbeat(const cs_heartbeat_t *hb);

/* Send an alarm over LTE-M/NB-IoT (and BLE local if paired). */
int cs_radio_send_alarm(const cs_alarm_msg_t *m);

/* Send an acknowledgement notification up to the cloud. */
int cs_radio_send_ack(uint32_t seq, uint32_t technician_id);

/* BLE-only operations (during commissioning). */
int cs_radio_ble_pair_start(void);
int cs_radio_ble_pair_stop(void);
int cs_radio_ble_cal_push(const uint8_t *cal_blob, size_t len);

/* Power down the cellular modem entirely (between heartbeats). */
void cs_radio_cell_sleep(void);
/* Power up and wait for RDY (up to BG770A_RDY_TIMEOUT_MS). */
int  cs_radio_cell_wake(void);

/* Poll for any incoming IPC (e.g. ack from app/cloud bridge).
 * Returns true and fills *op / *payload if a message arrived. */
bool cs_radio_poll_incoming(uint8_t *op, uint8_t *payload, size_t *len);

#endif /* CRYO_SENTINEL_CS_RADIO_H */