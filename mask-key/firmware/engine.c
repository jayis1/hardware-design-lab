/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>


unsigned engine_popcount(uint32_t value)
{
    unsigned count = 0;
    while (value) {
        value &= value - 1;
        ++count;
    }
    return count;
}

uint32_t engine_relation_cell(uint8_t mask)
{
    if (mask > 15)
        return 0;
    /* Low three bits are the witnessed controls; bit three is the response.
     * Row-major relation storage is intentionally distinct from raw masks. */
    return 1u << (2 * (mask & 7u) + (mask >> 3));
}

uint32_t engine_project(uint16_t settings)
{
    uint32_t result = 0;
    for (unsigned mask = 0; mask < 16; ++mask) {
        if (settings & (1u << mask))
            result |= engine_relation_cell((uint8_t)mask);
    }
    return result;
}

unsigned engine_suggest(const Engine *e)
{
    /* Suggest only unobserved INPUTS. Never prescribe the observed output.
     * This covers full control settings, not proof of subset sufficiency. */
    unsigned best = 8;
    unsigned cost = 4;
    for (unsigned input = 0; input < 8; ++input) {
        unsigned changes = engine_popcount(input ^ (e->raw & 7u));
        if (engine_row(e, input))
            continue;
        if (changes < cost) {
            best = input;
            cost = changes;
        }
    }
    return best;
}

void engine_init(Engine *e)
{
    /* Explicit initialization avoids a freestanding libc memset dependency. */
    e->now = 0;
    e->samples = 0;
    e->gaps = 0;
    e->missed = 0;
    e->tx_errors = 0;
    e->visits = 0;
    e->coverage = 0;
    e->qualified_ms = 0;
    e->unknown_ms = 0;
    e->settings = 0;
    e->hold = 0;
    e->raw = 0;
    e->credited = 0;
    e->seeded = 0;
    e->fault = F_NONE;
}

void engine_fault(Engine *e, uint8_t fault)
{
    /* First cause wins; historical evidence is never erased by a later fault. */
    if (!e->fault)
        e->fault = fault;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors != UINT32_MAX)
        ++e->tx_errors;
}

static bool add(uint32_t *target, uint32_t amount)
{
    if (UINT32_MAX - *target < amount)
        return false;
    *target += amount;
    return true;
}

static bool acquire(Engine *next, uint32_t elapsed, uint8_t raw)
{
    if (!add(&next->samples, 1))
        return false;
    if (elapsed != 1) {
        if (!add(&next->gaps, 1) || !add(&next->missed, elapsed - 1))
            return false;
        /* No interval across this hole can earn dwell or coverage. */
        next->raw = raw;
        next->hold = 0;
        next->credited = 0;
        return true;
    }
    if (raw != next->raw) {
        next->raw = raw;
        next->hold = 0;
        next->credited = 0;
        return add(&next->unknown_ms, 1);
    }
    if (next->hold < HOLD_MS) {
        ++next->hold;
        /* Qualification endpoint does not retroactively earn dwell. */
        if (!add(&next->unknown_ms, 1))
            return false;
        if (next->hold == HOLD_MS) {
            if (!add(&next->visits, 1))
                return false;
            next->credited = 1;
            next->settings |= (uint16_t)(1u << raw);
            next->coverage |= engine_relation_cell(raw);
        }
        return true;
    }
    return add(&next->qualified_ms, 1);
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->fault) {
        e->now = now;
        return;
    }
    if (raw > 15) {
        e->now = now;
        engine_fault(e, F_INPUT);
        return;
    }
    if (!e->seeded) {
        e->seeded = 1;
        e->now = now;
        e->raw = raw;
        e->samples = 1;
        return;
    }
    uint32_t elapsed = now - e->now;
    if (!elapsed)
        return;
    /* Transactional update: overflow cannot commit half a coverage admission. */
    Engine next = *e;
    next.now = now;
    if (!acquire(&next, elapsed, raw)) {
        e->now = now;
        engine_fault(e, F_EXHAUST);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e)
        return false;
    if (e->raw > 15 || e->seeded > 1 || e->credited > 1)
        return false;
    if (e->fault > F_EXHAUST || e->hold > HOLD_MS)
        return false;
    if (e->coverage != engine_project(e->settings))
        return false;
    if (e->credited != (e->hold == HOLD_MS))
        return false;
    if (e->credited && !(e->settings & (1u << e->raw)))
        return false;
    if (e->visits < engine_popcount(e->settings))
        return false;
    if ((e->visits == 0) != (e->settings == 0))
        return false;
    if (e->gaps > e->missed)
        return false;
    if (!e->seeded)
        return e->samples == 0 && e->hold == 0 && e->visits == 0;
    if (!e->samples)
        return false;
    /* Every sample after seeding is a gap endpoint or a classified interval. */
    uint64_t intervals = (uint64_t)e->unknown_ms + e->qualified_ms + e->gaps;
    if (intervals != (uint64_t)e->samples - 1)
        return false;
    /* Each visit spent a complete qualification window. */
    if ((uint64_t)e->visits * HOLD_MS > e->unknown_ms)
        return false;
    return true;
}
