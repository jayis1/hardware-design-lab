/*
 * cs_alarm.c — Alarm state machine for Cryo-Sentinel.
 *
 * The state machine is explicit and monotone-ish: WARN and CRITICAL cannot
 * be cleared without (a) the offending invariant returning to tolerance and
 * (b) an explicit ack from a technician. WATCH is auto-cleared when the
 * invariant recovers.
 *
 * Author: jayis1
 * License: MIT
 */
#include "cs_alarm.h"
#include "board.h"
#include <string.h>

void cs_alarm_init(cs_alarm_t *a)
{
    memset(a, 0, sizeof(*a));
    a->state = CS_STATE_OK;
    a->reason = CS_REASON_NONE;
}

/* Decide the worst-case state implied by a reading, independent of hysteresis
 * and ack. Returns the candidate state and a reason. */
static cs_alarm_state_t classify(const cs_reading_t *r, float batt_pct,
                                 uint32_t now_min, cs_alarm_t *a,
                                 cs_reason_t *reason)
{
    /* Critical conditions. */
    if (r->level_ok && r->level_pct >= 0.0f &&
        r->level_pct < CS_LEVEL_CRIT_PCT) {
        *reason = CS_REASON_LEVEL_CRIT; return CS_STATE_CRITICAL;
    }
    if (r->level_ok && r->level_rate_ph > CS_LEVEL_RATE_CRIT_PH) {
        *reason = CS_REASON_LEVEL_RATE; return CS_STATE_CRITICAL;
    }
    if (r->imu_ok && r->tilt_deg > CS_TILT_CRIT_DEG) {
        *reason = CS_REASON_TILT_CRIT; return CS_STATE_CRITICAL;
    }
    if (batt_pct < CS_BATT_CRIT_PCT) {
        *reason = CS_REASON_BATT_CRIT; return CS_STATE_CRITICAL;
    }
    if (!r->probe_present) {
        *reason = CS_REASON_PROBE_GONE; return CS_STATE_CRITICAL;
    }

    /* Warn conditions. */
    if (r->level_ok && r->level_pct >= 0.0f &&
        r->level_pct < CS_LEVEL_WARN_PCT) {
        *reason = CS_REASON_LEVEL_WARN; return CS_STATE_WARN;
    }
    if (r->imu_ok && r->tilt_deg > CS_TILT_WARN_DEG) {
        *reason = CS_REASON_TILT_WARN; return CS_STATE_WARN;
    }
    if (r->lid_open) {
        if (a->lid_open_since_min == 0) a->lid_open_since_min = now_min;
        if ((now_min - a->lid_open_since_min) >= CS_LID_OPEN_WARN_MIN) {
            *reason = CS_REASON_LID_OPEN; return CS_STATE_WARN;
        }
    } else {
        a->lid_open_since_min = 0;
    }
    if (r->enclosure_open && a->state >= CS_STATE_WARN) {
        *reason = CS_REASON_ENC_OPEN_WARN; return CS_STATE_WARN;
    }

    /* Watch conditions. */
    if (r->ambient_ok && r->ambient_degC > CS_AMBIENT_WATCH_C) {
        *reason = CS_REASON_AMBIENT_HOT; return CS_STATE_WATCH;
    }
    if (!r->mains_present) {
        *reason = CS_REASON_MAINS_LOST; return CS_STATE_WATCH;
    }
    if (r->level_ok && r->rtd_ok[0] && r->rtd_ok[2]) {
        /* A flattening gradient (top - bottom narrowing) hints at accel boil-off. */
        if (r->gradient_slope < 80.0f) {
            *reason = CS_REASON_GRADIENT; return CS_STATE_WATCH;
        }
    }
    if (!r->level_ok || !r->imu_ok || !r->ambient_ok) {
        *reason = CS_REASON_SENSOR_FAULT; return CS_STATE_WATCH;
    }

    *reason = CS_REASON_NONE;
    return CS_STATE_OK;
}

