/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

/* Four reflected Gray tracks encode absolute sectors, not pulse counts.
 * Every pattern is legal; adjacency checks cannot detect a stuck track. */
uint8_t engine_decode(uint8_t gray)
{
    uint8_t binary = gray & 15u;
    binary ^= binary >> 1;
    binary ^= binary >> 2;
    return binary;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->state = WAIT;
}

void engine_fault(Engine *e, uint32_t fault)
{
    e->fault |= fault;
    e->state = HALT;
    /* Retained qualified position is historical after a terminal fault. */
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors < LIMIT)
        ++e->tx_errors;
    else
        engine_fault(e, F_COUNTER);
}

static void qualify(Engine *e, uint8_t raw)
{
    /* Qualify the complete word. Independent bit filters could synthesize
     * a code that never existed at any sampled instant. */
    if (!e->stable || e->candidate != raw) {
        e->candidate = raw;
        e->stable = 1;
    } else if (e->stable < QUALIFY) {
        ++e->stable;
    }
    if (e->stable == QUALIFY) {
        e->qualified = raw;
        e->valid = 1;
    }
}

static void position(Engine *e)
{
    uint32_t next = engine_decode((uint8_t)e->qualified);
    if (!e->anchored) {
        e->anchored = true;
        e->sector = next;
        return;
    }
    if (e->sector == next)
        return;
    uint32_t delta = (next + SECTORS - e->sector) % SECTORS;
    uint32_t direction = 0;
    if (delta == 1u) {
        ++e->forward;
        direction = 1;
    } else if (delta == SECTORS - 1u) {
        ++e->backward;
        direction = 2;
    } else {
        /* Retain absolute occupancy after a skipped position, but never
         * bridge direction through that unobserved part of the path. */
        ++e->skipped;
    }
    if (direction && e->direction && direction != e->direction)
        ++e->reversals;
    e->direction = direction;
    e->sector = next;
    e->run = 0;
}

static void interval(Engine *e, bool previous_clean, uint32_t previous_sector)
{
    bool clean = e->valid && e->stable == QUALIFY &&
                 e->raw == e->qualified;
    if (previous_clean && clean && previous_sector == e->sector) {
        ++e->dwell[e->sector];
        ++e->run;
        if (e->run > e->longest[e->sector])
            e->longest[e->sector] = e->run;
    } else {
        ++e->unknown;
        e->run = 0;
    }
    e->clean = clean;
}

static void acquire(Engine *e, uint8_t raw)
{
    bool previous_clean = e->clean;
    uint32_t previous_sector = e->sector;
    e->raw = raw;
    qualify(e, raw);
    if (e->valid && e->stable == QUALIFY) {
        position(e);
        e->state = TRACK;
    }
    if (e->sequence) {
        interval(e, previous_clean, previous_sector);
    } else {
        e->clean = false;
    }
    ++e->sequence;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->state == HALT) {
        e->now = now;
        return;
    }
    if (raw > 15u) {
        engine_fault(e, F_CONFIG);
        return;
    }
    if (e->seen && now - e->now != 1u) {
        e->now = now;
        if (e->gaps < LIMIT)
            ++e->gaps;
        engine_fault(e, F_GAP);
        return;
    }
    e->now = now;
    e->seen = true;
    /* Sequence bounds every measurement counter, including all bins.
     * Stop before mutation, preserving a complete historical partition. */
    if (e->sequence == LIMIT) {
        engine_fault(e, F_COUNTER);
        return;
    }
    acquire(e, raw);
}

bool engine_valid(const Engine *e)
{
    if (!e || e->sequence > LIMIT || e->state > HALT)
        return false;
    if (e->fault > 15u || (e->state == HALT) != (e->fault != 0))
        return false;
    if (e->raw > 15u || e->qualified > 15u || e->sector >= SECTORS)
        return false;
    if (e->valid > 1u || e->direction > 2u)
        return false;
    if (e->state == TRACK && !e->valid)
        return false;
    if (e->state == WAIT && e->valid)
        return false;
    if (e->valid && e->sector != engine_decode((uint8_t)e->qualified))
        return false;
    if (e->valid && e->sequence < QUALIFY)
        return false;
    if (e->unknown > LIMIT || e->run > LIMIT)
        return false;
    if (e->tx_errors > LIMIT || e->gaps > 1u)
        return false;
    if ((e->gaps != 0) != ((e->fault & F_GAP) != 0))
        return false;
    uint64_t total = e->unknown;
    for (unsigned i = 0; i < SECTORS; ++i) {
        if (e->dwell[i] > LIMIT || e->longest[i] > e->dwell[i])
            return false;
        if ((e->dwell[i] == 0) != (e->longest[i] == 0))
            return false;
        total += e->dwell[i];
    }
    uint32_t elapsed = e->sequence ? e->sequence - 1u : 0;
    if (total != elapsed)
        return false;
    if (e->run > e->longest[e->sector])
        return false;
    if (e->run && (!e->valid || e->raw != e->qualified))
        return false;
    uint64_t moves = (uint64_t)e->forward + e->backward + e->skipped;
    if (moves > e->sequence / QUALIFY)
        return false;
    if (e->reversals > (uint64_t)e->forward + e->backward)
        return false;
    if (e->direction == 1 && !e->forward)
        return false;
    if (e->direction == 2 && !e->backward)
        return false;
    if (!e->valid && (moves || total != e->unknown || e->run))
        return false;
    return true;
}
