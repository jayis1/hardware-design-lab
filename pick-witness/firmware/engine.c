/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <string.h>
#include <limits.h>
static uint32_t add_sat(uint32_t a, uint32_t b)
{
    if (UINT32_MAX-a < b)
        return UINT32_MAX;
    return a+b;
}
static void count(Engine *e, uint32_t *value)
{
    if (*value == UINT32_MAX) {
        e->saturated=true;
        e->reason=COUNTER_LIMIT;
        return;
    }
    ++*value;
}
static void event(Engine *e, Reason why)
{
    e->reason=why;
    count(e,&e->events);
}
static void anomaly(Engine *e, Reason why)
{
    count(e,&e->anomalies);
    event(e,why);
}
static void invalidate(Engine *e)
{
    e->pending=false;
    e->departed=false;
    e->ready=false;
    e->origin=0;
    e->have_pick=false;
    e->pending_age=0;
    e->flight_age=0;
}
void engine_init(Engine *e, uint32_t now)
{
    memset(e,0,sizeof *e);
    e->now=now;
    e->min_interval=UINT32_MAX;
}
void engine_transport_loss(Engine *e)
{
    count(e,&e->transport_losses);
}
static uint8_t dock(uint8_t state)
{
    return state & (LEFT_DOCK|RIGHT_DOCK);
}
static uint8_t shed(uint8_t state)
{
    return (state & SHED) ? 1u : 0u;
}
static void anchor(Engine *e, uint8_t state)
{
    uint8_t d=dock(state);
    /* A dock plus released beater is the only safe synchronization pose. */
    if ((d==LEFT_DOCK || d==RIGHT_DOCK) && !(state & BEATER)) {
        e->origin=d;
        e->ready=true;
        e->departed=false;
    }
}
static void reject_beat(Engine *e, Reason why)
{
    count(e,&e->rejected);
    event(e,why);
    e->pending=false;
    e->pending_age=0;
}
static void beat(Engine *e, uint8_t state)
{
    if (!e->pending) {
        reject_beat(e,EMPTY_BEAT);
        return;
    }
    if (shed(state)!=e->pending_shed) {
        reject_beat(e,SHED_MOVED);
        return;
    }
    if (e->have_pick && e->last_shed==e->pending_shed) {
        reject_beat(e,REPEAT_SHED);
        return;
    }
    count(e,&e->accepted);
    e->last_shed=e->pending_shed;
    e->have_pick=true;
    e->last_interval=e->pending_age;
    if (e->last_interval<e->min_interval)
        e->min_interval=e->last_interval;
    if (e->last_interval>e->max_interval)
        e->max_interval=e->last_interval;
    e->pending=false;
    e->pending_age=0;
    event(e,ACCEPT);
}
/* One qualified shed must persist from departure through arrival. */
static bool flight_valid(Engine *e, uint8_t old, uint8_t state)
{
    bool leaving=!dock(state) && dock(old)==e->origin;
    if (leaving && shed(old)!=shed(state)) {
        invalidate(e);
        anomaly(e,AMBIGUOUS);
        return false;
    }
    if (e->departed && shed(state)!=e->flight_shed) {
        invalidate(e);
        anomaly(e,SHED_MOVED);
        return false;
    }
    if (leaving) {
        e->flight_shed=shed(state);
        e->flight_age=0;
    }
    return true;
}
static void age_flight(Engine *e, uint32_t elapsed)
{
    if (!e->departed)
        return;
    e->flight_age=add_sat(e->flight_age,elapsed);
    if (e->flight_age>MAX_BEAT_MS) {
        invalidate(e);
        anomaly(e,FLIGHT_EXPIRED);
    }
}
static void transition(Engine *e, uint8_t old, uint8_t state)
{
    uint8_t d=dock(state);
    bool rising_beat=(state & BEATER) && !(old & BEATER);
    if (d==(LEFT_DOCK|RIGHT_DOCK)) {
        invalidate(e);
        anomaly(e,BOTH_DOCKS);
        return;
    }
    if (!e->ready) {
        anchor(e,state);
        return;
    }
    /* A simultaneous arrival and beat has no observable ordering. */
    if (rising_beat && d!=dock(old)) {
        invalidate(e);
        anomaly(e,AMBIGUOUS);
        return;
    }
    if (!flight_valid(e,old,state))
        return;
    if (e->pending && shed(old)!=shed(state)) {
        e->pending=false;
        anomaly(e,SHED_MOVED);
    }
    if (!d && dock(old)==e->origin) {
        e->departed=true;
        /* Beginning another throw consumes an un-beaten arrival. */
        if (e->pending) {
            e->pending=false;
            anomaly(e,DOUBLE_PASS);
        }
    }
    if (d && d!=dock(old)) {
        if (!e->departed) {
            invalidate(e);
            anomaly(e,AMBIGUOUS);
            return;
        }
        if (d==e->origin) {
            e->departed=false;
            anomaly(e,RETURNED);
        } else {
            e->origin=d;
            e->departed=false;
            e->pending=true;
            e->pending_age=0;
            e->pending_shed=shed(state);
        }
    }
    if (rising_beat)
        beat(e,state);
}
void engine_sample(Engine *e, uint8_t raw, uint32_t now)
{
    uint32_t elapsed=now-e->now;
    if (!elapsed)
        return;
    e->now=now;
    e->raw=raw & 15u;
    /* Never synthesize stable observations over missing acquisition time. */
    if (elapsed!=1u) {
        count(e,&e->gaps);
        invalidate(e);
        e->qualified=false;
        e->qualified_ms=0;
        e->candidate=e->raw;
        anomaly(e,SAMPLE_GAP);
        return;
    }
    age_flight(e,elapsed);
    if (e->pending) {
        e->pending_age=add_sat(e->pending_age,elapsed);
        if (e->pending_age>MAX_BEAT_MS) {
            e->pending=false;
            anomaly(e,LATE_BEAT);
        }
    }
    /* Whole-vector qualification prevents separately debounced fake order. */
    if (e->raw!=e->candidate) {
        e->candidate=e->raw;
        e->qualified_ms=1;
        return;
    }
    if (e->qualified_ms<QUALIFY_MS)
        ++e->qualified_ms;
    if (e->qualified_ms<QUALIFY_MS)
        return;
    if (!e->qualified) {
        e->qualified=true;
        e->stable=e->candidate;
        if (dock(e->stable)==3u)
            anomaly(e,BOTH_DOCKS);
        else
            anchor(e,e->stable);
        return;
    }
    if (e->stable!=e->candidate) {
        uint8_t old=e->stable;
        e->stable=e->candidate;
        transition(e,old,e->stable);
    }
}
const char *engine_reason(Reason reason)
{
    switch (reason) {
    case NONE: return "none";
    case ACCEPT: return "accepted";
    case EMPTY_BEAT: return "beat-without-pass";
    case REPEAT_SHED: return "repeated-shed";
    case BOTH_DOCKS: return "both-docks";
    case DOUBLE_PASS: return "unbeaten-pass";
    case SHED_MOVED: return "shed-moved-before-beat";
    case LATE_BEAT: return "arrival-expired";
    case SAMPLE_GAP: return "acquisition-gap";
    case AMBIGUOUS: return "ambiguous-order";
    case RETURNED: return "returned-to-origin";
    case COUNTER_LIMIT: return "counter-limit";
    case FLIGHT_EXPIRED: return "flight-expired";
    default: return "invalid";
    }
}
