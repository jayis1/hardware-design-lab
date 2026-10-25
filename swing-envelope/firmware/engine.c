/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <string.h>
#include <limits.h>

static uint32_t add(uint32_t a, uint32_t b)
{
    if (UINT32_MAX-a < b)
        return UINT32_MAX;
    return a+b;
}

static void increment(uint32_t *value)
{
    *value=add(*value,1);
}

/* Invalidate interval evidence, but retain historical counters and values.
 * The flags tell consumers that those previous values are not current. */
static void lose_continuity(Engine *e)
{
    e->pending=0;
    e->pending_age=0;
    e->cycle_age=0;
    e->idle_age=0;
    e->direction=0;
    e->outer_seen=0;
    e->cycle_anchor=false;
    e->period_valid=false;
    e->ready=false;
}

void engine_init(Engine *e, uint32_t now)
{
    memset(e,0,sizeof *e);
    e->now=now;
    e->previous=now;
}

void engine_transport_loss(Engine *e)
{
    increment(&e->transport_losses);
}

static void fault(Engine *e)
{
    increment(&e->faults);
    lose_continuity(e);
    e->blocked=true;
}

static void finish_excursion(Engine *e)
{
    /* A rightward crossing starts a right-side excursion; a leftward
     * crossing ends it. No result exists for the first partial excursion. */
    if (e->direction==1) {
        if (e->outer_seen & 8u)
            increment(&e->right_seen);
        else
            increment(&e->right_missed);
    }
    if (e->direction==2) {
        if (e->outer_seen & 4u)
            increment(&e->left_seen);
        else
            increment(&e->left_missed);
    }
}

static void complete_crossing(Engine *e, uint8_t direction)
{
    if (e->direction==direction) {
        fault(e);
        return;
    }
    finish_excursion(e);
    if (direction==1) {
        e->east_ms=e->pending_age;
        if (e->cycle_anchor) {
            e->period_ms=e->cycle_age;
            e->period_valid=true;
        }
        e->cycle_anchor=true;
        e->cycle_age=0;
    } else {
        e->west_ms=e->pending_age;
    }
    increment(&e->crossings);
    e->direction=direction;
    e->outer_seen=0;
    e->pending=0;
    e->pending_age=0;
    e->idle_age=0;
}

static void inner_edge(Engine *e, uint8_t edge)
{
    if (e->pending==0) {
        e->pending=edge;
        e->pending_age=0;
        return;
    }
    if (e->pending==edge) {
        /* Re-entry into the same gate is ambiguous: do not silently
         * reinterpret a bounce or a reversal as a clean trajectory. */
        fault(e);
        return;
    }
    if (!e->pending_age || e->pending_age>MAX_PAIR_MS) {
        fault(e);
        return;
    }
    complete_crossing(e, e->pending==1 ? 1 : 2);
}

static void qualified_change(Engine *e, uint8_t value)
{
    uint8_t previous=e->levels;
    uint8_t rising=(uint8_t)(value & ~previous);
    e->levels=value;
    /* Gate zones must be physically disjoint, including outer zones. */
    if (value && (value & (uint8_t)(value-1u))) {
        if (!e->blocked)
            fault(e);
        return;
    }
    if (!e->ready) {
        if (!value) {
            e->ready=true;
            e->blocked=false;
        }
        return;
    }
    if (rising & 12u) {
        /* Outer evidence is relevant only on the corresponding excursion.
         * At startup the pendulum may legitimately begin at an outer gate. */
        if (!e->direction)
            return;
        uint8_t expected=e->direction==1 ? 8u : 4u;
        if (rising!=expected || e->pending) {
            fault(e);
            return;
        }
        e->outer_seen|=rising;
    }
    if (rising & 3u)
        inner_edge(e,(uint8_t)(rising & 3u));
}

static void advance_time(Engine *e, uint32_t dt)
{
    if (e->pending)
        e->pending_age=add(e->pending_age,dt);
    if (e->cycle_anchor)
        e->cycle_age=add(e->cycle_age,dt);
    if (e->direction)
        e->idle_age=add(e->idle_age,dt);
    bool pair_expired=e->pending && e->pending_age>MAX_PAIR_MS;
    bool cycle_expired=e->cycle_anchor && e->cycle_age>MAX_CYCLE_MS;
    bool stopped=e->direction && e->idle_age>MAX_CYCLE_MS;
    if (pair_expired || cycle_expired || stopped) {
        increment(&e->timeouts);
        lose_continuity(e);
        /* Requalification of an all-clear vector is required after timeout. */
        e->qualified=false;
        e->stable_age=0;
    }
}

void engine_sample(Engine *e, uint8_t raw, uint32_t now)
{
    uint32_t dt=now-e->previous;
    e->now=now;
    if (!dt)
        return;
    e->previous=now;
    if (raw>15u) {
        fault(e);
        e->qualified=false;
        e->stable_age=0;
        return;
    }
    if (dt>2u) {
        increment(&e->gaps);
        lose_continuity(e);
        e->candidate=raw;
        e->stable_age=0;
        e->qualified=false;
        return;
    }
    advance_time(e,dt);
    if (raw!=e->candidate) {
        e->candidate=raw;
        e->stable_age=0;
        e->qualified=false;
        return;
    }
    e->stable_age=add(e->stable_age,dt);
    if (!e->qualified && e->stable_age>=QUALIFY_MS) {
        e->qualified=true;
        qualified_change(e,raw);
    }
}

uint32_t engine_flags(const Engine *e)
{
    uint32_t flags=0;
    if (e->ready)
        flags|=1u;
    if (e->period_valid)
        flags|=2u;
    if (e->pending)
        flags|=4u;
    if (e->blocked)
        flags|=8u;
    return flags;
}
