/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

/* Offsets are measured from the first qualified opening, not raw edges.
 * The state machine deliberately observes only slow manual coupon trials. */
void engine_init(Engine *e)
{
    *e = (Engine){0};
    for (unsigned i = 0; i < 4; ++i) {
        e->at[i] = ABSENT;
        e->result.at[i] = ABSENT;
    }
}

void engine_fault(Engine *e, uint32_t flag)
{
    e->faults |= flag;
    e->state = FAULT;
    e->baseline_valid = false;
}

static bool increment(Engine *e, uint32_t *value)
{
    if (*value == UINT32_MAX) {
        engine_fault(e, F_OVERFLOW);
        return false;
    }
    ++*value;
    return true;
}

void engine_tx_error(Engine *e)
{
    (void)increment(e, &e->tx_errors);
}

static void finish(Engine *e, uint32_t reason)
{
    /* An exhausted identifier preserves the previous result unchanged. */
    if (!increment(e, &e->result.id))
        return;
    e->result.reason = reason;
    e->result.seen = e->seen;
    uint32_t elapsed = e->now - e->start;
    e->result.duration = elapsed > MAX_TRIAL_MS ? MAX_TRIAL_MS : elapsed;
    for (unsigned i = 0; i < 4; ++i)
        e->result.at[i] = e->at[i];
    e->state = WAIT;
    e->baseline_valid = false;
}

static void restart_qualification(Engine *e, uint8_t raw)
{
    e->known = 0;
    e->candidate = raw;
    e->baseline_valid = false;
    for (unsigned i = 0; i < 4; ++i)
        e->candidate_since[i] = e->now;
}

static void qualify(Engine *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw ^ e->candidate) & bit) {
            e->candidate ^= bit;
            e->candidate_since[i] = e->now;
        }
        if (e->now - e->candidate_since[i] < QUALIFY_MS)
            continue;
        e->known |= bit;
        if (e->candidate & bit)
            e->mask |= bit;
        else
            e->mask &= (uint8_t)~bit;
    }
}

static void baseline(Engine *e)
{
    /* A complete closed baseline is mandatory even after automatic reset. */
    if (e->known != 15 || e->mask != 15 || e->candidate != 15) {
        e->baseline_valid = false;
        return;
    }
    if (!e->baseline_valid) {
        e->baseline_valid = true;
        e->baseline = e->now;
        return;
    }
    if (e->now - e->baseline >= BASELINE_MS)
        e->state = READY;
}

static void begin(Engine *e)
{
    e->start = e->now;
    e->seen = 0;
    for (unsigned i = 0; i < 4; ++i)
        e->at[i] = ABSENT;
    e->state = ACTIVE;
    e->baseline_valid = false;
}

static void capture(Engine *e)
{
    /* Reclosing ANY previously observed tab invalidates this trial. */
    if (e->seen & e->mask) {
        finish(e, RECLOSE);
        return;
    }
    uint8_t opened = (uint8_t)(~e->mask & 15u);
    uint8_t fresh = (uint8_t)(opened & ~e->seen);
    uint32_t elapsed = e->now - e->start;
    for (unsigned i = 0; i < 4; ++i) {
        if (fresh & (1u << i))
            e->at[i] = elapsed;
    }
    e->seen |= fresh;
    /* Downstream may coincide with upstream at this resolution, but must
     * not precede its own side. Cross-width order is intentionally free. */
    if (((e->seen & 4u) && !(e->seen & 1u)) ||
        ((e->seen & 8u) && !(e->seen & 2u))) {
        finish(e, ORDER);
        return;
    }
    if (e->seen == 15)
        finish(e, COMPLETE);
}

void engine_sample(Engine *e, uint32_t now, uint8_t closed)
{
    if (e->state == FAULT)
        return;
    closed &= 15u;
    e->now = now;
    if (!e->initialized) {
        e->initialized = true;
        e->last = now;
        restart_qualification(e, closed);
        return;
    }
    uint32_t delta = now - e->last;
    if (!delta)
        return;
    e->last = now;
    if (delta > 2u) {
        if (e->state == ACTIVE)
            finish(e, GAP);
        if (e->state == FAULT)
            return;
        if (!increment(e, &e->gaps))
            return;
        e->state = WAIT;
        restart_qualification(e, closed);
        return;
    }
    qualify(e, closed);
    if (e->state == WAIT) {
        baseline(e);
        return;
    }
    if (e->state == READY) {
        if (e->mask == 15)
            return;
        begin(e);
    }
    if (e->state == ACTIVE) {
        /* Timeout wins over a final opening on the exact deadline. */
        if (e->now - e->start >= MAX_TRIAL_MS) {
            finish(e, TIMEOUT);
            return;
        }
        capture(e);
    }
}

bool engine_valid(const Engine *e)
{
    if (e->state > FAULT || e->mask > 15 || e->known > 15)
        return false;
    if (e->faults > 7 || ((e->state == FAULT) != (e->faults != 0)))
        return false;
    const Result *r = &e->result;
    if (r->reason > GAP || r->seen > 15 || r->duration > MAX_TRIAL_MS)
        return false;
    if ((r->id == 0) != (r->reason == NONE))
        return false;
    if (r->reason == NONE && (r->seen || r->duration))
        return false;
    uint32_t earliest = ABSENT;
    uint32_t latest = 0;
    for (unsigned i = 0; i < 4; ++i) {
        bool seen = (r->seen & (1u << i)) != 0;
        if (seen != (r->at[i] != ABSENT))
            return false;
        if (!seen)
            continue;
        if (r->at[i] > r->duration)
            return false;
        if (r->at[i] < earliest)
            earliest = r->at[i];
        if (r->at[i] > latest)
            latest = r->at[i];
    }
    if (r->reason && (!r->seen || earliest != 0))
        return false;
    if (r->reason == COMPLETE) {
        if (r->seen != 15 || latest != r->duration)
            return false;
        if (r->at[2] < r->at[0] || r->at[3] < r->at[1])
            return false;
    }
    if (r->reason == TIMEOUT && r->duration != MAX_TRIAL_MS)
        return false;
    if (r->reason == ORDER && !(((r->seen & 4) && !(r->seen & 1)) ||
                                ((r->seen & 8) && !(r->seen & 2))))
        return false;
    return true;
}
