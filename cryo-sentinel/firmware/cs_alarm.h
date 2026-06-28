/*
 * cs_alarm.h — Alarm state machine for Cryo-Sentinel.
 *
 * Author: jayis1
 * License: MIT
 */
#ifndef CRYO_SENTINEL_CS_ALARM_H
#define CRYO_SENTINEL_CS_ALARM_H

#include <stdbool.h>
#include <stdint.h>
#include "cs_sensor.h"

typedef enum {
    CS_STATE_OK       = 0,
    CS_STATE_WATCH    = 1,
    CS_STATE_WARN     = 2,
    CS_STATE_CRITICAL = 3
} cs_alarm_state_t;

/* Reason codes that drive transitions. */
typedef enum {
    CS_REASON_NONE            = 0,
    CS_REASON_LEVEL_WARN      = 1,
    CS_REASON_LEVEL_CRIT      = 2,
    CS_REASON_LEVEL_RATE      = 3,
    CS_REASON_GRADIENT        = 4,
    CS_REASON_LID_OPEN        = 5,
    CS_REASON_TILT_WARN       = 6,
    CS_REASON_TILT_CRIT       = 7,
    CS_REASON_AMBIENT_HOT     = 8,
    CS_REASON_MAINS_LOST      = 9,
    CS_REASON_BATT_CRIT       = 10,
    CS_REASON_ENC_OPEN_WARN   = 11,
    CS_REASON_PROBE_GONE      = 12,
    CS_REASON_SENSOR_FAULT    = 13
} cs_reason_t;

typedef struct {
    cs_alarm_state_t state;
    cs_reason_t      reason;
    uint32_t         state_entered_min;   /* epoch minute of entry */
    uint32_t         last_alarm_sent_min; /* for repeat pacing */
    bool             acked;
    uint32_t         acked_by;            /* technician id from app */
    uint32_t         lid_open_since_min;
    uint32_t         enclosure_open_since_min;
    uint32_t         mains_lost_since_min;
} cs_alarm_t;

/* Initialize the alarm context. */
void cs_alarm_init(cs_alarm_t *a);

/* Evaluate the latest reading and advance the state machine. Returns the
 * new state and fills *reason_out if a transition happened. */
cs_alarm_state_t cs_alarm_eval(cs_alarm_t *a,
                               const cs_reading_t *r,
                               float batt_pct,
                               uint32_t now_min,
                               cs_reason_t *reason_out);

/* Acknowledge the current alarm (from the app, via cloud or BLE). */
void cs_alarm_ack(cs_alarm_t *a, uint32_t technician_id);

/* Decide whether a repeat alarm send is due. */
bool cs_alarm_repeat_due(const cs_alarm_t *a, uint32_t now_min);

/* Human-readable label for a reason code (for the audit log). */
const char *cs_reason_label(cs_reason_t r);

/* Human-readable label for a state. */
const char *cs_state_label(cs_alarm_state_t s);

#endif /* CRYO_SENTINEL_CS_ALARM_H */