/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

static bool increment(uint32_t *value)
{
    if (*value == UINT32_MAX)
        return false;
    ++*value;
    return true;
}

/* An active departure always reserves room for its eventual disposition:
 * completed + aborted < starts <= UINT32_MAX. Abandonment cannot overflow
 * when the public conservation invariant holds. */
static void abandon(Engine *e)
{
    for (unsigned i = 0; i < 2; ++i) {
        if (e->active & (1u << i))
            ++e->aborted[i];
    }
    e->active = 0;
    e->ready = 0;
    e->marked = 0;
}

void engine_fault(Engine *e, uint32_t flag)
{
    abandon(e);
    e->faults |= flag & 7u;
}

void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx_errors))
        engine_fault(e, F_OVERFLOW);
}

static void invalidate(Engine *e, uint8_t raw)
{
    abandon(e);
    e->qualified = 0;
    e->raw = raw;
    for (unsigned i = 0; i < 4; ++i)
        e->stable[i] = 1;
}

/* Each contact has its own five-consecutive-observation filter. A changing
 * unrelated lid must never postpone a scoop departure or return. Retain
 * previously qualified levels while a new candidate is being debounced. */
static void qualify(Engine *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw ^ e->raw) & bit) {
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY_MS) {
            ++e->stable[i];
        }
        if (e->stable[i] >= QUALIFY_MS) {
            e->qualified |= bit;
            e->levels = (uint8_t)((e->levels & ~bit) | (raw & bit));
        }
    }
    e->raw = raw;
}

static bool known(const Engine *e, unsigned bit)
{
    return (e->qualified & (1u << bit)) != 0;
}

static bool closed(const Engine *e, unsigned bit)
{
    return (e->levels & (1u << bit)) != 0;
}

static bool foreign_open(const Engine *e, unsigned scoop)
{
    unsigned lid = scoop == 0 ? 3u : 1u;
    return known(e, lid) && !closed(e, lid);
}

/* Work on a staged copy: an overflowing second channel cannot publish a
 * half-updated first channel. The caller will fault the prior valid state. */
static bool observe(Engine *next, const Engine *previous)
{
    for (unsigned i = 0; i < 2; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        unsigned dock = i * 2;
        if (!known(next, dock))
            continue;
        if (closed(next, dock)) {
            if (next->active & bit) {
                if (!increment(&next->completed[i]))
                    return false;
                next->active &= (uint8_t)~bit;
                next->marked &= (uint8_t)~bit;
            }
            next->ready |= bit;
            continue;
        }
        if (next->ready & bit) {
            if (!increment(&next->starts[i]))
                return false;
            next->active |= bit;
            next->ready &= (uint8_t)~bit;
        }
        if (!(next->active & bit))
            continue;
        /* Count only intervals with an active excursion at both sampled
         * endpoints. Completion bins and unknown boundaries are excluded. */
        if (previous->active & bit) {
            if (!increment(&next->away_ms[i]))
                return false;
            if (foreign_open(previous, i) && foreign_open(next, i)) {
                if (!increment(&next->foreign_ms[i]))
                    return false;
            }
        }
        /* Even a zero-width qualified coincidence is evidence, not time.
         * Mark once per excursion; repeated lid openings do not inflate it. */
        if (foreign_open(next, i) && !(next->marked & bit)) {
            if (!increment(&next->flagged[i]))
                return false;
            next->marked |= bit;
        }
    }
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    raw &= 15u;
    if (!e->initialized) {
        e->initialized = true;
        e->now = now;
        invalidate(e, raw);
        return;
    }
    uint32_t dt = now - e->now;
    if (dt == 0)
        return;
    if (e->faults) {
        /* Terminal evidence remains frozen while uptime still progresses. */
        e->now = now;
        return;
    }
    if (dt != 1) {
        e->now = now;
        invalidate(e, raw);
        if (!increment(&e->gaps))
            engine_fault(e, F_OVERFLOW);
        return;
    }
    Engine next = *e;
    next.now = now;
    qualify(&next, raw);
    if (!observe(&next, e)) {
        e->now = now;
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->raw > 15 || e->levels > 15 || e->qualified > 15)
        return false;
    if (e->ready > 3 || e->active > 3 || e->marked > 3 || e->faults > 7)
        return false;
    if ((e->ready & e->active) || (e->marked & ~e->active))
        return false;
    if (e->faults && (e->active || e->ready || e->marked))
        return false;
    for (unsigned i = 0; i < 2; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        uint64_t dispositions = (uint64_t)e->completed[i] + e->aborted[i];
        dispositions += (e->active & bit) != 0;
        if (e->starts[i] != dispositions)
            return false;
        if (e->flagged[i] > e->starts[i])
            return false;
        uint64_t possible_flags = (uint64_t)e->completed[i] + e->aborted[i];
        possible_flags += (e->marked & bit) != 0;
        if (e->flagged[i] > possible_flags)
            return false;
        if (e->foreign_ms[i] > e->away_ms[i])
            return false;
        if (e->foreign_ms[i] && !e->flagged[i])
            return false;
        if (e->away_ms[i] && !e->starts[i])
            return false;
        if ((e->marked & bit) && !e->flagged[i])
            return false;
        if (e->active & bit) {
            if (!known(e, i * 2) || closed(e, i * 2))
                return false;
        }
        if (e->ready & bit) {
            if (!known(e, i * 2) || !closed(e, i * 2))
                return false;
        }
    }
    return true;
}
