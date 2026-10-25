/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

void ns_init(NS *e, uint32_t now)
{
    *e = (NS){0};
    e->previous = now;
    e->state = NS_WAIT;
    for (unsigned i = 0; i < NS_CHANNELS; ++i)
        e->candidate_since[i] = now;
}

void ns_fault(NS *e)
{
    e->state = NS_FAULT;
    e->baseline = false;
}

static void finish(NS *e, uint32_t now, enum ns_reason reason)
{
    /* Exhaustion is terminal, never silently reuse an event identity. */
    if (e->result.id == UINT32_MAX) {
        ns_fault(e);
        return;
    }
    ++e->result.id;
    e->result.reason = reason;
    uint32_t duration = now - e->start;
    e->result.duration = duration > NS_LIMIT ? NS_LIMIT : duration;
    e->result.first_settle = e->first_settle;
    e->result.full_ms = e->full_ms;
    e->result.longest = e->longest;
    e->result.drops = e->drops;
    e->result.drop_mask = e->drop_mask;
    e->result.seen = e->seen;
    e->state = NS_WAIT;
    e->baseline = false;
}

static void begin(NS *e, uint32_t now)
{
    e->start = now;
    e->state = NS_ACTIVE;
    e->full = e->mask == 15u;
    e->ever_full = e->full;
    e->settled = false;
    e->run = 0;
    e->full_ms = 0;
    e->longest = 0;
    e->first_settle = 0;
    e->drops = 0;
    e->drop_mask = 0;
    e->seen = e->mask;
}

static void qualify(NS *e, uint32_t now, uint8_t raw)
{
    /* Independent channels preserve inter-corner timing under chatter. */
    for (unsigned i = 0; i < NS_CHANNELS; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw ^ e->candidate) & bit) {
            e->candidate ^= bit;
            e->candidate_since[i] = now;
        }
        if (now - e->candidate_since[i] >= NS_DEBOUNCE)
            e->qualified |= bit;
        if (!(e->qualified & bit))
            continue;
        if (now - e->candidate_since[i] < NS_DEBOUNCE)
            continue;
        if (e->candidate & bit)
            e->mask |= bit;
        else
            e->mask &= (uint8_t)~bit;
    }
}

static void wait_empty(NS *e, uint32_t now)
{
    if (e->qualified != 15u || e->mask) {
        e->baseline = false;
        return;
    }
    if (!e->baseline) {
        e->baseline = true;
        e->baseline_since = now;
        return;
    }
    if (now - e->baseline_since >= NS_BASELINE)
        e->state = NS_READY;
}

static void gap(NS *e, uint32_t now, uint8_t raw)
{
    if (e->gaps == UINT32_MAX) {
        ns_fault(e);
        return;
    }
    ++e->gaps;
    if (e->state == NS_ACTIVE)
        finish(e, now, NS_GAP);
    if (e->state != NS_FAULT)
        e->state = NS_WAIT;
    e->baseline = false;
    e->qualified = 0;
    e->candidate = raw;
    /* Retain the previous qualified mask; raw post-gap data is not evidence. */
    for (unsigned i = 0; i < NS_CHANNELS; ++i)
        e->candidate_since[i] = now;
}

static void observe(NS *e, uint32_t now, uint32_t elapsed, uint8_t old)
{
    /* Integrate the previous qualified vector up to this tick. The incoming
     * vector must still be full before granting a new settling milestone. */
    if (e->full) {
        e->run += elapsed;
        e->full_ms += elapsed;
        if (e->run > e->longest)
            e->longest = e->run;
    }
    e->seen |= e->mask;
    if (e->mask == 15u) {
        e->ever_full = true;
        if (!e->full)
            e->run = 0;
        e->full = true;
        if (!e->settled && e->run >= NS_SETTLE) {
            e->settled = true;
            e->first_settle = now - e->start;
        }
    } else {
        /* Complete withdrawal ends a trial, not four corner dropout events.
         * Only partial vectors after the first full seating count dropouts. */
        if (e->ever_full && e->mask) {
            uint8_t fallen = (uint8_t)(old & ~e->mask);
            e->drop_mask |= fallen;
            for (unsigned i = 0; i < NS_CHANNELS; ++i) {
                if (fallen & (1u << i))
                    ++e->drops;
            }
        }
        e->full = false;
        e->run = 0;
    }
    if (!e->mask)
        finish(e, now, e->settled ? NS_OK : NS_UNSETTLED);
}

void ns_tick(NS *e, uint32_t now, uint8_t raw)
{
    raw &= 15u;
    uint32_t elapsed = now - e->previous;
    if (!elapsed || e->state == NS_FAULT)
        return;
    e->previous = now;
    if (elapsed > 1u) {
        gap(e, now, raw);
        return;
    }
    uint8_t old = e->mask;
    qualify(e, now, raw);
    if (e->state == NS_WAIT) {
        wait_empty(e, now);
        return;
    }
    if (e->state == NS_READY) {
        if (e->mask)
            begin(e, now);
        return;
    }
    /* Limit the observation before accounting beyond the finite trial.
     * No observed interval can exceed NS_LIMIT, including timestamp wrap. */
    if (now - e->start >= NS_LIMIT) {
        if (e->full) {
            e->run += elapsed;
            e->full_ms += elapsed;
            if (e->run > e->longest)
                e->longest = e->run;
        }
        finish(e, now, NS_TIMEOUT);
        return;
    }
    observe(e, now, elapsed, old);
}
