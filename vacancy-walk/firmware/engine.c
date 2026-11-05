/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

static unsigned population(uint8_t mask)
{
    unsigned count = 0;
    for (unsigned i = 0; i < 4; ++i)
        count += (mask >> i) & 1u;
    return count;
}

static uint8_t index_of(uint8_t bit)
{
    for (uint8_t i = 0; i < 4; ++i)
        if (bit == (1u << i))
            return i;
    return ABSENT;
}

static bool adjacent(uint8_t a, uint8_t b)
{
    return a < 4 && b < 4 && (((a + 1u) % 4 == b) || ((b + 1u) % 4 == a));
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->vacancy = ABSENT;
    e->source = ABSENT;
}

static void halt(Engine *e, uint8_t result)
{
    /* A pending lift is accounted exactly once, even if later faults arrive. */
    if (e->state == CARRY) {
        ++e->abandoned;
        e->last_ms = e->age;
    }
    if (e->state != HALT)
        e->result = result;
    e->state = HALT;
    e->previous_clean = false;
}

void engine_fault(Engine *e, uint8_t fault)
{
    e->fault |= fault;
    halt(e, ABORT);
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors == UINT32_MAX) {
        engine_fault(e, F_EXHAUST);
        return;
    }
    ++e->tx_errors;
}

static bool qualify(Engine *e, uint8_t raw)
{
    /* Treat occupancy as one vector: no invented intermediate combination.
     * This deliberately lets any channel chatter delay vector qualification. */
    if (!e->stable || raw != e->candidate) {
        e->candidate = raw;
        e->stable = 1;
    } else if (e->stable < QUALIFY_MS) {
        ++e->stable;
    }
    if (e->stable == QUALIFY_MS) {
        e->qualified = raw;
        e->valid = 1;
    }
    return e->valid && raw == e->qualified;
}

static void baseline(Engine *e, bool clean)
{
    /* Once tracking began, moving during rearm is not a new setup. */
    if (clean && e->result != NONE && e->qualified != e->base) {
        halt(e, AMBIGUOUS);
        return;
    }
    if (!clean || population(e->qualified) != 3) {
        e->baseline = 0;
        e->previous_clean = false;
        return;
    }
    if (e->base != e->qualified) {
        e->base = e->qualified;
        e->baseline = 0;
        e->previous_clean = false;
    }
    if (e->previous_clean && e->baseline < BASELINE_MS)
        ++e->baseline;
    e->previous_clean = true;
    if (e->baseline == BASELINE_MS) {
        e->vacancy = index_of((uint8_t)(15u ^ e->base));
        e->source = ABSENT;
        e->state = READY;
    }
}

static void departure(Engine *e)
{
    if (e->qualified == e->base)
        return;
    uint8_t removed = e->base & (uint8_t)~e->qualified;
    if (population(e->qualified) != 2 || population(removed) != 1 ||
        (e->qualified & (uint8_t)~e->base)) {
        halt(e, AMBIGUOUS);
        return;
    }
    uint8_t source = index_of(removed);
    if (!adjacent(e->vacancy, source)) {
        halt(e, NONADJACENT);
        return;
    }
    if (e->starts == LIMIT) {
        engine_fault(e, F_EXHAUST);
        return;
    }
    ++e->starts;
    e->source = source;
    e->age = 0;
    e->state = CARRY;
    e->baseline = 0;
    e->previous_clean = false;
}

static void arrival(Engine *e)
{
    uint8_t transit = e->base & (uint8_t)~(1u << e->source);
    if (e->qualified == transit)
        return;
    if (e->qualified == e->base) {
        ++e->canceled;
        e->result = CANCELED;
    } else if (e->qualified == (uint8_t)(15u ^ (1u << e->source))) {
        ++e->moves;
        /* Direction is VACANCY direction, opposite the puck's movement. */
        if ((e->vacancy + 1u) % 4 == e->source)
            ++e->clockwise;
        else
            ++e->counterclockwise;
        e->vacancy = e->source;
        e->base = e->qualified;
        e->result = MOVED;
    } else {
        halt(e, AMBIGUOUS);
        return;
    }
    e->last_ms = e->age;
    e->state = WAIT;
    e->baseline = 0;
    e->previous_clean = true;
    e->source = ABSENT;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - e->now;
    bool gap = e->have_sample && delta != 1u;
    e->now = now;
    e->have_sample = true;
    if (e->sequence == UINT32_MAX) {
        engine_fault(e, F_EXHAUST);
        return;
    }
    ++e->sequence;
    if (raw > 15) {
        engine_fault(e, F_INPUT);
        return;
    }
    e->raw = raw;
    if (gap) {
        if (e->gaps != UINT32_MAX)
            ++e->gaps;
        e->fault |= F_GAP;
        halt(e, LOST);
        e->valid = 0;
        e->stable = 0;
    }
    bool clean = qualify(e, raw);
    if (e->state == HALT)
        return;
    if (e->state == CARRY) {
        ++e->age;
        /* Reject at the deadline before evaluating a tied arrival. */
        if (e->age >= DEADLINE_MS) {
            halt(e, TIMEOUT);
            return;
        }
    }
    if (e->state == WAIT) {
        baseline(e, clean);
        return;
    }
    if (!clean)
        return;
    if (e->state == READY)
        departure(e);
    else if (e->state == CARRY)
        arrival(e);
}

bool engine_valid(const Engine *e)
{
    if (!e || e->state > HALT || e->result > ABORT || e->fault > 31 ||
        e->raw > 15 || e->qualified > 15 || e->valid > 1 ||
        e->vacancy > ABSENT || e->source > ABSENT || e->base > 15)
        return false;
    if (e->starts > LIMIT || e->moves > e->starts ||
        e->canceled > e->starts || e->abandoned > e->starts)
        return false;
    if (e->moves + e->canceled + e->abandoned + (e->state == CARRY) != e->starts)
        return false;
    if (e->clockwise > e->moves || e->counterclockwise > e->moves ||
        e->clockwise + e->counterclockwise != e->moves)
        return false;
    if (e->baseline > BASELINE_MS || e->age > DEADLINE_MS || e->last_ms > DEADLINE_MS)
        return false;
    if (e->fault && e->state != HALT)
        return false;
    if (e->state == READY || e->state == CARRY) {
        if (!e->valid || e->vacancy >= 4 ||
            e->base != (15u ^ (1u << e->vacancy)))
            return false;
    }
    if (e->state == READY && (e->source != ABSENT || e->qualified != e->base))
        return false;
    if (e->state == CARRY) {
        if (!adjacent(e->vacancy, e->source) || e->age >= DEADLINE_MS ||
            e->qualified != (e->base & ~(1u << e->source)))
            return false;
    }
    if (e->state == HALT && e->result < NONADJACENT)
        return false;
    return true;
}
