/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
uint32_t engine_sat_add(uint32_t a, uint32_t b)
{
    if (UINT32_MAX - a < b)
        return UINT32_MAX;
    return a + b;
}
bool engine_active(const Engine *e)
{
    return e->phase == PH_ARMED || e->phase == PH_TRANSFER;
}
void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->phase = PH_SETTLE;
    e->drain_ms = UNKNOWN_MS;
    e->fill_ms = UNKNOWN_MS;
}
static bool topology_ok(uint8_t mask)
{
    if ((mask & SOURCE_HIGH) && !(mask & SOURCE_LOW))
        return false;
    if ((mask & RECEIVER_HIGH) && !(mask & RECEIVER_LOW))
        return false;
    return true;
}
static void fail(Engine *e, uint32_t flag)
{
    e->flags |= flag;
    if (engine_active(e))
        e->failed = engine_sat_add(e->failed, 1);
    e->phase = PH_FAULT;
    e->ready_ms = 0;
    e->draining = false;
    e->filling = false;
}
static void arm(Engine *e)
{
    e->phase = PH_ARMED;
    e->trial = engine_sat_add(e->trial, 1);
    e->flags = 0;
    e->elapsed_ms = 0;
    e->drain_age = 0;
    e->fill_age = 0;
    e->drain_ms = UNKNOWN_MS;
    e->fill_ms = UNKNOWN_MS;
    e->draining = false;
    e->filling = false;
    e->ready_ms = 0;
}
static void transition(Engine *e, uint8_t previous)
{
    uint8_t rising = (uint8_t)(e->mask & ~previous);
    uint8_t falling = (uint8_t)(previous & ~e->mask);
    if (!topology_ok(e->mask)) {
        fail(e, FL_TOPOLOGY);
        return;
    }
    if (!engine_active(e))
        return;
    /* A source rise or receiver fall during a trial is not transfer. */
    if ((rising & (SOURCE_LOW | SOURCE_HIGH)) ||
        (falling & (RECEIVER_LOW | RECEIVER_HIGH))) {
        fail(e, FL_REVERSE);
        return;
    }
    /* Never invent zero-duration bands when both thresholds jump. */
    if ((falling & 3u) == 3u || (rising & 12u) == 12u) {
        fail(e, FL_SKIPPED);
        return;
    }
    if (falling & SOURCE_HIGH) {
        e->draining = true;
        e->drain_age = 0;
        e->phase = PH_TRANSFER;
    }
    if (rising & RECEIVER_LOW) {
        e->filling = true;
        e->fill_age = 0;
        e->phase = PH_TRANSFER;
    }
    if (falling & SOURCE_LOW) {
        if (!e->draining) {
            fail(e, FL_SKIPPED);
            return;
        }
        e->drain_ms = e->drain_age;
        e->draining = false;
    }
    if (rising & RECEIVER_HIGH) {
        if (!e->filling) {
            fail(e, FL_SKIPPED);
            return;
        }
        e->fill_ms = e->fill_age;
        e->filling = false;
    }
    if (e->mask == 12 && e->drain_ms != UNKNOWN_MS &&
        e->fill_ms != UNKNOWN_MS) {
        e->phase = PH_COMPLETE;
        e->cycles = engine_sat_add(e->cycles, 1);
    }
}
static void age_trial(Engine *e, uint32_t delta)
{
    if (!engine_active(e))
        return;
    e->elapsed_ms = engine_sat_add(e->elapsed_ms, delta);
    if (e->draining)
        e->drain_age = engine_sat_add(e->drain_age, delta);
    if (e->filling)
        e->fill_age = engine_sat_add(e->fill_age, delta);
    if (e->elapsed_ms >= TRIAL_LIMIT_MS)
        fail(e, FL_TIMEOUT);
}
static void ready_check(Engine *e, uint32_t delta)
{
    if (engine_active(e))
        return;
    if (!e->qualified || e->mask != 3 || e->candidate != 3) {
        e->ready_ms = 0;
        return;
    }
    e->ready_ms = engine_sat_add(e->ready_ms, delta);
    if (e->phase == PH_SETTLE)
        e->phase = PH_READY;
    if (e->ready_ms >= READY_MS)
        arm(e);
}
void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    raw &= 15u;
    if (!e->initialized) {
        e->initialized = true;
        e->last_ms = now;
        e->candidate = raw;
        return;
    }
    uint32_t delta = now - e->last_ms;
    e->last_ms = now;
    if (!delta)
        return;
    /* A missed sampling interval invalidates an active experiment.
     * Restart qualification, retaining the last known stable mask. */
    if (delta > 2) {
        e->gaps = engine_sat_add(e->gaps, 1);
        if (engine_active(e))
            fail(e, FL_GAP);
        e->qualified = false;
        e->candidate_ms = 0;
        e->ready_ms = 0;
        e->candidate = raw;
        return;
    }
    age_trial(e, delta);
    if (raw != e->candidate) {
        e->candidate = raw;
        e->candidate_ms = 0;
    } else {
        e->candidate_ms = engine_sat_add(e->candidate_ms, delta);
    }
    if (e->candidate_ms >= DEBOUNCE_MS &&
        (!e->qualified || e->mask != e->candidate)) {
        uint8_t previous = e->mask;
        bool was_qualified = e->qualified;
        e->mask = e->candidate;
        e->qualified = true;
        if (was_qualified)
            transition(e, previous);
        else if (!topology_ok(e->mask))
            fail(e, FL_TOPOLOGY);
    }
    ready_check(e, delta);
}
void engine_transport_loss(Engine *e)
{
    e->drops = engine_sat_add(e->drops, 1);
    e->flags |= FL_TRANSPORT;
}
