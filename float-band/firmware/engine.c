/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

/* Only these thermometer words can bracket a surface. A missing low
 * reference cannot be compensated by two higher references being wet. */
static uint8_t decode(uint8_t word)
{
    switch (word) {
    case 0: return 0;
    case 1: return 1;
    case 3: return 2;
    case 7: return 3;
    default: return UNKNOWN;
    }
}

static bool add(uint32_t *value)
{
    if (*value >= LIMIT)
        return false;
    ++*value;
    return true;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->band = UNKNOWN;
    e->captured = UNKNOWN;
}

void engine_fault(Engine *e, uint32_t fault)
{
    e->fault |= fault;
    /* Retain all statistics, but never finish a pending envelope after loss. */
    e->envelope = false;
    e->direction = 0;
}

void engine_tx_error(Engine *e)
{
    if (!add(&e->tx_errors))
        engine_fault(e, F_OVERFLOW);
}

/* Reference data is bundled. Independent bit debounce could synthesize
 * an intermediate band that never existed as a simultaneous raw word. */
static bool reference(Engine *e, uint8_t refs)
{
    if (!e->ref_run || refs != e->ref_candidate) {
        e->ref_candidate = refs;
        e->ref_run = 1;
    } else if (e->ref_run < QUALIFY) {
        ++e->ref_run;
    }
    if (e->ref_run == QUALIFY && (!e->ref_valid || refs != e->refs)) {
        uint8_t old = e->band;
        uint8_t band = decode(refs);
        uint8_t direction = 0;
        if (e->ref_valid && old < UNKNOWN && band < UNKNOWN) {
            if (band == old + 1u)
                direction = 1;
            else if (old == band + 1u)
                direction = 2;
            else if (old != band && !add(&e->skips))
                return false;
            if (direction && e->direction && direction != e->direction) {
                if (!add(&e->reversals))
                    return false;
            }
        }
        e->direction = direction;
        e->band = band;
        e->refs = refs;
        e->ref_valid = true;
    }
    if (e->ref_valid && e->band == UNKNOWN && refs == e->refs) {
        if (!add(&e->invalid_ticks))
            return false;
    }
    return true;
}

/* The DUT qualifier is independent of reference chatter. A sustained DUT
 * edge remains visible even when every reference is unusable. Attribution
 * instead requires one unchanged, previously qualified reference word over
 * the complete raw edge envelope. A raw glitch permanently taints that edge. */
static bool contact(Engine *e, uint8_t refs, uint8_t dut)
{
    if (!e->dut_valid) {
        if (!e->dut_run || dut != e->dut_candidate) {
            e->dut_candidate = dut;
            e->dut_run = 1;
        } else if (e->dut_run < QUALIFY) {
            ++e->dut_run;
        }
        if (e->dut_run == QUALIFY) {
            e->dut = dut;
            e->dut_valid = true;
        }
        return true;
    }
    if (dut == e->dut) {
        if (e->envelope && !add(&e->noise))
            return false;
        e->envelope = false;
        e->dut_run = 0;
        return true;
    }
    if (!e->envelope) {
        e->envelope = true;
        e->dut_run = 1;
        e->captured = e->band;
        e->tainted = !e->ref_valid || e->band == UNKNOWN || refs != e->refs;
    } else {
        ++e->dut_run;
    }
    if (refs != e->refs || !e->ref_valid || decode(refs) != e->captured)
        e->tainted = 1;
    if (e->dut_run == QUALIFY) {
        unsigned band = e->tainted ? UNKNOWN : e->captured;
        if (!add(&e->edges) || !add(&e->bins[dut][band]))
            return false;
        e->dut = dut;
        e->dut_run = 0;
        e->envelope = false;
    }
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->fault) {
        e->now = now; /* A heartbeat after halt is not acquisition. */
        return;
    }
    if (e->started && now - e->now != 1u) {
        e->now = now;
        if (!add(&e->gaps))
            engine_fault(e, F_OVERFLOW);
        engine_fault(e, F_GAP);
        return;
    }
    /* Copy before arithmetic: overflow cannot publish half an edge or a
     * reference movement without its associated diagnostics. */
    Engine next = *e;
    next.started = true;
    next.now = now;
    next.raw = raw & 15u;
    if (!add(&next.sequence) ||
        !contact(&next, next.raw & 7u, (next.raw >> 3) & 1u) ||
        !reference(&next, next.raw & 7u)) {
        e->now = now;
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->fault > 15u || e->raw > 15u || e->band > UNKNOWN ||
        e->direction > 2u || e->dut > 1u || e->ref_run > QUALIFY ||
        e->dut_run > QUALIFY || e->captured > UNKNOWN)
        return false;
    uint64_t total = 0;
    for (unsigned side = 0; side < 2; ++side) {
        for (unsigned band = 0; band < 5; ++band) {
            if (e->bins[side][band] > LIMIT)
                return false;
            total += e->bins[side][band];
        }
    }
    if (total != e->edges || e->edges > LIMIT || e->sequence > LIMIT ||
        e->noise > LIMIT || e->invalid_ticks > LIMIT || e->skips > LIMIT ||
        e->reversals > LIMIT || e->gaps > LIMIT || e->tx_errors > LIMIT)
        return false;
    if (e->fault && (e->envelope || e->direction))
        return false;
    if (!e->ref_valid && e->band != UNKNOWN)
        return false;
    if (e->ref_valid && decode(e->refs) != e->band)
        return false;
    return true;
}
