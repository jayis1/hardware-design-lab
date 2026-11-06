/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

static unsigned population(uint32_t bits)
{
    unsigned result = 0;
    for (unsigned i = 0; i < 4; ++i)
        result += (bits >> i) & 1u;
    return result;
}

static bool increment(uint32_t *value)
{
    if (*value >= LIMIT)
        return false;
    ++*value;
    return true;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint32_t fault)
{
    if (e->state == HALT)
        return;
    /* Active inventory already fits inside insertions, so abandonment fits.
     * Retained removal intervals and rank statistics never change on fault. */
    e->abandoned += population(e->occupied);
    e->occupied = 0;
    for (unsigned i = 0; i < 4; ++i)
        e->stamp[i] = 0;
    e->state = HALT;
    e->fault = fault;
}

void engine_tx_error(Engine *e)
{
    /* Transport health must not wrap or rewrite measurement outcomes. */
    if (e->tx_errors < LIMIT)
        ++e->tx_errors;
}

static void qualify(Engine *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t level = (raw >> i) & 1u;
        if (!e->stable[i] || e->candidate[i] != level) {
            e->candidate[i] = level;
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY) {
            uint32_t bit = 1u << i;
            e->valid |= bit;
            if (level)
                e->qualified |= bit;
            else
                e->qualified &= ~bit;
        }
    }
}

static bool departures(Engine *e, uint32_t removed)
{
    if (!removed)
        return true;
    e->last_mask = removed;
    /* Evaluate every departure against the SAME pre-change inventory.
     * Index order must never create an ordering within a simultaneous group. */
    for (unsigned i = 0; i < 4; ++i) {
        e->low[i] = 0;
        e->high[i] = 0;
        if (!(removed & (1u << i)))
            continue;
        for (unsigned j = 0; j < 4; ++j) {
            if (!e->stamp[j] || i == j)
                continue;
            if (e->stamp[j] < e->stamp[i])
                ++e->low[i];
            if (e->stamp[j] <= e->stamp[i])
                ++e->high[i];
        }
        if (!increment(&e->removals))
            return false;
        if (e->low[i] == e->high[i]) {
            if (!increment(&e->rank[e->low[i]]))
                return false;
        } else if (!increment(&e->ambiguous)) {
            return false;
        }
    }
    for (unsigned i = 0; i < 4; ++i) {
        if (removed & (1u << i))
            e->stamp[i] = 0;
    }
    e->occupied &= ~removed;
    return true;
}

static bool arrivals(Engine *e, uint32_t added)
{
    if (!added)
        return true;
    if (!increment(&e->epoch))
        return false;
    /* One cohort stamp per qualified change group; arbitrary first slot. */
    for (unsigned i = 0; i < 4; ++i) {
        if (!(added & (1u << i)))
            continue;
        if (!increment(&e->insertions))
            return false;
        e->stamp[i] = e->epoch;
        e->occupied |= 1u << i;
    }
    return true;
}

static bool account(Engine *e)
{
    if (e->state == WAIT) {
        /* Empty raw endpoint as well as qualified baseline: no claim about
         * inventory present at startup or an opening still being debounced. */
        if (e->valid == 15 && !e->qualified && !e->raw)
            e->state = RUNNING;
        return true;
    }
    uint32_t removed = e->occupied & ~e->qualified;
    uint32_t added = e->qualified & ~e->occupied;
    /* Additions in this observation are younger than every old member and
     * are excluded from departure ranking. This is a defined group boundary,
     * not a claim about physical sub-millisecond chronology. */
    return departures(e, removed) && arrivals(e, added);
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - e->now;
    if (e->state == HALT) {
        e->now = now;
        return;
    }
    if (raw > 15) {
        e->now = now;
        engine_fault(e, F_CONFIG);
        return;
    }
    if (e->seen && delta != 1) {
        e->now = now;
        e->gaps = 1;
        engine_fault(e, F_GAP);
        return;
    }
    if (e->sequence == UINT32_MAX) {
        e->now = now;
        engine_fault(e, F_COUNTER);
        return;
    }
    /* Staging makes multi-removal and insertion exhaustion all-or-nothing. */
    Engine next = *e;
    next.now = now;
    next.seen = true;
    ++next.sequence;
    next.raw = raw;
    qualify(&next, raw);
    if (!account(&next)) {
        e->now = now;
        engine_fault(e, F_COUNTER);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->state > HALT)
        return false;
    if (e->fault != 0 && e->fault != F_CONFIG &&
        e->fault != F_WATCHDOG && e->fault != F_GAP && e->fault != F_COUNTER)
        return false;
    if ((e->state == HALT) != (e->fault != 0))
        return false;
    if (e->raw > 15 || e->qualified > 15 || e->valid > 15 ||
        e->occupied > 15 || e->last_mask > 15)
        return false;
    if (e->gaps > 1 || (e->gaps != (e->fault == F_GAP)))
        return false;
    if (e->insertions > LIMIT || e->removals > LIMIT ||
        e->epoch > LIMIT || e->abandoned > 4 || e->tx_errors > LIMIT)
        return false;
    if (e->abandoned && e->state != HALT)
        return false;
    if (e->insertions != e->removals + e->abandoned + population(e->occupied))
        return false;
    if (e->epoch > e->insertions || e->insertions > 4u * e->epoch)
        return false;
    if ((e->epoch != 0) != (e->insertions != 0))
        return false;
    if (e->state == WAIT && (e->insertions || e->occupied))
        return false;
    if (e->state == HALT && e->occupied)
        return false;
    if (e->state == RUNNING && (e->valid != 15 || e->occupied != e->qualified))
        return false;
    uint64_t classified = e->ambiguous;
    unsigned latest_unique[4] = {0};
    unsigned latest_ambiguous = 0;
    for (unsigned i = 0; i < 4; ++i) {
        if (e->rank[i] > e->removals)
            return false;
        classified += e->rank[i];
        if ((e->stamp[i] != 0) != ((e->occupied & (1u << i)) != 0))
            return false;
        if (e->stamp[i] > e->epoch)
            return false;
        if (e->low[i] > e->high[i] || e->high[i] > 3)
            return false;
        if (e->last_mask & (1u << i)) {
            if (e->low[i] == e->high[i])
                ++latest_unique[e->low[i]];
            else
                ++latest_ambiguous;
        } else if (e->low[i] || e->high[i]) {
            return false;
        }
    }
    if (classified != e->removals)
        return false;
    if ((e->last_mask != 0) != (e->removals != 0))
        return false;
    if (population(e->last_mask) > e->removals || latest_ambiguous > e->ambiguous)
        return false;
    for (unsigned i = 0; i < 4; ++i) {
        if (latest_unique[i] > e->rank[i])
            return false;
    }
    return true;
}
