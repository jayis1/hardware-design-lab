/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint8_t reason)
{
    /* Keep the first cause and all accepted evidence. DONE is immutable. */
    if (e->phase == DONE || e->phase == HALT)
        return;
    e->fault = reason;
    e->phase = HALT;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors != UINT32_MAX)
        ++e->tx_errors;
}

static void qualify(Engine *e, uint8_t raw)
{
    /* Independent channels: a chattering peer cannot postpone a closure. */
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        uint8_t value = (raw & bit) != 0;
        if (!e->stable[i] || value != e->candidate[i]) {
            e->candidate[i] = value;
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY_SAMPLES) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY_SAMPLES) {
            e->valid |= bit;
            if (value)
                e->mask |= bit;
            else
                e->mask &= (uint8_t)~bit;
        }
    }
}

static bool all_settled(const Engine *e)
{
    for (unsigned i = 0; i < 3; ++i) {
        if (e->lane[i].quiet != SETTLE_MS)
            return false;
    }
    return true;
}

static void wait_baseline(Engine *e)
{
    bool clean = e->valid == 15 && e->mask == 8 && e->raw == 8;
    if (clean && e->prior_baseline) {
        if (e->baseline < BASELINE_MS)
            ++e->baseline;
    } else {
        e->baseline = 0;
    }
    e->prior_baseline = clean;
    if (e->baseline == BASELINE_MS)
        e->phase = READY;
}

static void await_release(Engine *e)
{
    /* A return preceding the accepted release origin has no measurable
     * nonnegative latency. Reject rather than silently reporting zero. */
    if ((e->raw & 7u) || (e->mask & 7u)) {
        engine_fault(e, F_PRECLOSED);
        return;
    }
    if (!(e->mask & 8u) && !(e->raw & 8u)) {
        e->phase = ACTIVE;
        e->age = 0;
        e->prior_agree = 0;
    }
}

static bool capture_edges(Engine *e, uint8_t previous)
{
    /* Commit all channels atomically. A capacity failure must not retain
     * half of a simultaneous sample. */
    Lane next[3];
    for (unsigned i = 0; i < 3; ++i)
        next[i] = e->lane[i];
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        bool before = (previous & bit) != 0;
        bool after = (e->mask & bit) != 0;
        if (!before && after) {
            if (next[i].closures == MAX_CLOSURES)
                return false;
            if (!next[i].closures)
                next[i].first = e->age;
            ++next[i].closures;
            next[i].last = e->age;
        }
        if (before && !after)
            ++next[i].reopens;
        bool agree = (e->raw & e->mask & bit) != 0;
        if (agree && (e->prior_agree & bit) && before) {
            if (next[i].quiet < SETTLE_MS)
                ++next[i].quiet;
        } else {
            next[i].quiet = 0;
        }
    }
    for (unsigned i = 0; i < 3; ++i)
        e->lane[i] = next[i];
    e->prior_agree = e->raw & e->mask & 7u;
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    raw &= 15u;
    if (e->clocked && now == e->now)
        return;
    uint32_t elapsed = now - e->now;
    bool gap = e->clocked && elapsed != 1u;
    e->clocked = true;
    e->now = now;
    e->raw = raw;
    if (e->phase == DONE || e->phase == HALT)
        return;
    if (gap) {
        engine_fault(e, F_GAP);
        return;
    }
    uint8_t previous = e->mask;
    qualify(e, raw);
    switch (e->phase) {
    case WAIT:
        wait_baseline(e);
        break;
    case READY:
        await_release(e);
        break;
    case ACTIVE:
        ++e->age;
        /* An expired trial cannot earn a simultaneous final settlement. */
        if (e->age >= DEADLINE_MS) {
            engine_fault(e, F_TIMEOUT);
            break;
        }
        if (raw & 8u) {
            engine_fault(e, F_REHELD);
            break;
        }
        if (!capture_edges(e, previous)) {
            /* Restore the accepted occupancy when no edge was committed. */
            e->mask = previous;
            engine_fault(e, F_OVERFLOW);
            break;
        }
        if (all_settled(e))
            e->phase = DONE;
        break;
    default:
        engine_fault(e, F_CONFIG);
        break;
    }
}

static bool lane_valid(const Lane *p, uint32_t age)
{
    if (p->closures > MAX_CLOSURES || p->reopens > p->closures)
        return false;
    if (p->closures - p->reopens > 1u)
        return false;
    if (p->quiet > SETTLE_MS)
        return false;
    if (!p->closures)
        return !p->first && !p->last && !p->reopens && !p->quiet;
    if (p->first < QUALIFY_SAMPLES || p->first > p->last)
        return false;
    if (p->last > age)
        return false;
    if (p->closures == 1 && p->first != p->last)
        return false;
    /* A closure/reopen/closure round trip needs two qualification windows. */
    if (p->last - p->first < (p->closures - 1u) * 2u * QUALIFY_SAMPLES)
        return false;
    if (p->quiet > age - p->last)
        return false;
    if (p->quiet && p->closures == p->reopens)
        return false;
    return true;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->phase > HALT || e->fault > F_REHELD)
        return false;
    if ((e->phase == HALT) != (e->fault != F_NONE))
        return false;
    if (e->raw > 15 || e->mask > 15 || e->valid > 15)
        return false;
    if (e->age > DEADLINE_MS || e->baseline > BASELINE_MS)
        return false;
    if (e->phase == ACTIVE && e->age >= DEADLINE_MS)
        return false;
    if (e->phase == DONE && (e->age >= DEADLINE_MS || !all_settled(e)))
        return false;
    if ((e->phase == READY || e->phase == ACTIVE || e->phase == DONE) &&
        (e->valid != 15 || e->baseline != BASELINE_MS))
        return false;
    if (e->phase <= READY && e->age)
        return false;
    for (unsigned i = 0; i < 3; ++i) {
        if (!lane_valid(&e->lane[i], e->age))
            return false;
        if (e->phase <= READY && e->lane[i].closures)
            return false;
        /* On HALT the fault tick may qualify an uncommitted edge. */
        if (e->phase == ACTIVE || e->phase == DONE) {
            uint32_t occupied = (e->mask >> i) & 1u;
            if (e->lane[i].closures - e->lane[i].reopens != occupied)
                return false;
        }
    }
    if ((e->phase == ACTIVE || e->phase == DONE) && (e->mask & 8u))
        return false;
    return true;
}
