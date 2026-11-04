/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint8_t reason)
{
    /* First fault wins. Frozen trial evidence never becomes a new trial. */
    if (e->phase == HALT)
        return;
    e->fault = reason;
    e->phase = HALT;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors == UINT32_MAX) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    ++e->tx_errors;
}

static void qualify(Engine *e, uint8_t raw)
{
    /* Separate qualification prevents unrelated contact chatter from hiding
     * another fastener's stable transition. Timestamps are qualifying ticks,
     * not guessed physical engagement instants. */
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        uint8_t level = (uint8_t)((raw >> i) & 1u);
        if (e->candidate[i] != level || !e->stable[i]) {
            e->candidate[i] = level;
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY_SAMPLES) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY_SAMPLES) {
            e->valid |= bit;
            if (level)
                e->mask |= bit;
            else
                e->mask &= (uint8_t)~bit;
        }
    }
}

static bool append(Engine *e)
{
    /* One record per changed vector preserves true sampled ties. Sorting
     * contacts numerically would invent an order within a tied observation. */
    if (e->count == MAX_EVENTS) {
        engine_fault(e, F_CAPACITY);
        return false;
    }
    e->event[e->count].at = e->age;
    e->event[e->count].mask = e->mask;
    ++e->count;
    return true;
}

static void baseline(Engine *e, bool open)
{
    if (open && e->prior_open) {
        if (e->baseline < BASELINE_MS)
            ++e->baseline;
    } else {
        e->baseline = 0;
    }
    e->prior_open = open;
    if (e->baseline == BASELINE_MS)
        e->phase = READY;
}

static void settlement(Engine *e, bool closed)
{
    /* Both interval endpoints must be raw and qualified all closed. A
     * rejected raw glitch still cancels prior uninterrupted-settlement credit. */
    if (closed && e->prior_closed)
        ++e->quiet;
    else
        e->quiet = 0;
    e->prior_closed = closed;
    if (e->quiet == SETTLE_MS)
        e->phase = DONE;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - e->now;
    bool first = !e->clocked;
    e->now = now;
    e->clocked = true;
    if (raw > 15u) {
        engine_fault(e, F_CONFIG);
        return;
    }
    e->raw = raw;
    if (e->phase == HALT || e->phase == DONE)
        return;
    if (!first && delta != 1u) {
        engine_fault(e, F_GAP);
        return;
    }
    if (e->samples == UINT32_MAX) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    ++e->samples;
    uint8_t previous = e->mask;
    qualify(e, raw);
    bool open = e->valid == 15u && !e->raw && !e->mask;
    bool closed = e->valid == 15u && e->raw == 15u && e->mask == 15u;
    if (e->phase == WAIT) {
        baseline(e, open);
        return;
    }
    if (e->phase == READY) {
        if (!e->mask)
            return;
        e->phase = ACTIVE;
        e->age = 0;
        e->prior_closed = closed;
        (void)append(e);
        return;
    }
    /* The rejecting deadline takes precedence over both event capture and
     * settlement. No event at age 60000 is claimed by this finite notebook. */
    ++e->age;
    if (e->age >= DEADLINE_MS) {
        engine_fault(e, F_TIMEOUT);
        return;
    }
    if (e->mask != previous && !append(e))
        return;
    settlement(e, closed);
}

static bool events_valid(const Engine *e)
{
    uint8_t previous = 0;
    uint32_t prior_time = 0;
    for (unsigned i = 0; i < MAX_EVENTS; ++i) {
        const Event *v = &e->event[i];
        if (i >= e->count) {
            if (v->at || v->mask)
                return false;
            continue;
        }
        if (v->mask > 15u || v->mask == previous)
            return false;
        if (v->at > e->age || v->at >= DEADLINE_MS)
            return false;
        if (i == 0 && v->at != 0)
            return false;
        if (i && v->at <= prior_time)
            return false;
        previous = v->mask;
        prior_time = v->at;
    }
    return true;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->phase > HALT || e->fault > F_CAPACITY)
        return false;
    if (e->raw > 15u || e->mask > 15u || e->valid > 15u)
        return false;
    if (e->count > MAX_EVENTS || e->age > DEADLINE_MS)
        return false;
    if (e->quiet > SETTLE_MS || e->baseline > BASELINE_MS)
        return false;
    if ((e->phase == HALT) != (e->fault != F_NONE))
        return false;
    if (!events_valid(e))
        return false;
    if (e->phase == WAIT || e->phase == READY) {
        if (e->count || e->age || e->quiet)
            return false;
    }
    if (e->phase == READY && e->baseline != BASELINE_MS)
        return false;
    if (e->phase == ACTIVE || e->phase == DONE) {
        if (!e->count || e->valid != 15u)
            return false;
        if (e->event[e->count - 1u].mask != e->mask)
            return false;
    }
    if (e->phase == ACTIVE) {
        if (e->age >= DEADLINE_MS || e->quiet >= SETTLE_MS)
            return false;
    }
    if (e->phase == DONE) {
        if (e->age >= DEADLINE_MS)
            return false;
        if (e->quiet != SETTLE_MS || e->mask != 15u)
            return false;
        if (e->age < e->event[e->count - 1u].at + SETTLE_MS)
            return false;
    }
    if (e->quiet && e->mask != 15u && e->phase != HALT)
        return false;
    if (!e->count && e->age)
        return false;
    if (e->fault == F_TIMEOUT && e->age != DEADLINE_MS)
        return false;
    if (e->fault == F_CAPACITY && e->count != MAX_EVENTS)
        return false;
    return true;
}
