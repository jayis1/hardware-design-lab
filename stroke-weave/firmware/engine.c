/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

static bool traveling(const Lane *p)
{
    return p->phase == OUTBOUND || p->phase == RETURNING;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint32_t fault)
{
    if (e->state == HALT)
        return;
    /* Retain last completed duration, but abandon every pending excursion.
     * starts bounds abandoned, leaving room even on counter exhaustion. */
    for (unsigned i = 0; i < 2; ++i) {
        if (traveling(&e->lane[i]))
            ++e->lane[i].abandoned;
        e->lane[i].phase = UNKNOWN;
    }
    e->fault = fault;
    e->state = HALT;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors < LIMIT)
        ++e->tx_errors;
}

static void qualify(Engine *e, uint8_t raw)
{
    e->raw = raw;
    /* Independent contacts avoid one chattering piston masking its peer.
     * Opposing qualified contacts are a conservative geometry fault. */
    for (unsigned i = 0; i < 4; ++i) {
        unsigned bit = 1u << i;
        unsigned level = (raw & bit) != 0;
        if (!e->stable[i] || e->candidate[i] != level) {
            e->candidate[i] = (uint8_t)level;
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY) {
            e->valid |= bit;
            e->qualified = (e->qualified & ~bit) | (level ? bit : 0);
        }
    }
}

static uint32_t advance_lane(Lane *p, unsigned q, unsigned raw, bool *delivery)
{
    *delivery = false;
    if (q == 3)
        return F_GEOMETRY;
    if (traveling(p)) {
        ++p->age;
        /* A tied endpoint cannot rescue an already expired stroke. */
        if (p->age >= DEADLINE)
            return F_DEADLINE;
        if (!q)
            return 0;
        if (q != raw)
            return F_BOUNDARY;
        unsigned origin = p->phase == OUTBOUND ? 1u : 2u;
        if (q == origin) {
            ++p->canceled;
        } else {
            p->last_ms = p->age;
            if (origin == 1) {
                ++p->deliveries;
                *delivery = true;
            } else {
                ++p->refills;
            }
        }
        p->phase = q == 1 ? HOME : END;
        p->age = 0;
        return 0;
    }
    if (p->phase == UNKNOWN) {
        /* Midstroke attachment is not a completed or started stroke. */
        if (q && raw == q)
            p->phase = q == 1 ? HOME : END;
        return 0;
    }
    unsigned expected = p->phase == HOME ? 1u : 2u;
    if (q == expected)
        return 0;
    /* Require an observed clear region, never interpolate a direct swap. */
    if (q || raw)
        return F_BOUNDARY;
    ++p->starts;
    p->phase = expected == 1 ? OUTBOUND : RETURNING;
    p->age = 0;
    return 0;
}

static void delivery_order(Engine *e, bool a, bool b)
{
    if (a && b) {
        /* No imposed A-before-B order for the same acquired endpoint. */
        ++e->ties;
        e->anchor = 0;
        e->streak = 0;
        return;
    }
    if (!a && !b)
        return;
    unsigned side = a ? 1u : 2u;
    if (e->anchor == side) {
        ++e->repeated;
        ++e->streak;
    } else {
        if (e->anchor)
            ++e->alternating;
        e->streak = 1;
    }
    e->anchor = side;
    if (e->streak > e->peak)
        e->peak = e->streak;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->state == HALT) {
        e->now = now;
        return;
    }
    uint32_t delta = now - e->now;
    e->now = now;
    if (raw > 15) {
        engine_fault(e, F_CONFIG);
        return;
    }
    if (e->seen && delta != 1) {
        e->gaps = 1;
        engine_fault(e, F_GAP);
        return;
    }
    e->seen = true;
    if (e->sequence == UINT32_MAX) {
        engine_fault(e, F_COUNTER);
        return;
    }
    ++e->sequence;
    qualify(e, raw);
    /* Stage both lanes and order together. A fault on B must not silently
     * publish a successful A stroke from the same ambiguous observation. */
    Engine next = *e;
    bool delivery[2] = {false, false};
    for (unsigned i = 0; i < 2; ++i) {
        unsigned mask = 3u << (2u * i);
        if ((e->valid & mask) != mask)
            continue;
        uint32_t fault = advance_lane(&next.lane[i],
            (e->qualified >> (2u * i)) & 3u,
            (raw >> (2u * i)) & 3u, &delivery[i]);
        if (fault) {
            engine_fault(e, fault);
            return;
        }
    }
    delivery_order(&next, delivery[0], delivery[1]);
    if (!engine_valid(&next)) {
        engine_fault(e, F_COUNTER);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->state > HALT || e->fault > F_GEOMETRY)
        return false;
    if ((e->state == HALT) != (e->fault != 0))
        return false;
    if (e->fault && (e->fault & (e->fault - 1)))
        return false;
    if (e->raw > 15 || e->qualified > 15 || e->valid > 15)
        return false;
    if (e->gaps > 1 || (e->gaps && e->fault != F_GAP))
        return false;
    if (e->tx_errors > LIMIT || e->anchor > 2)
        return false;
    uint64_t delivered = 0;
    for (unsigned i = 0; i < 2; ++i) {
        const Lane *p = &e->lane[i];
        if (p->phase > RETURNING || p->starts > LIMIT)
            return false;
        if (p->deliveries > LIMIT || p->refills > LIMIT ||
            p->canceled > LIMIT || p->abandoned > LIMIT)
            return false;
        uint64_t outcomes = (uint64_t)p->deliveries + p->refills + p->canceled + p->abandoned;
        if (p->starts != outcomes + traveling(p))
            return false;
        /* Full strokes alternate direction even if origins differ at boot. */
        if (p->deliveries > p->refills + 1u || p->refills > p->deliveries + 1u)
            return false;
        if (p->abandoned > 1u || (p->abandoned && e->state != HALT))
            return false;
        if (e->state == HALT && !p->abandoned && p->age)
            return false;
        if (p->age >= DEADLINE || p->last_ms >= DEADLINE)
            return false;
        if (p->last_ms && p->last_ms < QUALIFY)
            return false;
        if ((p->deliveries || p->refills) != (p->last_ms != 0))
            return false;
        if (e->state == HALT && p->phase != UNKNOWN)
            return false;
        if (e->state != HALT) {
            if (p->deliveries > p->refills && p->phase != END && p->phase != RETURNING)
                return false;
            if (p->refills > p->deliveries && p->phase != HOME && p->phase != OUTBOUND)
                return false;
            unsigned q = (e->qualified >> (2u * i)) & 3u;
            unsigned valid = (e->valid >> (2u * i)) & 3u;
            if (p->phase && valid != 3)
                return false;
            if (p->phase == HOME && q != 1)
                return false;
            if (p->phase == END && q != 2)
                return false;
            if (traveling(p) && q)
                return false;
            if (!traveling(p) && p->age)
                return false;
        }
        delivered += p->deliveries;
    }
    if (e->ties > e->lane[0].deliveries || e->ties > e->lane[1].deliveries)
        return false;
    uint64_t singles = delivered - 2u * e->ties;
    if ((uint64_t)e->alternating + e->repeated + (e->anchor != 0) > singles)
        return false;
    if (e->anchor && !e->lane[e->anchor - 1].deliveries)
        return false;
    if ((e->anchor != 0) != (e->streak != 0))
        return false;
    if (e->streak > e->peak || e->peak > singles)
        return false;
    if (e->peak && e->peak > (uint64_t)e->repeated + 1)
        return false;
    if (singles && !e->peak)
        return false;
    return true;
}
