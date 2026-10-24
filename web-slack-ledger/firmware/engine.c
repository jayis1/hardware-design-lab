/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* Gray-code cycles are 00 -> 01 -> 11 -> 10 -> 00.
 * A simultaneous two-bit change cannot identify direction. Never guess it. */
static const int8_t quadrature[16] = {
    0, 1, -1, 2,
    -1, 0, 2, 1,
    1, 2, 0, -1,
    2, -1, 1, 0
};

static uint32_t add_elapsed(uint32_t value, uint32_t delta)
{
    if (UINT32_MAX - value < delta)
        return UINT32_MAX;
    return value + delta;
}

static void increment(uint32_t *value, bool *saturated)
{
    if (*value == UINT32_MAX) {
        *saturated = true;
        return;
    }
    ++*value;
}

static void reset_axis(Axis *axis, uint8_t phase)
{
    axis->position = 0;
    axis->forward = 0;
    axis->backward = 0;
    axis->invalid = 0;
    axis->edges = 0;
    axis->previous = phase;
}

void engine_init(Engine *e, uint32_t time, uint8_t raw)
{
    memset(e, 0, sizeof *e);
    reset_axis(&e->axis[0], raw & 3u);
    reset_axis(&e->axis[1], (raw >> 2) & 3u);
    e->time = time;
}

int64_t engine_difference(const Engine *e)
{
    /* Cast before subtraction; the signed difference needs 33 bits. */
    return (int64_t)e->axis[0].position - (int64_t)e->axis[1].position;
}

bool engine_valid(const Engine *e)
{
    return !e->gaps && !e->axis[0].invalid && !e->axis[1].invalid
           && !e->saturated;
}

uint32_t engine_flags(const Engine *e)
{
    uint32_t flags = 0;
    if (e->gaps)
        flags |= 1u;
    if (e->axis[0].invalid)
        flags |= 2u;
    if (e->axis[1].invalid)
        flags |= 4u;
    if (e->saturated)
        flags |= 8u;
    return flags;
}

static int8_t observe_axis(Engine *e, unsigned index, uint8_t phase)
{
    Axis *a = &e->axis[index];
    unsigned transition = ((unsigned)a->previous << 2) | phase;
    int8_t movement = quadrature[transition];
    a->previous = phase;
    if (movement == 2) {
        increment(&a->invalid, &e->saturated);
        return 0;
    }
    if (!movement)
        return 0;
    increment(&a->edges, &e->saturated);
    if (movement > 0) {
        increment(&a->forward, &e->saturated);
        if (a->position == INT32_MAX) {
            e->saturated = true;
            return 0;
        }
        ++a->position;
    } else {
        increment(&a->backward, &e->saturated);
        if (a->position == INT32_MIN) {
            e->saturated = true;
            return 0;
        }
        --a->position;
    }
    return movement;
}

static void close_leg(Engine *e)
{
    int64_t low = e->leg_min - e->origin;
    int64_t high = e->leg_max - e->origin;
    if (low < 0)
        low = -low;
    if (high < 0)
        high = -high;
    e->leg_peak = low > high ? low : high;
    e->previous_leg_ms = e->leg_ms;
}

static void observe_leg(Engine *e, int8_t drive, int64_t before)
{
    int64_t difference = engine_difference(e);
    if (!e->direction && drive) {
        e->direction = drive;
        /* Initial leg is anchored to boot-zero, not its first edge. */
        e->origin = 0;
        e->leg_ms = 0;
    } else if (drive && drive != e->direction) {
        close_leg(e);
        increment(&e->reversals, &e->saturated);
        e->direction = drive;
        /* Anchor before the first reverse edge; include that edge in new leg. */
        e->origin = before;
        e->leg_min = e->origin;
        e->leg_max = e->origin;
        e->leg_ms = 0;
    }
    if (difference < e->leg_min)
        e->leg_min = difference;
    if (difference > e->leg_max)
        e->leg_max = difference;
}

void engine_sample(Engine *e, uint32_t time, uint8_t raw)
{
    uint32_t elapsed = time - e->time;
    if (!elapsed)
        return;
    e->time = time;
    e->leg_ms = add_elapsed(e->leg_ms, elapsed);
    /* Time wrap is harmless; a missed tick is not continuous evidence. */
    if (elapsed != 1u) {
        increment(&e->gaps, &e->saturated);
        e->axis[0].previous = raw & 3u;
        e->axis[1].previous = (raw >> 2) & 3u;
        e->idle_ms = 0;
        return;
    }
    int64_t before = engine_difference(e);
    int8_t upstream = observe_axis(e, 0, raw & 3u);
    int8_t downstream = observe_axis(e, 1, (raw >> 2) & 3u);
    if (upstream || downstream)
        e->idle_ms = 0;
    else
        e->idle_ms = add_elapsed(e->idle_ms, elapsed);
    observe_leg(e, upstream, before);
}
