/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

bool engine_active(const Engine *e)
{
    return e->phase >= OUTWARD && e->phase <= OUTER_BACK;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

/* Checked updates occur on a private copy. No half-transition is published. */
static bool increment(uint32_t *value)
{
    if (*value == UINT32_MAX)
        return false;
    ++*value;
    return true;
}

static bool reject(Engine *e, unsigned reason)
{
    if (engine_active(e)) {
        if (!increment(&e->aborted))
            return false;
        e->last_age = e->age;
        e->last = reason;
    }
    if (reason == RESULT_ORDER || reason == RESULT_INVALID) {
        if (!increment(&e->violations))
            return false;
    }
    e->phase = WAIT;
    e->age = 0;
    e->cycle_trips = 0;
    return true;
}

void engine_fault(Engine *e, uint32_t flag)
{
    if (engine_active(e)) {
        /* Conservation guarantees room: starts includes this active cycle. */
        ++e->aborted;
        e->last = RESULT_FAULT;
        e->last_age = e->age;
    }
    e->fault |= flag;
    e->phase = HALT;
    e->age = 0;
    e->cycle_trips = 0;
}

void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx_errors))
        engine_fault(e, F_OVERFLOW);
}

static bool complete(Engine *e)
{
    if (!increment(&e->completed))
        return false;
    if (e->cycle_trips && !increment(&e->nested))
        return false;
    e->last = e->cycle_trips ? RESULT_NESTED : RESULT_OUTER;
    e->last_age = e->age;
    if (e->age > e->maximum)
        e->maximum = e->age;
    e->age = 0;
    e->cycle_trips = 0;
    e->phase = READY;
    return true;
}

static bool transition(Engine *e, uint32_t previous)
{
    uint32_t mask = e->qualified;
    if (e->phase == WAIT) {
        if (mask == 5 && e->raw == 5)
            e->phase = READY;
        return true;
    }
    if (mask == previous)
        return true;
    if (e->phase == READY) {
        /* Count the departure even if its first observed destination is wrong. */
        if (!(mask & 1u)) {
            if (!increment(&e->starts))
                return false;
            e->phase = OUTWARD;
            e->age = 0;
        }
        if (mask != 4 || e->raw != 4)
            return reject(e, RESULT_ORDER);
        return true;
    }
    switch (e->phase) {
    case OUTWARD:
        if (mask == 6 && e->raw == 6)
            e->phase = EXTENDED;
        else
            return reject(e, RESULT_ORDER);
        break;
    case EXTENDED:
        if (mask == 2 && e->raw == 2)
            e->phase = INNER_OUT;
        else if (mask == 4 && e->raw == 4)
            e->phase = OUTER_BACK;
        else
            return reject(e, RESULT_ORDER);
        break;
    case INNER_OUT:
        if (mask == 10 && e->raw == 10)
            e->phase = INNER_FAR;
        else
            return reject(e, RESULT_ORDER);
        break;
    case INNER_FAR:
        if (mask == 2 && e->raw == 2)
            e->phase = INNER_BACK;
        else
            return reject(e, RESULT_ORDER);
        break;
    case INNER_BACK:
        if (mask != 6 || e->raw != 6)
            return reject(e, RESULT_ORDER);
        if (!increment(&e->inner_trips) || !increment(&e->cycle_trips))
            return false;
        e->phase = EXTENDED;
        break;
    case OUTER_BACK:
        if (mask != 5 || e->raw != 5)
            return reject(e, RESULT_ORDER);
        return complete(e);
    default:
        return false;
    }
    return true;
}

static bool step(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t elapsed = now - e->now;
    e->now = now;
    e->raw = raw;
    if (!increment(&e->sequence))
        return false;
    if (e->initialized && elapsed != 1u) {
        if (!increment(&e->gaps) || !reject(e, RESULT_GAP))
            return false;
        e->valid = 0;
        e->stable = 0;
        e->candidate = raw;
        return true;
    }
    if (!e->initialized) {
        e->initialized = true;
        e->candidate = raw;
        e->stable = 0;
        return true;
    }
    if (engine_active(e)) {
        ++e->age;
        /* The rejecting deadline wins over arrival at an endpoint. */
        if (e->age >= LIMIT) {
            if (!increment(&e->timeouts))
                return false;
            return reject(e, RESULT_TIMEOUT);
        }
        /* Continuous raw guard: rejected glitches still break authorization. */
        if ((e->phase == OUTWARD || e->phase == OUTER_BACK) &&
            (raw & 12u) != 4u)
            return reject(e, RESULT_ORDER);
        if (e->phase >= INNER_OUT && e->phase <= INNER_BACK &&
            (raw & 3u) != 2u)
            return reject(e, RESULT_ORDER);
    }
    if (raw != e->candidate) {
        e->candidate = raw;
        e->stable = 0;
    } else if (e->stable < QUALIFY) {
        ++e->stable;
    }
    if (e->stable < QUALIFY)
        return true;
    uint32_t previous = e->qualified;
    bool was_valid = e->valid != 0;
    e->qualified = raw;
    e->valid = 1;
    if ((raw & 3u) == 3u || (raw & 12u) == 12u) {
        /* One diagnostic per qualified bad-vector transition, not per sample. */
        if (!was_valid || previous != raw || e->phase != WAIT)
            return reject(e, RESULT_INVALID);
        return true;
    }
    return transition(e, previous);
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->phase == HALT) {
        e->now = now;
        return;
    }
    if (raw > 15) {
        engine_fault(e, F_CONFIG);
        return;
    }
    Engine next = *e;
    if (!step(&next, now, raw)) {
        e->now = now;
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->phase > HALT || e->fault > 7 || e->raw > 15 ||
        e->qualified > 15 || e->valid > 1 || e->stable > QUALIFY)
        return false;
    if ((e->phase == HALT) != (e->fault != 0))
        return false;
    if ((uint64_t)e->completed + e->aborted + engine_active(e) != e->starts)
        return false;
    if (e->nested > e->completed || e->nested > e->inner_trips)
        return false;
    if (e->timeouts > e->aborted || e->last > RESULT_FAULT)
        return false;
    if (e->age >= LIMIT || e->last_age > LIMIT || e->maximum >= LIMIT)
        return false;
    if (!engine_active(e) && (e->age || e->cycle_trips))
        return false;
    if (e->cycle_trips > e->inner_trips)
        return false;
    if ((e->phase != WAIT && e->phase != HALT) && !e->valid)
        return false;
    /* Qualified endpoint topology is fixed by the trajectory state. */
    static const uint8_t expected[] = {0, 5, 4, 6, 2, 10, 2, 4};
    if (e->phase > WAIT && e->phase < HALT &&
        e->qualified != expected[e->phase])
        return false;
    return true;
}
