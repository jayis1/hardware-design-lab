/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>
#include <limits.h>

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

static void abandon(Engine *e)
{
    /* starts = completions + abandonments + active reserves this slot. */
    if (e->state == ACTIVE)
        ++e->aborted;
    e->active_ms = 0;
    e->peak = 0;
    e->state = WAIT;
    e->anchor = 0;
}

void engine_fault(Engine *e, uint8_t flag)
{
    if (e->state != HALTED)
        abandon(e);
    e->flags |= flag;
    e->state = HALTED;
}

void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx))
        engine_fault(e, F_OVERFLOW);
}

static void qualify(Engine *e, uint8_t raw)
{
    /* Qualify each two-bit rail as a bundle: individual-bit delays could
     * synthesize a forbidden thermometer code during a legal direct step.
     * The other rail's chatter cannot delay this rail. */
    for (unsigned i = 0; i < 2; ++i) {
        uint8_t mask = (uint8_t)(3u << (2u*i));
        if ((e->candidate ^ raw) & mask) {
            e->candidate = (uint8_t)((e->candidate & ~mask) | (raw & mask));
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY_MS) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY_MS) {
            e->levels = (uint8_t)((e->levels & ~mask) | (raw & mask));
            e->known |= mask;
        }
    }
}

static bool legal(uint8_t levels)
{
    return (levels & 3u) != 2u && ((levels >> 2) & 3u) != 2u;
}

static unsigned stage(unsigned code)
{
    return code == 3u ? 2u : code;
}

static unsigned difference(uint8_t levels)
{
    unsigned left = stage(levels & 3u);
    unsigned right = stage((levels >> 2) & 3u);
    return left > right ? left - right : right - left;
}

static void ready(Engine *e)
{
    if (e->known != 15u || e->raw != e->levels)
        return;
    if (e->levels == 0u || e->levels == 15u) {
        e->state = READY;
        e->anchor = e->levels == 0u ? 1u : 2u;
    }
}

static bool process(Engine *e, const Engine *previous)
{
    if (!legal(e->levels)) {
        engine_fault(e, F_CODE);
        return true;
    }
    if (e->state == WAIT) {
        ready(e);
        return true;
    }
    unsigned origin = e->anchor == 1u ? 0u : 15u;
    unsigned destination = origin ^ 15u;
    unsigned direction = e->anchor - 1u;
    if (e->state == READY) {
        if (e->levels == origin)
            return true;
        if (!increment(&e->starts))
            return false;
        e->state = ACTIVE;
        e->active_ms = 0;
        e->peak = (uint8_t)difference(e->levels);
        /* No observed intermediate vector: count the attempt, not a pass. */
        if (e->levels == destination) {
            abandon(e);
            ready(e);
        }
        return true;
    }
    if (e->state != ACTIVE)
        return true;
    ++e->active_ms;
    unsigned skew = difference(e->levels);
    if (skew > e->peak)
        e->peak = (uint8_t)skew;
    /* Both endpoints must agree and both rails must be stable. Neither
     * debounce time nor a sampled raw glitch is credited retroactively.
     * Direction bins are selected by origin, not instantaneous velocity. */
    if (e->raw == e->levels && previous->raw == previous->levels
        && previous->levels == e->levels && e->levels != 0u
        && e->levels != 15u) {
        if (!increment(&e->bins[3u*direction + skew]))
            return false;
    }
    /* A held partial drawer cannot create an indefinitely long traverse. */
    if (e->active_ms >= MAX_TRAVEL_MS) {
        abandon(e);
        ready(e);
        return true;
    }
    if (e->levels == origin) {
        abandon(e);
        ready(e);
        return true;
    }
    if (e->levels != destination)
        return true;
    /* Do not finish against a raw endpoint already disappearing. */
    if (e->raw != e->levels) {
        abandon(e);
        ready(e);
        return true;
    }
    if (!increment(&e->completed[direction]))
        return false;
    e->last_ms = e->active_ms;
    e->last_dir = (uint8_t)direction;
    e->last_peak = e->peak;
    e->active_ms = 0;
    e->peak = 0;
    e->state = WAIT;
    e->anchor = 0;
    ready(e);
    return true;
}

void engine_sample(Engine *e, uint32_t time, uint8_t raw)
{
    if (e->sampled && time == e->time)
        return;
    Engine next = *e;
    next.time = time;
    next.raw = raw & 15u;
    next.sampled = true;
    if (e->state == HALTED) {
        e->time = time;
        e->raw = next.raw;
        return;
    }
    if (e->sampled && time - e->time != 1u) {
        if (!increment(&next.gaps))
            next.flags |= F_OVERFLOW;
        engine_fault(&next, F_GAP);
        *e = next;
        return;
    }
    qualify(&next, next.raw);
    if (!process(&next, e)) {
        /* Transaction rollback prevents half-published multi-field results. */
        e->time = time;
        e->raw = next.raw;
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->raw > 15 || e->levels > 15 || e->known > 15)
        return false;
    if (e->known != 0 && e->known != 3 && e->known != 12 && e->known != 15)
        return false;
    if (e->state > HALTED || e->anchor > 2 || e->last_dir > 1)
        return false;
    if (e->peak > 2 || e->last_peak > 2)
        return false;
    if (e->flags > 31 || ((e->flags != 0) != (e->state == HALTED)))
        return false;
    if (e->gaps && !(e->flags & F_GAP))
        return false;
    if (!legal(e->levels) && !(e->flags & F_CODE))
        return false;
    uint64_t outcomes = (uint64_t)e->completed[0] + e->completed[1] + e->aborted;
    if (outcomes + (e->state == ACTIVE) != e->starts)
        return false;
    if (e->active_ms >= MAX_TRAVEL_MS || e->last_ms >= MAX_TRAVEL_MS)
        return false;
    if (e->state != ACTIVE && (e->active_ms || e->peak))
        return false;
    if ((e->state == READY || e->state == ACTIVE) != (e->anchor != 0))
        return false;
    if (e->state == READY && e->levels != (e->anchor == 1u ? 0u : 15u))
        return false;
    if (e->state == ACTIVE && (e->levels == 0 || e->levels == 15))
        return false;
    if ((e->state == READY || e->state == ACTIVE) && e->known != 15u)
        return false;
    if (e->state == ACTIVE && e->peak < difference(e->levels))
        return false;
    bool completed = e->completed[0] || e->completed[1];
    if (completed != (e->last_ms != 0))
        return false;
    if (!completed && (e->last_dir || e->last_peak))
        return false;
    if (completed && !e->completed[e->last_dir])
        return false;
    for (unsigned i = 0; i < 6; ++i) {
        if (!e->starts && e->bins[i])
            return false;
    }
    return true;
}
