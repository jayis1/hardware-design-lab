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
    /* Conservation reserves one abandonment slot for an active stroke. */
    if (e->state == ACTIVE) {
        ++e->aborted;
        e->active_ms = 0;
    }
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
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t mask = (uint8_t)(1u << i);
        if ((e->candidate ^ raw) & mask) {
            e->candidate ^= mask;
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

static bool baseline(const Engine *e)
{
    unsigned endpoint = e->levels & 3u;
    return e->known == 15u && (endpoint == 1u || endpoint == 2u)
        && (e->raw & 3u) == endpoint;
}

static void ready(Engine *e)
{
    if (baseline(e)) {
        e->state = READY;
        e->anchor = e->levels & 3u;
    }
}

static bool process(Engine *e, const Engine *previous)
{
    unsigned endpoint = e->levels & 3u;
    if ((e->raw & 3u) == 3u || endpoint == 3u) {
        engine_fault(e, F_ENDPOINTS);
        return true;
    }
    if (e->state == WAIT) {
        ready(e);
        return true;
    }
    if (e->state == READY) {
        if (endpoint == e->anchor)
            return true;
        if (endpoint || (e->raw & 3u)) {
            /* Direct endpoint swaps have no observed travel segment. */
            e->state = WAIT;
            e->anchor = 0;
            ready(e);
            return true;
        }
        if (!increment(&e->starts))
            return false;
        e->state = ACTIVE;
        e->active_ms = 0;
        return true;
    }
    if (e->state != ACTIVE)
        return true;

    ++e->active_ms;
    /* Bins exclude departure/arrival, changing/raw-glitch endpoints,
     * and valve qualification ambiguity at either side of the interval.
     * They remain lifetime evidence even when a stroke is abandoned. */
    if (!(e->raw & 3u) && !endpoint && !(previous->raw & 3u)
        && !(previous->levels & 3u) && e->known == 15u
        && previous->known == 15u && e->raw == e->levels
        && previous->raw == previous->levels && e->levels == previous->levels) {
        unsigned direction = e->anchor == 1u ? 1u : 0u;
        unsigned bin = direction * 4u + (e->levels >> 2);
        if (!increment(&e->bins[bin]))
            return false;
    }
    /* Timeout wins over simultaneous endpoint qualification. */
    if (e->active_ms >= MAX_STROKE_MS) {
        abandon(e);
        ready(e);
        return true;
    }
    if (!endpoint)
        return true;
    if (endpoint == e->anchor || (e->raw & 3u) != endpoint) {
        abandon(e);
        ready(e);
        return true;
    }
    unsigned direction = e->anchor == 1u ? 1u : 0u;
    if (!increment(&e->completed[direction]))
        return false;
    e->last_ms = e->active_ms;
    e->last_dir = (uint8_t)direction;
    e->active_ms = 0;
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
        /* Rollback all speculative accounting before terminal abandonment. */
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
    if (e->state > HALTED || e->anchor > 2 || e->last_dir > 1)
        return false;
    if (e->flags > 31 || ((e->flags != 0) != (e->state == HALTED)))
        return false;
    if (e->gaps && !(e->flags & F_GAP))
        return false;
    uint64_t outcomes = (uint64_t)e->completed[0] + e->completed[1] + e->aborted;
    if (outcomes + (e->state == ACTIVE) != e->starts)
        return false;
    if (e->active_ms >= MAX_STROKE_MS || e->last_ms >= MAX_STROKE_MS)
        return false;
    if (e->state != ACTIVE && e->active_ms)
        return false;
    if ((e->state == READY || e->state == ACTIVE) && !e->anchor)
        return false;
    if ((e->state == WAIT || e->state == HALTED) && e->anchor)
        return false;
    if (e->state == READY && (e->levels & 3u) != e->anchor)
        return false;
    if (e->state == ACTIVE && (e->levels & 3u))
        return false;
    if ((e->state == READY || e->state == ACTIVE) && e->known != 15u)
        return false;
    bool completed = e->completed[0] || e->completed[1];
    if (completed != (e->last_ms != 0))
        return false;
    if (completed && !e->completed[e->last_dir])
        return false;
    for (unsigned i = 0; i < 8; ++i) {
        if (!e->starts && e->bins[i])
            return false;
    }
    return true;
}
