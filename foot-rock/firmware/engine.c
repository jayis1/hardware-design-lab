/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

unsigned support_class(uint32_t mask)
{
    if (mask == 15)
        return 4;
    for (unsigned foot = 0; foot < 4; ++foot) {
        if (mask == (15u ^ (1u << foot)))
            return foot;
    }
    return 5;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->anchor = NONE;
}

void engine_fault(Engine *e, uint32_t fault)
{
    /* Retain evidence verbatim, including the last anchor, on terminal faults. */
    e->fault |= fault;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors != UINT32_MAX)
        ++e->tx_errors;
}

static bool add(uint32_t *value, uint32_t amount)
{
    if (*value > UINT32_MAX - amount)
        return false;
    *value += amount;
    return true;
}

static void candidate_start(Engine *e, uint32_t raw)
{
    e->candidate = (uint8_t)raw;
    e->stable = 0;
    e->valid = 0;
    e->clean = false;
    e->settle = 0;
}

static bool visit(Engine *e, unsigned foot)
{
    if (foot == e->anchor)
        return true;
    if (!add(&e->visits[foot], 1))
        return false;
    if (e->anchor == NONE) {
        if (!add(&e->seeds, 1))
            return false;
    } else {
        unsigned index = e->anchor * 4 + foot;
        if (!add(&e->edges[index], 1))
            return false;
        if ((e->anchor ^ foot) == 3) {
            if (!add(&e->diagonal, 1))
                return false;
        } else {
            if (!add(&e->adjacent, 1))
                return false;
        }
    }
    e->anchor = foot;
    return true;
}

static bool acquire(Engine *e, uint32_t dt, uint32_t raw)
{
    bool previous_clean = e->clean;
    uint32_t previous_raw = e->raw;
    if (!add(&e->samples, 1))
        return false;
    e->raw = raw;
    /* Two or fewer raw contacts are handling/uncertainty, even when debounce
     * would reject that excursion. Never bridge a rock across such a loss. */
    unsigned kind = support_class(raw);
    if (kind == 5)
        e->anchor = NONE;
    if (!e->seeded || dt != 1) {
        if (e->seeded && !add(&e->missed, dt - 1))
            return false;
        e->seeded = true;
        e->anchor = NONE;
        candidate_start(e, raw);
        return add(&e->unknown, 1);
    }
    if (raw != e->candidate) {
        candidate_start(e, raw);
        return add(&e->unknown, 1);
    }
    if (e->stable < QUALIFY)
        ++e->stable;
    if (e->stable == QUALIFY) {
        e->qualified = raw;
        e->valid = 1;
    }
    e->clean = e->valid && e->qualified == raw;
    /* A visit is a qualified endpoint, not an interval or physical pivot. */
    if (e->clean && kind < 4 && !visit(e, kind))
        return false;
    if (!(e->clean && previous_clean && previous_raw == raw)) {
        e->settle = 0;
        return add(&e->unknown, 1);
    }
    if (!add(&e->dwell[kind], 1))
        return false;
    if (kind == 4) {
        if (e->settle < SETTLE)
            ++e->settle;
        if (e->settle == SETTLE)
            e->anchor = NONE;
    } else {
        e->settle = 0;
    }
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint32_t raw)
{
    uint32_t dt = now - e->now;
    e->now = now;
    if (e->fault)
        return;
    if (raw > 15) {
        engine_fault(e, F_INPUT);
        return;
    }
    if (dt == 0 || dt >= 0x80000000u) {
        engine_fault(e, F_TIME);
        return;
    }
    /* Speculative whole-state copy: exhaustion cannot publish half an edge. */
    Engine next = *e;
    if (!acquire(&next, dt, raw)) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->raw > 15 || e->qualified > 15)
        return false;
    if (e->valid > 1 || e->fault > 31 || e->anchor > NONE)
        return false;
    if (e->valid && (e->qualified != e->raw || e->samples < QUALIFY + 1))
        return false;
    if (e->settle > SETTLE)
        return false;
    if (e->settle && (!e->valid || e->raw != 15))
        return false;
    if (e->settle == SETTLE && e->anchor != NONE)
        return false;
    if (e->anchor < NONE && !e->visits[e->anchor])
        return false;
    if (support_class(e->raw) == 5 && e->anchor != NONE)
        return false;
    if (e->valid && support_class(e->raw) < 4 && e->anchor != support_class(e->raw))
        return false;
    uint64_t intervals = e->unknown;
    for (unsigned i = 0; i < 6; ++i)
        intervals += e->dwell[i];
    if (intervals != e->samples || e->settle > e->dwell[4])
        return false;
    if (e->unknown < (e->samples < 6 ? e->samples : 6))
        return false;
    uint64_t visits = 0;
    uint64_t diagonal = 0;
    uint64_t adjacent = 0;
    for (unsigned i = 0; i < 4; ++i) {
        visits += e->visits[i];
        uint64_t incoming = 0;
        uint64_t outgoing = 0;
        for (unsigned j = 0; j < 4; ++j) {
            uint32_t count = e->edges[i * 4 + j];
            if (i == j && count)
                return false;
            if ((i ^ j) == 3)
                diagonal += count;
            else
                adjacent += count;
            incoming += e->edges[j * 4 + i];
            outgoing += count;
        }
        /* Every edge requires a prior visit; a retained anchor remains unspent. */
        if (incoming > e->visits[i])
            return false;
        if (outgoing + (e->anchor == i ? 1u : 0u) > e->visits[i])
            return false;
    }
    if (diagonal != e->diagonal || adjacent != e->adjacent)
        return false;
    if (visits != diagonal + adjacent + e->seeds || visits > e->samples)
        return false;
    if (visits && !e->seeds)
        return false;
    return true;
}
