/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* State 2 denotes an unknowable two-bit jump, never a double count. */
static const int8_t quadrature[16] = {
    0, 1, -1, 2,
    -1, 0, 2, 1,
    1, 2, 0, -1,
    2, -1, 1, 0
};

static void increment(Engine *e, uint32_t *value)
{
    if (*value == UINT32_MAX) {
        e->saturated = true;
        return;
    }
    ++*value;
}

void engine_init(Engine *e, uint32_t time, uint8_t raw)
{
    memset(e, 0, sizeof *e);
    e->time = time;
    e->axis[0].previous = raw & 3u;
    e->axis[1].previous = (raw >> 2) & 3u;
    e->state = PRIMING;
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

static int8_t decode(Engine *e, unsigned index, uint8_t raw)
{
    Axis *a = &e->axis[index];
    unsigned code = ((unsigned)a->previous << 2) | raw;
    int8_t movement = quadrature[code];
    a->previous = raw;
    if (movement == 2) {
        increment(e, &a->invalid);
        return 0;
    }
    if (!movement)
        return 0;
    if (movement > 0) {
        if (a->position == INT32_MAX) {
            e->saturated = true;
            return 0;
        }
        ++a->position;
    } else {
        if (a->position == INT32_MIN) {
            e->saturated = true;
            return 0;
        }
        --a->position;
    }
    return movement;
}

static void discard(Engine *e)
{
    if (e->state == TAKING_UP)
        increment(e, &e->rejected);
    e->state = PRIMING;
    e->direction = 0;
    e->following = 0;
    e->travel = 0;
    e->elapsed = 0;
    e->candidate_first = 0;
}

static void fault(Engine *e)
{
    discard(e);
    e->state = FAULT;
}

static void prime(Engine *e, int8_t input, int8_t output)
{
    /* Establish direction from the drive, not from free output motion. */
    if (input && input != e->direction) {
        e->direction = input;
        e->following = 0;
    }
    if (!e->direction || !output)
        return;
    if (output != e->direction) {
        e->following = 0;
        return;
    }
    ++e->following;
    if (e->following == CONFIRM_EDGES) {
        e->state = TRACKING;
        e->following = 0;
    }
}

static void begin(Engine *e, int8_t direction)
{
    increment(e, &e->reversals);
    e->direction = direction;
    e->state = TAKING_UP;
    /* Include the first reverse edge; reference is before this sample. */
    e->travel = 1;
    e->elapsed = 1;
    e->following = 0;
    e->candidate_first = 0;
}

static void confirm_output(Engine *e, int8_t output)
{
    if (!output)
        return;
    if (output != e->direction) {
        /* Opposite output movement is a rejected trial, not a new zero. */
        discard(e);
        return;
    }
    if (!e->following)
        e->candidate_first = e->travel;
    ++e->following;
    if (e->following < CONFIRM_EDGES)
        return;
    increment(e, &e->events);
    e->first = e->candidate_first;
    e->confirm = e->travel;
    e->duration = e->elapsed;
    e->last_direction = e->direction;
    e->state = TRACKING;
    e->following = 0;
    e->travel = 0;
    e->elapsed = 0;
}

static void observe(Engine *e, int8_t input, int8_t output)
{
    if (e->state == PRIMING) {
        prime(e, input, output);
        return;
    }
    if (e->state == TRACKING) {
        if (input && input != e->direction) {
            begin(e, input);
            confirm_output(e, output);
        }
        return;
    }
    if (e->state != TAKING_UP)
        return;
    if (input && input != e->direction) {
        discard(e);
        /* This ambiguous sample is not reused for priming. */
        return;
    }
    if (input)
        increment(e, &e->travel);
    if (e->elapsed >= TIMEOUT_MS) {
        discard(e);
        return;
    }
    confirm_output(e, output);
}

void engine_sample(Engine *e, uint32_t time, uint8_t raw)
{
    uint32_t elapsed = time - e->time;
    if (!elapsed)
        return;
    e->time = time;
    if (elapsed != 1u) {
        increment(e, &e->gaps);
        e->axis[0].previous = raw & 3u;
        e->axis[1].previous = (raw >> 2) & 3u;
        fault(e);
        return;
    }
    int8_t input = decode(e, 0, raw & 3u);
    int8_t output = decode(e, 1, (raw >> 2) & 3u);
    if (engine_flags(e)) {
        fault(e);
        return;
    }
    if (e->state == TAKING_UP)
        increment(e, &e->elapsed);
    observe(e, input, output);
    if (e->saturated)
        fault(e);
}