cs_alarm_state_t cs_alarm_eval(cs_alarm_t *a,
                               const cs_reading_t *r,
                               float batt_pct,
                               uint32_t now_min,
                               cs_reason_t *reason_out)
{
    cs_reason_t      r_new = CS_REASON_NONE;
    cs_alarm_state_t s_new = classify(r, batt_pct, now_min, a, &r_new);

    /* Track enclosure + mains timers. */
    if (r->enclosure_open) {
        if (a->enclosure_open_since_min == 0) a->enclosure_open_since_min = now_min;
    } else {
        a->enclosure_open_since_min = 0;
    }
    if (!r->mains_present) {
        if (a->mains_lost_since_min == 0) a->mains_lost_since_min = now_min;
    } else {
        a->mains_lost_since_min = 0;
    }

    /* Monotone escalation: never drop below the current state unless acked. */
    if (s_new > a->state) {
        a->state          = s_new;
        a->reason         = r_new;
        a->state_entered_min = now_min;
        a->acked          = false;
        a->acked_by       = 0;
        if (reason_out) *reason_out = r_new;
        return a->state;
    }

    /* If the invariant recovered and we're acked, de-escalate. */
    if (s_new < a->state && a->acked) {
        a->state          = s_new;
        a->reason         = r_new;
        a->state_entered_min = now_min;
        a->acked          = false;
        if (reason_out) *reason_out = CS_REASON_NONE;
        return a->state;
    }

    /* Same state, but reason may refine. */
    if (s_new == a->state && r_new != CS_REASON_NONE) {
        a->reason = r_new;
    }
    if (reason_out) *reason_out = CS_REASON_NONE;
    return a->state;
}

void cs_alarm_ack(cs_alarm_t *a, uint32_t technician_id)
{
    a->acked     = true;
    a->acked_by  = technician_id;
}

bool cs_alarm_repeat_due(const cs_alarm_t *a, uint32_t now_min)
{
    if (a->state < CS_STATE_WARN) return false;
    uint32_t interval = (a->state == CS_STATE_CRITICAL)
                        ? CS_ALARM_REPEAT_MIN
                        : (CS_ALARM_REPEAT_MIN * 4);  /* WARN repeats every 20 min */
    if (a->last_alarm_sent_min == 0) return true;
    return (now_min - a->last_alarm_sent_min) >= interval;
}

const char *cs_reason_label(cs_reason_t r)
{
    switch (r) {
    case CS_REASON_NONE:          return "none";
    case CS_REASON_LEVEL_WARN:    return "level_low_warn";
    case CS_REASON_LEVEL_CRIT:    return "level_low_crit";
    case CS_REASON_LEVEL_RATE:    return "level_drop_rate";
    case CS_REASON_GRADIENT:      return "gradient_flat";
    case CS_REASON_LID_OPEN:      return "lid_open";
    case CS_REASON_TILT_WARN:     return "tilt_warn";
    case CS_REASON_TILT_CRIT:     return "tilt_crit";
    case CS_REASON_AMBIENT_HOT:   return "ambient_hot";
    case CS_REASON_MAINS_LOST:    return "mains_lost";
    case CS_REASON_BATT_CRIT:     return "battery_crit";
    case CS_REASON_ENC_OPEN_WARN: return "enclosure_open";
    case CS_REASON_PROBE_GONE:    return "probe_removed";
    case CS_REASON_SENSOR_FAULT:  return "sensor_fault";
    default:                      return "unknown";
    }
}

const char *cs_state_label(cs_alarm_state_t s)
{
    switch (s) {
    case CS_STATE_OK:       return "OK";
    case CS_STATE_WATCH:    return "WATCH";
    case CS_STATE_WARN:     return "WARN";
    case CS_STATE_CRITICAL: return "CRITICAL";
    default:                return "?";
    }
}