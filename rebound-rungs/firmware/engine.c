/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

static bool active(const Engine *e)
{
    return e->state == DOWN || e->state == TURN || e->state == UP;
}
static bool terminal(const Engine *e)
{
    return e->state == DONE || e->state == FAULT;
}
static bool one_bit(uint32_t value)
{
    return value && !(value & (value - 1u));
}
static unsigned ordinal(uint32_t value)
{
    unsigned result = 0;
    while (value > 1u) {
        value >>= 1;
        ++result;
    }
    return result;
}
static unsigned population(uint32_t value)
{
    unsigned count = 0;
    while (value) {
        count += value & 1u;
        value >>= 1;
    }
    return count;
}
void engine_init(Engine *e)
{
    *e = (Engine){0};
}
void engine_fault(Engine *e, uint32_t reason)
{
    /* Terminal evidence is immutable; only transport diagnostics can change. */
    if (terminal(e))
        return;
    e->faults |= reason;
    e->state = FAULT;
}
void engine_tx_error(Engine *e)
{
    /* At saturation this diagnostic is a lower bound, never a wrapped count. */
    if (e->tx_errors < UINT32_MAX)
        ++e->tx_errors;
}
static void complete(Engine *e)
{
    e->end_ms = e->age;
    e->state = DONE;
}
static void edge(Engine *e, unsigned rung)
{
    if (e->state == READY) {
        if (rung != 3u) {
            engine_fault(e, F_ORDER);
            return;
        }
        e->started = e->now;
        e->age = 0;
        e->downmask = 8;
        e->down[3] = 0;
        e->events = 1;
        e->state = DOWN;
        return;
    }
    if (e->state == DOWN) {
        unsigned expected = 3u - population(e->downmask);
        if (rung != expected) {
            engine_fault(e, F_ORDER);
            return;
        }
        e->down[rung] = e->age;
        e->downmask |= 1u << rung;
        ++e->events;
        if (!rung)
            e->state = TURN;
        return;
    }
    if (e->state == TURN) {
        /* The second bottom visit is the first upward crossing, provided
         * the physical single-slider and disjoint-zone contract holds. */
        if (rung != 0u) {
            engine_fault(e, F_ORDER);
            return;
        }
        e->up[0] = e->age;
        e->upmask = 1;
        e->high = 0;
        ++e->events;
        e->state = UP;
        return;
    }
    if (e->state == UP) {
        if (rung == e->high) {
            ++e->events;
            complete(e);
            return;
        }
        if (rung != e->high + 1u) {
            engine_fault(e, F_ORDER);
            return;
        }
        e->high = rung;
        e->up[rung] = e->age;
        e->upmask |= 1u << rung;
        ++e->events;
        if (rung == 3u)
            complete(e); /* Upper-censored: no upper bracket exists. */
    }
}
static void qualify(Engine *e, uint32_t raw)
{
    if (raw != e->candidate) {
        e->candidate = raw;
        e->candidate_ms = 0;
        return;
    }
    if (e->candidate_ms < QUALIFY_MS)
        ++e->candidate_ms;
    if (e->candidate_ms != QUALIFY_MS)
        return;
    uint32_t previous = e->stable;
    bool was_qualified = e->qualified != 0;
    e->qualified = 1;
    e->stable = raw;
    if (terminal(e) || e->state == WAIT || !raw)
        return;
    if (!was_qualified || previous == raw)
        return;
    if (previous || !one_bit(raw)) {
        engine_fault(e, F_ORDER);
        return;
    }
    edge(e, ordinal(raw));
}
void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->initialized && now == e->now)
        return;
    uint32_t delta = now - e->now;
    uint32_t previous_raw = e->raw;
    bool previous_clear = e->qualified && !e->stable && !previous_raw;
    e->now = now;
    e->raw = raw;
    if (raw > 15u) {
        engine_fault(e, F_CONFIG);
        return;
    }
    if (!e->initialized) {
        e->initialized = true;
        e->candidate = raw;
        return;
    }
    if (terminal(e))
        return;
    if (delta != 1u) {
        ++e->gaps; /* One terminal gap at most; no counter overflow. */
        engine_fault(e, F_GAP);
        return;
    }
    if (active(e)) {
        e->age = now - e->started;
        /* Timeout wins over a crossing at the deadline. */
        if (e->age >= LIMIT_MS) {
            e->age = LIMIT_MS;
            engine_fault(e, F_TIMEOUT);
            return;
        }
    }
    if (e->state != WAIT && raw && !one_bit(raw)) {
        engine_fault(e, F_OVERLAP);
        return;
    }
    qualify(e, raw);
    if (e->state != WAIT)
        return;
    if (previous_clear && e->qualified && !e->stable && !raw) {
        if (e->clear_ms < CLEAR_MS)
            ++e->clear_ms;
        if (e->clear_ms == CLEAR_MS)
            e->state = READY;
    } else {
        e->clear_ms = 0;
    }
}

bool engine_valid(const Engine *e)
{
    if (!e || e->state > FAULT || e->raw > 15 || e->stable > 15)
        return false;
    if (e->qualified > 1 || e->faults > 63 || e->high > 3)
        return false;
    if (e->age > LIMIT_MS || e->clear_ms > CLEAR_MS || e->gaps > 1)
        return false;
    if ((active(e) || e->state == DONE) && e->age >= LIMIT_MS)
        return false;
    if ((e->state == FAULT) != (e->faults != 0))
        return false;
    if ((e->faults & F_GAP) != (e->gaps ? F_GAP : 0))
        return false;
    if (e->downmask != 0 && e->downmask != 8 && e->downmask != 12 &&
        e->downmask != 14 && e->downmask != 15)
        return false;
    if (e->upmask != 0 && e->upmask != 1 && e->upmask != 3 &&
        e->upmask != 7 && e->upmask != 15)
        return false;
    if (e->upmask && e->downmask != 15)
        return false;
    if (e->upmask && e->upmask != ((1u << (e->high + 1u)) - 1u))
        return false;
    if (!e->upmask && e->high)
        return false;
    unsigned expected = population(e->downmask) + population(e->upmask);
    if (e->state == DONE && e->high < 3)
        ++expected;
    if (e->events != expected)
        return false;
    uint32_t last = 0;
    bool seen = false;
    for (int i = 3; i >= 0; --i) {
        if (!(e->downmask & (1u << i))) {
            if (e->down[i])
                return false;
            continue;
        }
        if (e->down[i] > e->age || (seen && e->down[i] < last + 8u))
            return false;
        last = e->down[i];
        seen = true;
    }
    if (e->down[3])
        return false;
    for (unsigned i = 0; i < 4; ++i) {
        if (!(e->upmask & (1u << i))) {
            if (e->up[i])
                return false;
            continue;
        }
        if (e->up[i] > e->age || e->up[i] < last + 8u)
            return false;
        last = e->up[i];
    }
    if (e->state == WAIT || e->state == READY) {
        if (e->events || e->age || e->downmask || e->upmask)
            return false;
    }
    if (e->state == READY && e->clear_ms != CLEAR_MS)
        return false;
    if (e->state == DOWN && (e->downmask == 0 || e->downmask == 15))
        return false;
    if (e->state == TURN && (e->downmask != 15 || e->upmask))
        return false;
    if (e->state == UP && (!e->upmask || e->high == 3))
        return false;
    if (e->state == DONE) {
        if (!e->upmask || e->end_ms != e->age)
            return false;
        if (e->high == 3 ? e->end_ms != last : e->end_ms < last + 8u)
            return false;
    } else if (e->end_ms) {
        return false;
    }
    return true;
}
