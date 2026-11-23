/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

uint32_t parity_odd(uint32_t word)
{
    /* Three data lanes and one even-parity lane; no error correction. */
    word ^= word >> 2;
    word ^= word >> 1;
    return word & 1u;
}

static void abandon(Engine *e)
{
    /* Conservation guarantees headroom: an active start is not yet aborted. */
    if (e->active)
        ++e->aborted;
    e->active = 0;
    e->age = 0;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint32_t fault)
{
    if (!e->fault)
        abandon(e);
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

static bool observe_word(Engine *e, uint32_t word)
{
    if (parity_odd(word)) {
        if (!e->active) {
            if (!add(&e->starts, 1))
                return false;
            e->active = 1;
            e->age = 0;
        }
        return true;
    }
    if (e->active) {
        uint32_t kind = !e->known ? 3u : e->anchor == word ? 1u : 2u;
        uint32_t *count = kind == 1 ? &e->same : kind == 2 ? &e->changed : &e->orphan;
        if (!add(count, 1))
            return false;
        e->last_age = e->age;
        e->last_from = e->known ? e->anchor : 0;
        e->last_to = word;
        e->last_kind = kind;
        e->active = 0;
        e->age = 0;
    }
    e->known = 1;
    e->anchor = word;
    return true;
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
        abandon(e);
        e->known = 0;
        e->anchor = 0;
        e->seeded = true;
        candidate_start(e, raw);
        return add(&e->unknown, 1);
    }
    /* Episode age includes the uncertain transitions between qualified words,
     * unlike histogram dwell. It never credits missing acquisition time. */
    if (e->active && !add(&e->age, 1))
        return false;
    if (raw != e->candidate) {
        candidate_start(e, raw);
        return add(&e->unknown, 1);
    }
    if (e->stable < QUALIFY)
        ++e->stable;
    if (e->stable == QUALIFY) {
        e->qualified = raw;
        e->valid = 1;
        if (!observe_word(e, raw))
            return false;
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
    if (e->active > 1 || e->known > 1 || e->anchor >= MASKS || parity_odd(e->anchor))
        return false;
    if (!e->known && e->anchor)
        return false;
    if ((!e->active && e->age) || (e->fault && e->active) || e->age > e->samples)
        return false;
    if ((uint64_t)e->same + e->changed + e->orphan + e->aborted + e->active != e->starts)
        return false;
    if (e->starts > e->samples / (QUALIFY + 1))
        return false;
    if (!e->fault && e->valid) {
        if (e->active != parity_odd(e->qualified))
            return false;
        if (!e->active && (!e->known || e->anchor != e->qualified))
            return false;
    }
    const uint64_t recovered = (uint64_t)e->same + e->changed + e->orphan;
    if (!recovered) {
        if (e->last_kind || e->last_age || e->last_from || e->last_to)
            return false;
    } else {
        if (e->last_kind < 1 || e->last_kind > 3 || e->last_age < QUALIFY + 1 ||
            e->last_age > e->samples || e->last_from >= MASKS || e->last_to >= MASKS ||
            parity_odd(e->last_from) || parity_odd(e->last_to))
            return false;
        if (e->last_kind == 1 && (!e->same || e->last_from != e->last_to))
            return false;
        if (e->last_kind == 2 && (!e->changed || e->last_from == e->last_to))
            return false;
        if (e->last_kind == 3 && (!e->orphan || e->last_from))
            return false;
    }
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
