/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

unsigned tour_population(uint32_t mask)
{
    unsigned n = 0;
    for (unsigned i = 0; i < 4; ++i)
        n += (mask >> i) & 1u;
    return n;
}

static bool add(uint32_t *value, uint32_t amount)
{
    if (*value > UINT32_MAX - amount)
        return false;
    *value += amount;
    return true;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

static void abandon(Engine *e)
{
    /* Discarded counts only distinct first visits, not repeat strokes.
     * The sum of visits is capped at UINT32_MAX, reserving this headroom. */
    if (e->active) {
        ++e->aborted;
        e->discarded += tour_population(e->mask);
    }
    e->active = 0;
    e->mask = 0;
    e->age = 0;
    e->order = 0;
    e->armed = 0;
    e->blocked = true;
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

static bool visit(Engine *e, unsigned identity)
{
    uint64_t total = 0;
    for (unsigned i = 0; i < 4; ++i)
        total += e->visits[i];
    if (total == UINT32_MAX)
        return false;
    if (!add(&e->visits[identity], 1))
        return false;
    if (!e->active) {
        if (!add(&e->starts, 1))
            return false;
        e->active = 1;
        e->age = 0;
    }
    const uint32_t bit = 1u << identity;
    if (e->mask & bit) {
        if (!add(&e->repeats, 1))
            return false;
    } else {
        const unsigned position = tour_population(e->mask);
        e->order |= identity << (2 * position);
        e->mask |= bit;
    }
    e->armed = 0;
    if (e->mask == 15) {
        if (!add(&e->rounds, 1))
            return false;
        e->last_age = e->age;
        e->last_order = e->order;
        e->mask = 0;
        e->active = 0;
        e->age = 0;
        e->order = 0;
    }
    return true;
}

static bool acquire(Engine *e, uint32_t dt, uint32_t raw)
{
    if (!add(&e->samples, 1))
        return false;
    e->raw = raw;
    if (!e->seeded || dt != 1) {
        if (e->seeded && !add(&e->missed, dt - 1))
            return false;
        abandon(e);
        e->seeded = true;
        e->candidate = raw;
        e->stable = 0;
        e->valid = 0;
        e->envelope = raw;
        return true;
    }
    /* Rejecting deadline precedes new visit qualification. No tick is
     * retroactively counted as time before the first accepted visit. */
    if (e->active) {
        ++e->age;
        if (e->age >= DEADLINE) {
            if (!add(&e->timeouts, 1))
                return false;
            abandon(e);
        }
    }
    e->envelope |= raw;
    if (!e->blocked && tour_population(e->envelope) > 1) {
        if (!add(&e->disruptions, 1))
            return false;
        abandon(e);
    }
    if (raw != e->candidate) {
        e->candidate = raw;
        e->stable = 0;
        e->valid = 0;
        return true;
    }
    if (e->stable < QUALIFY)
        ++e->stable;
    if (e->stable < QUALIFY)
        return true;
    e->qualified = raw;
    e->valid = 1;
    if (!raw) {
        /* A complete all-clear window is required after boot, interruption,
         * accepted visit and raw multi-contact/hand-exchange ambiguity. */
        e->envelope = 0;
        e->blocked = false;
        e->armed = 1;
        return true;
    }
    if (!e->armed || e->blocked || e->envelope != raw)
        return true;
    for (unsigned i = 0; i < 4; ++i) {
        if (raw == (1u << i))
            return visit(e, i);
    }
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint32_t raw)
{
    const uint32_t dt = now - e->now;
    e->now = now;
    if (e->fault)
        return;
    if (raw > 15) {
        engine_fault(e, F_INPUT);
        return;
    }
    if (!dt || dt >= 0x80000000u) {
        engine_fault(e, F_TIME);
        return;
    }
    Engine next = *e;
    if (!acquire(&next, dt, raw)) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}

static bool order_valid(uint32_t order, uint32_t mask)
{
    const unsigned count = tour_population(mask);
    uint32_t seen = 0;
    if (order >> (2 * count))
        return false;
    for (unsigned i = 0; i < count; ++i) {
        uint32_t bit = 1u << ((order >> (2 * i)) & 3u);
        if (seen & bit)
            return false;
        seen |= bit;
    }
    return seen == mask;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->fault > 31 || e->raw > 15 || e->qualified > 15)
        return false;
    if (e->valid > 1 || e->armed > 1 || e->active > 1 || e->mask >= 15)
        return false;
    if (e->valid && (e->qualified != e->raw || e->samples < QUALIFY + 1))
        return false;
    if (e->active != (e->mask != 0) || e->age >= DEADLINE)
        return false;
    if ((!e->active && e->age) || (e->fault && (e->active || e->armed)))
        return false;
    if (!order_valid(e->order, e->mask))
        return false;
    if ((uint64_t)e->rounds + e->aborted + e->active != e->starts)
        return false;
    if (e->timeouts > e->aborted || e->age > e->samples)
        return false;
    if (e->discarded < e->aborted || (uint64_t)e->discarded > 3ull * e->aborted)
        return false;
    uint64_t visits = 0;
    for (unsigned i = 0; i < 4; ++i) {
        visits += e->visits[i];
        if (e->visits[i] < e->rounds + ((e->mask >> i) & 1u))
            return false;
    }
    if (visits > UINT32_MAX || visits > e->samples / (2 * (QUALIFY + 1)))
        return false;
    if (visits != 4ull * e->rounds + tour_population(e->mask) + e->discarded + e->repeats)
        return false;
    if (!e->rounds) {
        if (e->last_age || e->last_order)
            return false;
    } else if (!order_valid(e->last_order, 15) || e->last_age < 36 ||
               e->last_age >= DEADLINE || e->last_age > e->samples) {
        return false;
    }
    return true;
}
