/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

/* There is no allocation and no dependency on a vendor runtime. */
void cw_init(CW *e, uint32_t now)
{
    *e = (CW){0};
    e->state = CW_WAIT;
    e->previous = now;
    for (unsigned i = 0; i < CW_CHANNELS; ++i)
        e->candidate_since[i] = now;
}

void cw_fault(CW *e)
{
    e->state = CW_FAULT;
    e->baseline = false;
}

static void wait_for_release(CW *e)
{
    if (e->state != CW_FAULT)
        e->state = CW_WAIT;
    e->baseline = false;
}

/* Result IDs never wrap: old and new evidence must not alias. */
static bool next_result(CW *e)
{
    if (e->result.id == UINT32_MAX) {
        cw_fault(e);
        return false;
    }
    ++e->result.id;
    return true;
}

static void finish(CW *e, uint32_t now, enum cw_reason reason)
{
    if (!next_result(e))
        return;
    e->result.keys = e->seen;
    e->result.reason = reason;
    e->result.duration = now - e->start;
    if (e->result.duration > CW_LIMIT)
        e->result.duration = CW_LIMIT;
    e->result.spread = 0;
    e->result.overlap = 0;
    e->result.release = 0;
    /* Invalid trials retain identity and duration, not plausible timing. */
    if (reason == CW_OK) {
        uint32_t latest_press = 0;
        uint32_t earliest_lift = CW_LIMIT;
        uint32_t latest_lift = 0;
        for (unsigned i = 0; i < CW_CHANNELS; ++i) {
            if (!(e->seen & (1u << i)))
                continue;
            if (e->press[i] > latest_press)
                latest_press = e->press[i];
            if (e->lift[i] < earliest_lift)
                earliest_lift = e->lift[i];
            if (e->lift[i] > latest_lift)
                latest_lift = e->lift[i];
        }
        e->result.spread = latest_press;
        e->result.release = latest_lift - earliest_lift;
        if (earliest_lift > latest_press)
            e->result.overlap = earliest_lift - latest_press;
    }
    wait_for_release(e);
}

static void begin(CW *e, uint32_t now)
{
    e->state = CW_ACTIVE;
    e->start = now;
    e->seen = 0;
    e->released = 0;
    e->retap = false;
    for (unsigned i = 0; i < CW_CHANNELS; ++i) {
        e->press[i] = 0;
        e->lift[i] = 0;
    }
}

/* Each input gets its own qualification clock. Chatter on B cannot
 * postpone a press or release on A. Timestamps are qualification times. */
static void qualify(CW *e, uint32_t now, uint8_t raw)
{
    for (unsigned i = 0; i < CW_CHANNELS; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw ^ e->candidate) & bit) {
            e->candidate ^= bit;
            e->candidate_since[i] = now;
            e->qualified &= (uint8_t)~bit;
        }
        if (now - e->candidate_since[i] >= CW_DEBOUNCE) {
            e->qualified |= bit;
            e->mask = (uint8_t)((e->mask & ~bit) | (raw & bit));
        }
    }
}

static void gap(CW *e, uint32_t now, uint8_t raw)
{
    if (e->gaps == UINT32_MAX) {
        cw_fault(e);
        return;
    }
    ++e->gaps;
    if (e->state == CW_ACTIVE)
        finish(e, now, CW_GAP);
    wait_for_release(e);
    e->candidate = raw;
    e->qualified = 0;
    for (unsigned i = 0; i < CW_CHANNELS; ++i)
        e->candidate_since[i] = now;
    /* Preserve mask until fresh evidence qualifies each input. */
}

static void baseline(CW *e, uint32_t now)
{
    if (e->mask || e->candidate || e->qualified != 15u) {
        e->baseline = false;
        return;
    }
    if (!e->baseline) {
        e->baseline = true;
        e->baseline_since = now;
    }
    if (now - e->baseline_since >= CW_BASELINE)
        e->state = CW_READY;
}

static void transitions(CW *e, uint32_t now, uint8_t old)
{
    uint8_t rises = (uint8_t)(e->mask & ~old);
    uint8_t falls = (uint8_t)(old & ~e->mask);
    for (unsigned i = 0; i < CW_CHANNELS; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if (rises & bit) {
            if (e->seen & bit) {
                e->retap = true;
            } else {
                e->seen |= bit;
                e->press[i] = now - e->start;
            }
        }
        if ((falls & bit) && !(e->released & bit)) {
            e->released |= bit;
            e->lift[i] = now - e->start;
        }
    }
}

void cw_tick(CW *e, uint32_t now, uint8_t raw)
{
    if (e->state == CW_FAULT)
        return;
    raw &= 15u;
    uint32_t elapsed = now - e->previous;
    if (!elapsed)
        return;
    e->previous = now;
    if (elapsed > 2u) {
        gap(e, now, raw);
        return;
    }
    uint8_t old = e->mask;
    qualify(e, now, raw);
    if (e->state == CW_WAIT) {
        baseline(e, now);
        return;
    }
    if (e->state == CW_READY && e->mask)
        begin(e, now);
    if (e->state != CW_ACTIVE)
        return;
    transitions(e, now, old);
    /* Deadline wins over a release qualifying on the same tick. */
    if (now - e->start >= CW_LIMIT) {
        finish(e, now, CW_TIMEOUT);
        return;
    }
    if (!e->mask)
        finish(e, now, e->retap ? CW_RETAP : CW_OK);
}
