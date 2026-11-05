/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

static bool increment(uint32_t *value)
{
    if (*value == UINT32_MAX)
        return false;
    ++*value;
    return true;
}

static void saturate(uint32_t *value)
{
    if (*value != UINT32_MAX)
        ++*value;
}

/* Zero and multi-contact codes are not station identities. */
static uint8_t identity(uint8_t mask)
{
    for (uint8_t i = 0; i < 4; ++i) {
        if (mask == (1u << i))
            return i;
    }
    return UNKNOWN_STATION;
}

static void break_run(Engine *e)
{
    e->direction = 0;
    e->run = 0;
}

/* An outstanding start reserves room for its abandonment: starts cannot
 * exceed UINT32_MAX and completed+aborted+active always equals starts. */
static void abandon(Engine *e)
{
    if (e->active) {
        ++e->aborted;
        e->active = false;
    }
    e->age = 0;
    e->station = UNKNOWN_STATION;
    e->origin = UNKNOWN_STATION;
    break_run(e);
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->station = UNKNOWN_STATION;
    e->origin = UNKNOWN_STATION;
    e->last_from = UNKNOWN_STATION;
    e->last_to = UNKNOWN_STATION;
}

void engine_fault(Engine *e, uint8_t fault)
{
    e->fault |= fault;
    abandon(e);
}

void engine_tx_error(Engine *e)
{
    saturate(&e->tx_errors);
}

/* Only complete four-bit signatures are qualified. Independent filtering
 * could manufacture a one-hot station from two overlapping raw contacts. */
static bool qualify(Engine *e, uint8_t raw)
{
    if (raw != e->candidate) {
        e->candidate = raw;
        e->stable_age = 0;
        return false;
    }
    if (e->stable_age < QUALIFY_MS)
        ++e->stable_age;
    if (e->stable_age != QUALIFY_MS)
        return false;
    if (e->ready && e->qualified == raw)
        return false;
    e->qualified = raw;
    e->ready = true;
    return true;
}

static bool finish(Engine *e, uint8_t destination)
{
    uint8_t delta = (uint8_t)((destination + 4u - e->origin) % 4u);
    uint8_t direction = 0;
    if (delta == 0) {
        if (!increment(&e->returns))
            return false;
    } else if (delta == 2) {
        if (!increment(&e->jumps))
            return false;
    } else {
        direction = delta == 1 ? 1 : 2;
        uint32_t *count = direction == 1 ? &e->cw : &e->ccw;
        if (!increment(count))
            return false;
        if (e->direction && e->direction != direction) {
            if (!increment(&e->reversals))
                return false;
        }
        e->run = e->direction == direction ? (uint8_t)(e->run + 1) : 1;
        if (e->run == 4) {
            uint32_t *laps = direction == 1 ? &e->cw_laps : &e->ccw_laps;
            if (!increment(laps))
                return false;
            e->run = 0;
        }
    }
    if (!direction)
        e->run = 0;
    e->direction = direction;
    e->last_from = e->origin;
    e->last_to = destination;
    e->last_ms = e->age;
    e->last_valid = true;
    e->active = false;
    e->age = 0;
    e->origin = UNKNOWN_STATION;
    e->station = destination;
    return true;
}

static bool transition(Engine *e)
{
    uint8_t target = identity(e->qualified);
    if (e->qualified && target == UNKNOWN_STATION) {
        /* Count one overlap on entry, not once per held sample. */
        if (!increment(&e->overlaps))
            return false;
        abandon(e);
        return true;
    }
    if (target != UNKNOWN_STATION) {
        if (e->active)
            return finish(e, target);
        if (e->station != UNKNOWN_STATION && e->station != target) {
            /* No qualified all-open departure: retain diagnostic only. */
            if (!increment(&e->shortcuts))
                return false;
            break_run(e);
        }
        e->station = target;
        return true;
    }
    if (e->station != UNKNOWN_STATION && !e->active) {
        if (!increment(&e->starts))
            return false;
        e->active = true;
        e->origin = e->station;
        e->station = UNKNOWN_STATION;
        e->age = 0;
    }
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t elapsed = now - e->now;
    e->now = now;
    if (raw > 15) {
        engine_fault(e, F_INPUT);
        return;
    }
    e->raw = raw;
    if (e->fault)
        return;
    if (!e->have_sample || elapsed != 1) {
        if (e->have_sample) {
            saturate(&e->gaps);
            abandon(e);
        }
        e->have_sample = true;
        e->ready = false;
        e->candidate = raw;
        e->stable_age = 0;
        return;
    }
    /* Copy before a multi-counter transition. Overflow publishes none of
     * that transition, then abandons exactly the original active journey. */
    Engine next = *e;
    if (next.active) {
        ++next.age;
        if (next.age >= DEADLINE_MS) {
            if (!increment(&next.timeouts)) {
                engine_fault(e, F_EXHAUST);
                return;
            }
            abandon(&next);
        }
    }
    if (qualify(&next, raw) && !transition(&next)) {
        engine_fault(e, F_EXHAUST);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->raw > 15 || e->qualified > 15 || e->candidate > 15)
        return false;
    if (e->station > 4 || e->origin > 4 || e->last_from > 4 || e->last_to > 4)
        return false;
    if (e->fault > 15 || e->direction > 2 || e->run > 3)
        return false;
    if (e->stable_age > QUALIFY_MS || e->age >= DEADLINE_MS)
        return false;
    uint64_t completed = (uint64_t)e->cw + e->ccw + e->returns + e->jumps;
    if (completed + e->aborted + e->active != e->starts)
        return false;
    if (e->reversals > (uint64_t)e->cw + e->ccw)
        return false;
    if ((uint64_t)e->cw_laps * 4 > e->cw || (uint64_t)e->ccw_laps * 4 > e->ccw)
        return false;
    if (e->timeouts > e->aborted || (!e->direction && e->run))
        return false;
    if (e->active && (e->origin == 4 || e->station != 4 || e->fault || !e->ready))
        return false;
    if (!e->active && (e->age || e->origin != 4))
        return false;
    if (e->last_valid != (completed != 0))
        return false;
    if (e->last_valid) {
        if (e->last_from == 4 || e->last_to == 4 || e->last_ms < 6 || e->last_ms >= DEADLINE_MS)
            return false;
    } else if (e->last_from != 4 || e->last_to != 4 || e->last_ms) {
        return false;
    }
    return true;
}
