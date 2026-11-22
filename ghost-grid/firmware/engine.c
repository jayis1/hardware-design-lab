/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

uint32_t matrix_scan(uint32_t physical)
{
    /* In a 2x2 undioded matrix only a three-edge path adds a false corner.
     * Inactive drive lines must be high impedance for this ideal model. */
    uint32_t count = 0;
    for (unsigned i = 0; i < 4; ++i)
        count += (physical >> i) & 1u;
    return count == 3 ? 15u : physical;
}

uint32_t matrix_ghost(uint32_t physical)
{
    return matrix_scan(physical) & ~physical;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint32_t fault)
{
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
    e->run = 0;
}

static bool acquire(Engine *e, uint32_t dt, uint32_t raw)
{
    const bool previous_clean = e->clean;
    const uint32_t previous_raw = e->raw;
    if (!add(&e->samples, 1))
        return false;
    e->raw = raw;
    if (!e->seeded || dt != 1) {
        if (e->seeded && !add(&e->missed, dt - 1))
            return false;
        e->seeded = true;
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
    /* The qualification endpoint cannot claim the preceding interval.
     * Rejected glitches also split segments, never merely subtract time. */
    if (!(e->clean && previous_clean && previous_raw == raw)) {
        e->run = 0;
        return add(&e->unknown, 1);
    }
    if (!e->run && !add(&e->segments[raw], 1))
        return false;
    if (!add(&e->run, 1) || !add(&e->bins[raw], 1))
        return false;
    if (e->run > e->peaks[raw])
        e->peaks[raw] = e->run;
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint32_t raw)
{
    const uint32_t dt = now - e->now;
    e->now = now;
    if (e->fault)
        return;
    if (raw >= MASKS) {
        engine_fault(e, F_INPUT);
        return;
    }
    if (dt == 0 || dt >= 0x80000000u) {
        engine_fault(e, F_TIME);
        return;
    }
    /* A multi-counter observation is one transaction. Do not publish a
     * segment or exposure if another counter cannot accept the sample. */
    Engine next = *e;
    if (!acquire(&next, dt, raw)) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->raw >= MASKS || e->qualified >= MASKS)
        return false;
    if (e->valid > 1 || e->fault > 31)
        return false;
    if (e->valid && e->qualified != e->raw)
        return false;
    if (e->valid && e->samples < QUALIFY + 1)
        return false;
    if (e->run && (!e->valid || e->run > e->peaks[e->raw]))
        return false;
    uint64_t accounted = e->unknown;
    for (unsigned i = 0; i < MASKS; ++i) {
        const uint32_t total = e->bins[i];
        const uint32_t count = e->segments[i];
        const uint32_t peak = e->peaks[i];
        if (!total) {
            if (count || peak)
                return false;
        } else {
            if (!count || !peak || peak > total)
                return false;
            if ((uint64_t)peak + count - 1 > total)
                return false;
            if ((uint64_t)peak * count < total)
                return false;
        }
        accounted += total;
    }
    if (accounted != e->samples)
        return false;
    if (e->samples && e->unknown < (e->samples < 6 ? e->samples : 6))
        return false;
    return true;
}
