/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

/* Durations are bounded interval accumulators, never wrapping ages. */
static uint32_t capped_add(uint32_t a, uint32_t b, uint32_t cap)
{
    if (a >= cap || b >= cap - a)
        return cap;
    return a + b;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->state = WAIT;
}

static void finish(Engine *e, bool abort)
{
    if (e->state != ACTIVE)
        return;
    if (abort) {
        ++e->aborted;
    } else {
        if (!e->bottoms)
            e->reason |= R_NO_BOTTOM;
        if (e->reason)
            ++e->bad;
        else
            ++e->good;
    }
    e->last_reason = e->reason;
    e->last_duration = e->elapsed;
    e->state = DRAIN;
    e->empty_ms = 0;
}

void engine_fault(Engine *e, uint32_t fault)
{
    if (e->state == ACTIVE) {
        e->elapsed = capped_add(0, e->now - e->begun, TIMEOUT_MS);
        e->reason |= R_EXHAUSTED;
        finish(e, true);
    }
    e->fault |= fault;
    e->state = HALT;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors < COUNT_LIMIT)
        ++e->tx_errors;
    else
        engine_fault(e, F_COUNTER);
}

/* Stop before any speculative transition when diagnostic capacity is gone.
 * Completion counters have room because they are bounded by prior starts. */
static bool capacity(const Engine *e)
{
    return e->starts < COUNT_LIMIT && e->extra < COUNT_LIMIT &&
           e->gaps < COUNT_LIMIT && e->tx_errors < COUNT_LIMIT;
}

static void reset_qualification(Engine *e, uint8_t raw)
{
    e->qualified = 0;
    e->seat_ms = 0;
    e->empty_ms = 0;
    for (unsigned i = 0; i < 4; ++i) {
        Contact *c = &e->contact[i];
        c->candidate = (raw >> i) & 1u;
        c->since = e->now;
        c->valid = 0;
    }
}

static void qualify(Engine *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        Contact *c = &e->contact[i];
        uint8_t value = (raw >> i) & 1u;
        if (value != c->candidate) {
            c->candidate = value;
            c->since = e->now;
        }
        if (e->now - c->since >= QUALIFY_MS) {
            c->stable = c->candidate;
            c->valid = 1;
        }
        if (c->valid) {
            e->qualified |= 1u << i;
            if (c->stable)
                e->mask |= 1u << i;
            else
                e->mask &= ~(1u << i);
        }
    }
}

static void start(Engine *e, uint32_t previous, uint32_t seated)
{
    ++e->starts;
    e->state = ACTIVE;
    e->reason = 0;
    e->elapsed = 0;
    e->bottoms = 0;
    e->begun = e->now;
    /* Eligibility belongs to the previous observation, not a tied endpoint. */
    if (seated < SEAT_MS || (previous & 3u) != 3u ||
        (e->mask & 3u) != 3u || (e->raw & 3u) != 3u)
        e->reason |= R_EARLY;
    if ((previous & 4u) || (e->mask & 4u))
        e->reason |= R_CONFLICT;
}

static void active(Engine *e, uint32_t previous)
{
    e->elapsed = e->now - e->begun;
    if (e->elapsed >= TIMEOUT_MS) {
        e->elapsed = TIMEOUT_MS;
        e->reason |= R_TIMEOUT;
        finish(e, true);
        return;
    }
    /* A single sampled stop release is conservative evidence of lost seating.
     * Debouncing must not hide withdrawal at a punch endpoint. */
    if ((e->raw & 3u) != 3u || (e->mask & 3u) != 3u)
        e->reason |= R_LOST;
    if ((e->mask & 12u) == 12u || (e->raw & 12u) == 12u)
        e->reason |= R_CONFLICT;
    if ((e->mask & 4u) && !(previous & 4u)) {
        if (e->bottoms < 2u)
            ++e->bottoms;
        if (e->bottoms > 1u)
            e->reason |= R_REPEAT;
    }
    if ((e->mask & 8u) && !(previous & 8u)) {
        /* Return must follow bottom in an earlier qualified observation. */
        if ((e->mask & 4u) || (e->raw & 4u))
            e->reason |= R_CONFLICT;
        finish(e, false);
    }
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t dt = now - e->now;
    uint32_t previous_raw = e->raw;
    uint32_t previous_qualified = e->qualified;
    e->now = now;
    e->raw = raw & 15u;
    if (e->state == HALT)
        return;
    if (!capacity(e)) {
        engine_fault(e, F_COUNTER);
        return;
    }
    if (!e->initialized) {
        e->initialized = 1;
        reset_qualification(e, raw);
        return;
    }
    if (!dt)
        return;
    if (dt > MAX_GAP_MS) {
        ++e->gaps;
        if (e->state == ACTIVE) {
            e->elapsed = capped_add(e->elapsed, dt, TIMEOUT_MS);
            e->reason |= R_GAP;
            finish(e, true);
        }
        e->state = WAIT;
        reset_qualification(e, raw);
        return;
    }
    uint32_t previous = e->mask;
    uint32_t seated = e->seat_ms;
    qualify(e, raw);
    if (e->qualified != 15u)
        return;
    if (e->mask == 11u && e->raw == 11u) {
        if (previous == 11u && previous_raw == 11u && previous_qualified == 15u)
            e->seat_ms = capped_add(e->seat_ms, dt, SEAT_MS);
        else
            e->seat_ms = 0;
    }
    else if (!(e->state == READY && e->mask == 11u && e->raw == 3u))
        e->seat_ms = 0;
    if (e->mask == 8u && e->raw == 8u) {
        if (previous == 8u && previous_raw == 8u && previous_qualified == 15u)
            e->empty_ms = capped_add(e->empty_ms, dt, EMPTY_MS);
        else
            e->empty_ms = 0;
    }
    else
        e->empty_ms = 0;
    bool departure = (previous & 8u) && !(e->mask & 8u);
    switch (e->state) {
    case WAIT:
    case DRAIN:
        if (departure && e->state == DRAIN)
            ++e->extra;
        if (e->empty_ms == EMPTY_MS)
            e->state = READY;
        break;
    case READY:
        if (departure)
            start(e, previous, seated);
        break;
    case ACTIVE:
        active(e, previous);
        break;
    default:
        engine_fault(e, F_CONFIG);
        break;
    }
}

bool engine_valid(const Engine *e)
{
    if (!e || e->state > HALT || e->fault > 7u)
        return false;
    if (e->raw > 15u || e->mask > 15u || e->qualified > 15u)
        return false;
    if (e->starts > COUNT_LIMIT || e->good > e->starts ||
        e->bad > e->starts || e->aborted > e->starts)
        return false;
    if ((uint64_t)e->good + e->bad + e->aborted +
        (e->state == ACTIVE) != e->starts)
        return false;
    if (e->extra > COUNT_LIMIT || e->gaps > COUNT_LIMIT ||
        e->tx_errors > COUNT_LIMIT)
        return false;
    if (e->reason > 255u || e->last_reason > 255u || e->bottoms > 2u)
        return false;
    if (e->elapsed > TIMEOUT_MS || e->last_duration > TIMEOUT_MS)
        return false;
    if (e->seat_ms > SEAT_MS || e->empty_ms > EMPTY_MS)
        return false;
    if ((e->state == HALT) != (e->fault != 0u))
        return false;
    if (e->state == ACTIVE && e->qualified != 15u)
        return false;
    if (!e->starts && (e->elapsed || e->bottoms || e->reason ||
                       e->last_reason || e->last_duration))
        return false;
    return true;
}
