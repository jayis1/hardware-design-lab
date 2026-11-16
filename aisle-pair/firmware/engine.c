/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

/* Convert a maintained two-threshold word, never a pulse count. */
static int stage(uint8_t word)
{
    switch (word) {
    case 0: return 0;
    case 1: return 1;
    case 3: return 2;
    default: return -1;
    }
}

int engine_cell(uint8_t raw)
{
    if (raw > 15u)
        return -1;
    int a = stage(raw & 3u);
    int b = stage((raw >> 2) & 3u);
    if (a < 0 || b < 0)
        return -1;
    return 3 * a + b;
}

uint32_t engine_shared(const Engine *e)
{
    /* Both drawers beyond the early threshold: four disjoint cells. */
    return e->bins[4] + e->bins[5] + e->bins[7] + e->bins[8];
}

void engine_fault(Engine *e, uint32_t fault)
{
    e->fault |= fault;
    e->phase = HALT;
    e->run = 0;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors == LIMIT) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    ++e->tx_errors;
}

/* A and B qualify independently, but each two-bit word is indivisible.
 * An early and late switch cannot synthesize a nonexistent legal stage
 * from unrelated per-bit debounce histories. */
static void qualify(Engine *e)
{
    for (unsigned i = 0; i < 2; ++i) {
        unsigned shift = 2u * i;
        uint8_t word = (uint8_t)((e->raw >> shift) & 3u);
        if (!e->streak[i] || e->candidate[i] != word) {
            e->candidate[i] = word;
            e->streak[i] = 1;
        } else if (e->streak[i] < QUALIFY) {
            ++e->streak[i];
        }
        if (e->streak[i] == QUALIFY) {
            e->valid |= (uint8_t)(1u << i);
            e->qualified = (uint8_t)((e->qualified & ~(3u << shift)) |
                                     ((unsigned)word << shift));
        }
    }
}

static bool clean(const Engine *e)
{
    return e->valid == 3u && e->raw == e->qualified &&
           engine_cell(e->qualified) >= 0;
}

static bool shared_cell(int cell)
{
    return cell == 4 || cell == 5 || cell == 7 || cell == 8;
}

/* Partition each consecutive acquired interval exactly once. Qualification
 * boundaries and even subqualification raw glitches are unknown intervals.
 * This makes the denominator explicit rather than silently dropping time. */
static void account(Engine *next, const Engine *prior)
{
    ++next->intervals;
    int cell = engine_cell(next->qualified);
    bool attributable = clean(prior) && clean(next) &&
                        prior->qualified == next->qualified;
    if (!attributable) {
        ++next->unknown;
        next->run = 0;
        return;
    }
    ++next->bins[cell];
    if (!shared_cell(cell)) {
        next->run = 0;
        return;
    }
    if (!next->run)
        ++next->episodes;
    ++next->run;
    if (next->run > next->maximum)
        next->maximum = next->run;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t elapsed = now - e->now;
    if (e->phase == HALT) {
        e->now = now;
        return;
    }
    if (raw > 15u) {
        e->now = now;
        engine_fault(e, F_CONFIG);
        return;
    }
    if (e->started && elapsed != 1u) {
        e->now = now;
        if (e->gaps < LIMIT)
            ++e->gaps;
        engine_fault(e, F_GAP);
        return;
    }
    if (e->sequence == LIMIT) {
        e->now = now;
        engine_fault(e, F_OVERFLOW);
        return;
    }
    /* Staging keeps evidence updates atomic if future limits change. All
     * acquisition counters are bounded by sequence, so one reserved sample
     * guarantees headroom for this sample's complete accounting operation. */
    Engine next = *e;
    next.now = now;
    next.raw = raw;
    ++next.sequence;
    qualify(&next);
    if (next.valid == 3u)
        next.phase = OBSERVE;
    if (next.valid == 3u && engine_cell(next.qualified) < 0)
        ++next.invalid;
    if (e->started)
        account(&next, e);
    next.started = true;
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || (e->phase != WAIT && e->phase != OBSERVE && e->phase != HALT))
        return false;
    if (e->fault > 15u || ((e->phase == HALT) != (e->fault != 0)))
        return false;
    if (e->raw > 15u || e->qualified > 15u || e->valid > 3u)
        return false;
    if (e->phase == OBSERVE && e->valid != 3u)
        return false;
    if (e->phase == WAIT && e->valid == 3u)
        return false;
    if (e->sequence > LIMIT || e->tx_errors > LIMIT || e->gaps > 1u)
        return false;
    if (e->intervals != (e->sequence ? e->sequence - 1u : 0u))
        return false;
    uint64_t total = e->unknown;
    for (unsigned i = 0; i < CELLS; ++i) {
        if (e->bins[i] > e->intervals)
            return false;
        total += e->bins[i];
    }
    if (total != e->intervals)
        return false;
    /* The first qualified endpoint cannot credit its preceding interval. */
    uint32_t startup_unknown = e->intervals < QUALIFY - 1u ?
                               e->intervals : QUALIFY - 1u;
    if (e->unknown < startup_unknown)
        return false;
    uint32_t qualified_samples = e->sequence >= QUALIFY ?
                                 e->sequence - QUALIFY + 1u : 0u;
    if (e->invalid > qualified_samples)
        return false;
    if (e->sequence < QUALIFY && e->valid)
        return false;
    if (e->invalid && e->sequence < QUALIFY)
        return false;
    uint32_t shared = engine_shared(e);
    if (e->episodes > shared || e->maximum > shared || e->run > e->maximum)
        return false;
    if ((shared != 0) != (e->episodes != 0))
        return false;
    if ((shared != 0) != (e->maximum != 0))
        return false;
    if (shared && (uint64_t)e->maximum + e->episodes - 1u > shared)
        return false;
    if (shared > (uint64_t)e->maximum * e->episodes)
        return false;
    if (e->run && (!clean(e) || !shared_cell(engine_cell(e->qualified))))
        return false;
    if (e->phase == HALT && e->run)
        return false;
    if ((e->gaps != 0) != ((e->fault & F_GAP) != 0))
        return false;
    return true;
}
