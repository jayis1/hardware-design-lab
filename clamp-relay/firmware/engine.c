/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

/* Counts never wrap. A terminal fault preserves trial conservation. */
void engine_fault(Engine *e, uint32_t flag)
{
    if (e->result == ACTIVE) {
        ++e->aborted; /* starts already reserved this slot */
    }
    e->faults |= flag;
    e->result = FAULT;
    e->owner = 0;
    e->settle = 0;
}

static bool increment(Engine *e, uint32_t *value)
{
    if (*value == UINT32_MAX) {
        engine_fault(e, F_OVERFLOW);
        return false;
    }
    ++*value;
    return true;
}

void engine_tx_error(Engine *e)
{
    (void)increment(e, &e->tx_errors);
}

static uint8_t holding(uint8_t levels)
{
    uint8_t mask = 0;
    if ((levels & 3u) == 3u)
        mask |= 1u;
    if ((levels & 12u) == 12u)
        mask |= 2u;
    return mask;
}

/* This is a logical witness, deliberately not a force estimate. */
static uint8_t exclusive(uint8_t grips)
{
    return (grips == 1u || grips == 2u) ? grips : 0u;
}

static void finish(Engine *e, uint8_t result)
{
    if (result == DONE)
        ++e->completed;
    else
        ++e->aborted;
    e->result = result;
    e->owner = 0;
    e->candidate = 0;
    e->settle = 0;
}

static void forget_continuity(Engine *e, uint8_t raw)
{
    e->qualified = 0;
    e->raw = raw;
    e->owner = 0;
    e->candidate = 0;
    e->settle = 0;
    for (unsigned i = 0; i < 4; ++i)
        e->stable[i] = 0;
}

/* Independent qualification prevents a chattering unused switch from
 * delaying the other jaw's evidence. A gap restarts every qualifier. */
static void qualify(Engine *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((e->raw ^ raw) & bit) {
            e->stable[i] = 0;
        } else if (e->stable[i] < QUALIFY_MS) {
            ++e->stable[i];
        }
        if (e->stable[i] >= QUALIFY_MS) {
            e->levels = (uint8_t)((e->levels & ~bit) | (raw & bit));
            e->qualified |= bit;
        }
    }
    e->raw = raw;
    e->grips = holding(e->levels);
}

/* A stable exclusive witness is needed both to acquire an initial owner
 * and to finish a transfer. Time is measured in consecutive sample ticks. */
static bool settled(Engine *e)
{
    uint8_t next = exclusive(e->grips);
    if (next != e->candidate) {
        e->candidate = next;
        e->settle = 0;
    } else if (next && e->settle < SETTLE_MS) {
        ++e->settle;
    }
    return next && e->settle == SETTLE_MS;
}

static bool begin(Engine *e)
{
    if (!increment(e, &e->starts))
        return false;
    e->direction = e->owner;
    e->result = ACTIVE;
    e->elapsed_ms = 0;
    e->gap_ms = 0;
    e->overlap_ms = 0;
    e->max_gap_ms = 0;
    e->run_gap_ms = 0;
    /* One-vector change between exclusive owners is unresolved ordering,
     * not evidence of a zero-duration make-before-break transfer. */
    e->direct = exclusive(e->grips) && e->grips != e->owner;
    return true;
}

static void account(Engine *e, uint8_t previous_grips)
{
    ++e->elapsed_ms;
    if (!previous_grips) {
        ++e->gap_ms;
        ++e->run_gap_ms;
        if (e->run_gap_ms > e->max_gap_ms)
            e->max_gap_ms = e->run_gap_ms;
    } else {
        e->run_gap_ms = 0;
    }
    if (previous_grips == 3u)
        ++e->overlap_ms;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    raw &= 15u;
    if (!e->initialized) {
        e->initialized = true;
        e->now = now;
        forget_continuity(e, raw);
        return;
    }
    uint32_t dt = now - e->now;
    if (!dt)
        return;
    e->now = now;
    if (dt != 1u) {
        if (e->result == ACTIVE)
            finish(e, GAP);
        (void)increment(e, &e->gaps);
        forget_continuity(e, raw);
        return;
    }
    uint8_t previous_grips = e->grips;
    qualify(e, raw);
    if (e->faults)
        return;
    if (e->qualified != 15u)
        return;
    bool stable_owner = settled(e);
    if (e->result == ACTIVE) {
        account(e, previous_grips);
        /* Timeout wins ties with settlement: no optimistic completion. */
        if (e->elapsed_ms >= TRIAL_MS) {
            finish(e, TIMEOUT);
            return;
        }
        if (stable_owner) {
            uint8_t next = e->candidate;
            finish(e, next == e->direction ? RETURNED : DONE);
            e->owner = next;
            e->candidate = next;
            e->settle = SETTLE_MS;
        }
        return;
    }
    if (!e->owner) {
        if (stable_owner)
            e->owner = e->candidate;
        return;
    }
    if (e->grips != e->owner)
        (void)begin(e);
}

/* Encoder and host parser both enforce these necessary invariants. */
bool engine_valid(const Engine *e)
{
    uint64_t total = (uint64_t)e->completed + e->aborted;
    if (e->result == ACTIVE)
        ++total;
    if (total != e->starts)
        return false;
    if (e->levels > 15 || e->qualified > 15 || e->grips > 3)
        return false;
    if (e->grips != holding(e->levels))
        return false;
    if (e->owner > 2 || e->direction > 2 || e->result > FAULT)
        return false;
    if (e->faults & ~7u)
        return false;
    if (!!e->faults != (e->result == FAULT))
        return false;
    if (e->elapsed_ms > TRIAL_MS)
        return false;
    if (e->gap_ms + e->overlap_ms > e->elapsed_ms)
        return false;
    if (e->max_gap_ms > e->gap_ms || e->direct > 1)
        return false;
    if (!!e->gap_ms != !!e->max_gap_ms)
        return false;
    if (e->starts && !e->direction)
        return false;
    if (!e->starts && (e->elapsed_ms || e->gap_ms || e->overlap_ms || e->direction))
        return false;
    if (e->result == ACTIVE && (!e->owner || e->qualified != 15))
        return false;
    if (e->result == TIMEOUT && e->elapsed_ms != TRIAL_MS)
        return false;
    return true;
}
