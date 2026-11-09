/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

static void reject(Engine *e, uint32_t reason)
{
    if (e->state != ACTIVE)
        return;
    ++e->bad;
    e->reason = reason;
    e->state = SEEK;
    e->tooth_high = false;
}

void engine_fault(Engine *e, uint32_t fault)
{
    reject(e, (fault & F_GAP) ? R_GAP : R_FAULT);
    e->fault |= fault;
    e->state = HALT;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors < LIMIT)
        ++e->tx_errors;
    else
        engine_fault(e, F_COUNTER);
}

static void qualify(Engine *e, unsigned group, uint8_t mask)
{
    uint8_t value = (uint8_t)e->raw & mask;
    /* A/B is one bundle; index and tooth must not delay each other. */
    if (!e->stable[group] || e->candidate[group] != value) {
        e->candidate[group] = value;
        e->stable[group] = 1;
    } else if (e->stable[group] < QUALIFY) {
        ++e->stable[group];
    }
    if (e->stable[group] == QUALIFY) {
        e->qualified = (e->qualified & ~(uint32_t)mask) | value;
        e->valid |= 1u << group;
    }
}

static int movement(unsigned previous, unsigned current)
{
    /* Positive observed word: 00,01,11,10,00. Wiring defines the sign. */
    static const int8_t table[16] = {
         0, 1,-1, 2,
        -1, 0, 2, 1,
         1, 2, 0,-1,
         2,-1, 1, 0
    };
    return table[previous * 4 + current];
}

static void encoder(Engine *e, uint32_t before)
{
    int delta = movement(before & 3u, e->qualified & 3u);
    if (!delta)
        return;
    if (delta == 2) {
        reject(e, R_QUADRATURE);
        return;
    }
    uint32_t direction = delta > 0 ? 1u : 2u;
    if (e->direction && e->direction != direction) {
        reject(e, R_REVERSE);
        return;
    }
    e->direction = direction;
    ++e->steps;
    if (e->steps > EDGES)
        reject(e, R_STEPS);
}

static bool boundary_clean(const Engine *e)
{
    /* Even a raw, not-yet-qualified tooth crossing makes a cut ambiguous.
     * All groups must be settled so a deferred encoder edge cannot migrate
     * across the index boundary. */
    return e->valid == 7u && e->raw == e->qualified &&
           e->stable[0] == QUALIFY && e->stable[2] == QUALIFY &&
           !(e->raw & 8u) && !e->tooth_high;
}

static void start(Engine *e)
{
    ++e->starts;
    e->state = ACTIVE;
    e->steps = 0;
    e->teeth = 0;
    e->age = 0;
    e->direction = 0;
    e->tooth_high = false;
}

static void complete(Engine *e)
{
    /* Zero follower pulses is retained, not interpreted as a good gear.
     * "good" means an accepted acquisition, never a product-quality verdict. */
    if (!e->good) {
        e->reference = e->teeth;
        e->minimum = e->teeth;
        e->maximum = e->teeth;
    }
    if (e->teeth < e->minimum)
        e->minimum = e->teeth;
    if (e->teeth > e->maximum)
        e->maximum = e->teeth;
    if (e->teeth != e->reference)
        ++e->mismatch;
    e->last = e->teeth;
    e->last_ms = e->age;
    e->total += e->teeth;
    ++e->good;
    e->state = SEEK;
}

static void follower(Engine *e, uint32_t before)
{
    bool rose = !(before & 8u) && (e->qualified & 8u);
    bool fell = (before & 8u) && !(e->qualified & 8u);
    if (!rose && !fell)
        return;
    /* Require the index window to remain raw and qualified low for pulses.
     * This costs a small excluded arc, intentionally placed in a tooth valley. */
    if ((e->raw | e->qualified) & 4u) {
        reject(e, R_BOUNDARY);
        return;
    }
    if (rose) {
        e->tooth_high = true;
    } else if (e->tooth_high) {
        ++e->teeth;
        e->tooth_high = false;
    }
}

static void acquire(Engine *e, uint8_t raw)
{
    uint32_t before = e->qualified;
    uint32_t valid_before = e->valid;
    bool active_before = e->state == ACTIVE;
    e->raw = raw;
    qualify(e, 0, 3);
    qualify(e, 1, 4);
    qualify(e, 2, 8);
    ++e->sequence;
    if (active_before) {
        ++e->age;
        /* Rejecting deadline precedes all same-sample completions. */
        if (e->age >= TIMEOUT)
            reject(e, R_TIMEOUT);
        if (e->state == ACTIVE)
            encoder(e, before);
        if (e->state == ACTIVE)
            follower(e, before);
    }
    if ((e->valid & 2u) && !(e->qualified & 4u) && !(raw & 4u))
        e->index_armed = true;
    bool rise = (valid_before & 2u) && !(before & 4u) &&
                (e->qualified & 4u) && e->index_armed;
    if (!rise)
        return;
    e->index_armed = false;
    /* Never restart on the very observation that rejected a capture. */
    if (active_before && e->state != ACTIVE)
        return;
    bool clean = boundary_clean(e);
    if (e->state == ACTIVE) {
        if (!clean) {
            reject(e, R_BOUNDARY);
            return;
        }
        if (e->steps != EDGES)
            reject(e, R_STEPS);
        else
            complete(e);
    }
    if (clean)
        start(e);
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
        ++e->gaps;
        engine_fault(e, F_GAP);
        return;
    }
    e->now = now;
    e->seen = true;
    /* Every trial, edge and completed pulse consumes at least one sample;
     * sequence therefore bounds all measurement sums before increment. */
    if (e->sequence >= LIMIT) {
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
    if (e->raw > 15u || e->qualified > 15u || e->valid > 7u)
        return false;
    if (e->state == ACTIVE && e->valid != 7u)
        return false;
    if (e->starts != (uint64_t)e->good + e->bad + (e->state == ACTIVE))
        return false;
    if (e->starts > e->sequence || e->mismatch > e->good)
        return false;
    if (e->good && e->mismatch >= e->good)
        return false;
    if (e->steps > EDGES + 1u || e->direction > 2u || e->age > TIMEOUT)
        return false;
    if (e->state == ACTIVE && (e->steps > EDGES || e->age >= TIMEOUT))
        return false;
    if ((e->steps != 0) != (e->direction != 0))
        return false;
    if (e->teeth > e->age / (2u * QUALIFY))
        return false;
    if (e->total > e->sequence || e->tx_errors > LIMIT || e->gaps > 1u)
        return false;
    if ((e->gaps != 0) != ((e->fault & F_GAP) != 0))
        return false;
    if (e->reason > R_FAULT || (e->bad == 0) != (e->reason == R_NONE))
        return false;
    if (!e->good) {
        if (e->reference || e->minimum || e->maximum || e->total ||
            e->last || e->last_ms || e->mismatch)
            return false;
    } else {
        if (e->minimum > e->last || e->last > e->maximum)
            return false;
        if (e->minimum > e->reference || e->reference > e->maximum)
            return false;
        if (e->maximum > (TIMEOUT - 1u) / (2u * QUALIFY))
            return false;
        if (e->total < (uint64_t)e->minimum * e->good ||
            e->total > (uint64_t)e->maximum * e->good)
            return false;
        if (!e->mismatch && (e->minimum != e->reference || e->maximum != e->reference))
            return false;
        if (e->last_ms < EDGES * QUALIFY || e->last_ms >= TIMEOUT)
            return false;
    }
    return true;
}
