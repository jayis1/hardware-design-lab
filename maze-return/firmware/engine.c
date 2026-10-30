/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->pending = 4;
}

void engine_fault(Engine *e, uint32_t fault)
{
    /* Correlation cannot recover from quiet gates: an unseen checkpoint might
     * have changed the loop-erased route. Preserve evidence until reboot. */
    e->fault |= fault;
    e->state = HALT;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors < COUNT_LIMIT)
        ++e->tx_errors;
    else if (e->state != HALT)
        engine_fault(e, F_COUNTER);
}

static bool multiple(uint32_t mask)
{
    return (mask & (mask - 1u)) != 0;
}

static uint32_t index_of(uint32_t mask)
{
    for (uint32_t i = 0; i < 4; ++i)
        if (mask == (1u << i))
            return i;
    return 4;
}

static void seed(Engine *e)
{
    for (unsigned i = 0; i < 4; ++i) {
        e->contact[i].candidate = (e->raw >> i) & 1u;
        e->contact[i].since = e->now;
    }
    e->initialized = 1;
}

static void qualify(Engine *e)
{
    for (unsigned i = 0; i < 4; ++i) {
        Contact *c = &e->contact[i];
        uint8_t value = (e->raw >> i) & 1u;
        if (value != c->candidate) {
            c->candidate = value;
            c->since = e->now;
        }
        if (e->now - c->since >= QUALIFY_MS) {
            c->stable = c->candidate;
            c->valid = 1;
        }
        if (c->valid) {
            e->qualified |= 1u << i;
            if (c->stable)
                e->mask |= 1u << i;
            else
                e->mask &= ~(1u << i);
        }
    }
}

static void commit_visit(Engine *e)
{
    /* Calculate first; reject exhaustion without publishing half an update. */
    uint32_t found = 0;
    while (found < e->depth && e->path[found] != e->pending)
        ++found;
    uint32_t removed = found < e->depth ? e->depth - found - 1u : 0;
    if (e->visits == COUNT_LIMIT || e->erased > COUNT_LIMIT - removed) {
        engine_fault(e, F_COUNTER);
        return;
    }
    ++e->visits;
    if (found == e->depth) {
        /* Four distinct identities guarantee capacity when none was found. */
        e->path[e->depth++] = e->pending;
        ++e->added;
    } else if (!removed) {
        ++e->repeats;
    } else {
        ++e->loops;
        e->erased += removed;
        e->depth = found + 1u;
        for (uint32_t i = e->depth; i < 4; ++i)
            e->path[i] = 0;
    }
    e->pending = 4;
    e->state = ARMED;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t dt = now - e->now;
    uint32_t prior_raw = e->raw;
    uint32_t prior_mask = e->mask;
    uint32_t prior_qualified = e->qualified;
    e->now = now;
    e->raw = raw & 15u;
    if (e->state == HALT)
        return;
    if (!e->initialized) {
        seed(e);
        return;
    }
    if (!dt)
        return;
    if (dt > MAX_GAP_MS) {
        if (e->gaps < COUNT_LIMIT)
            ++e->gaps;
        engine_fault(e, F_GAP);
        return;
    }
    qualify(e);
    if (e->qualified != 15u)
        return;
    if (!e->raw && !e->mask) {
        if (!prior_raw && !prior_mask && prior_qualified == 15u) {
            e->clear_ms += dt;
            if (e->clear_ms > CLEAR_MS)
                e->clear_ms = CLEAR_MS;
        } else {
            e->clear_ms = 0;
        }
    } else {
        e->clear_ms = 0;
    }
    switch (e->state) {
    case WAIT:
        /* Boot activity is excluded. A quiet baseline anchors the first walk. */
        if (e->clear_ms == CLEAR_MS)
            e->state = ARMED;
        break;
    case ARMED:
        if (multiple(e->raw) || multiple(e->mask)) {
            engine_fault(e, F_AMBIGUOUS);
            break;
        }
        if (e->mask) {
            uint32_t gate = index_of(e->mask);
            if (e->raw != e->mask) {
                engine_fault(e, F_AMBIGUOUS);
                break;
            }
            e->pending = gate;
            e->begun = e->now;
            e->elapsed = 0;
            e->state = CAPTURE;
        }
        break;
    case CAPTURE:
        e->elapsed = e->now - e->begun;
        /* Timeout precedes completion at the same sampled endpoint. */
        if (e->elapsed >= TIMEOUT_MS) {
            e->elapsed = TIMEOUT_MS;
            engine_fault(e, F_TIMEOUT);
            break;
        }
        if ((e->raw | e->mask) & ~(1u << e->pending)) {
            engine_fault(e, F_AMBIGUOUS);
            break;
        }
        if (e->clear_ms == CLEAR_MS)
            commit_visit(e);
        break;
    default:
        engine_fault(e, F_CONFIG);
        break;
    }
}

uint32_t engine_route(const Engine *e)
{
    uint32_t route = 0;
    for (uint32_t i = 0; i < e->depth; ++i)
        route |= e->path[i] << (2u * i);
    return route;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->state > HALT || e->fault > 63u)
        return false;
    if ((e->state == HALT) != (e->fault != 0u))
        return false;
    if (e->raw > 15u || e->mask > 15u || e->qualified > 15u)
        return false;
    if (e->visits > COUNT_LIMIT || e->added > COUNT_LIMIT ||
        e->loops > COUNT_LIMIT || e->repeats > COUNT_LIMIT ||
        e->erased > COUNT_LIMIT || e->gaps > COUNT_LIMIT ||
        e->tx_errors > COUNT_LIMIT)
        return false;
    if ((uint64_t)e->added + e->loops + e->repeats != e->visits)
        return false;
    if (e->depth > 4 || e->erased > e->added || e->added - e->erased != e->depth)
        return false;
    if ((uint64_t)e->erased < e->loops || (uint64_t)e->erased > 3ull * e->loops)
        return false;
    if ((e->visits != 0u) != (e->depth != 0u))
        return false;
    uint32_t seen = 0;
    for (uint32_t i = 0; i < 4; ++i) {
        if (i < e->depth) {
            if (e->path[i] > 3 || (seen & (1u << e->path[i])))
                return false;
            seen |= 1u << e->path[i];
        } else if (e->path[i]) {
            return false;
        }
    }
    if (e->pending > 4 || e->clear_ms > CLEAR_MS || e->elapsed > TIMEOUT_MS)
        return false;
    if (e->state == CAPTURE && (e->pending == 4 || e->qualified != 15u))
        return false;
    if ((e->state == WAIT || e->state == ARMED) && e->pending != 4)
        return false;
    return true;
}
