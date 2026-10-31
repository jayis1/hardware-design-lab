/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>

/* Bounded arithmetic is transactional across BOTH holders on each sample. */
static bool add(uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount)
        return false;
    *value += amount;
    return true;
}
static bool single(unsigned pair)
{
    return pair == 1u || pair == 2u;
}
static void lose(Holder *h)
{
    /* Conservation reserves room for this abandonment at every departure. */
    if (h->state == TRANSIT)
        ++h->abandoned;
    h->state = WAIT;
    h->origin = 0;
    h->elapsed = 0;
    h->baseline = 0;
    h->candidate = 0;
}
void engine_init(Engine *e)
{
    *e = (Engine){0};
}
void engine_fault(Engine *e, uint32_t reason)
{
    if (!e->halted) {
        for (unsigned i = 0; i < 2; ++i)
            lose(&e->holder[i]);
    }
    e->halted = 1;
    e->fault |= reason;
}
void engine_tx_error(Engine *e)
{
    if (!add(&e->tx, 1))
        engine_fault(e, F_EXHAUST);
}
static void reset_qualification(Engine *e)
{
    e->known = 0;
    for (unsigned i = 0; i < 4; ++i)
        e->stable[i] = 0;
}
static void qualify(Engine *e, unsigned raw)
{
    /* Each contact qualifies independently: B chatter cannot hide A arrival. */
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (raw >> i) & 1u;
        if (!e->stable[i] || bit != e->raw_candidate[i]) {
            e->raw_candidate[i] = bit;
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY_SAMPLES) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY_SAMPLES) {
            unsigned flag = 1u << i;
            e->known |= flag;
            if (bit)
                e->mask |= flag;
            else
                e->mask &= ~flag;
        }
    }
}
static bool ambiguity(Holder *h)
{
    if (!add(&h->ambiguities, 1))
        return false;
    lose(h);
    return true;
}
static void baseline(Holder *h, unsigned pair, unsigned raw)
{
    /* First endpoint gets no retrospective elapsed time. Any raw mismatch
     * restarts the interval, even if its pulse is later filtered out. */
    if (!single(pair) || pair != raw) {
        h->baseline = 0;
        h->candidate = 0;
        return;
    }
    if (h->candidate != pair) {
        h->candidate = (uint8_t)pair;
        h->baseline = 0;
        return;
    }
    if (h->baseline < BASELINE_MS)
        ++h->baseline;
    if (h->baseline == BASELINE_MS) {
        h->state = DOCKED;
        h->origin = pair;
    }
}
static bool update_holder(Holder *h, unsigned pair, unsigned raw, uint32_t now)
{
    if (h->state == WAIT) {
        baseline(h, pair, raw);
        return true;
    }
    if (h->state == DOCKED) {
        if (pair == h->origin)
            return true;
        /* A direct dock handover or simultaneous presence is not a transfer. */
        if (pair != 0 || raw != 0)
            return ambiguity(h);
        if (!add(&h->started, 1))
            return false;
        h->state = TRANSIT;
        h->departure = now;
        h->elapsed = 0;
        return true;
    }
    h->elapsed = now - h->departure;
    /* Deadline wins over an arrival accepted on that same nominal tick. */
    if (h->elapsed >= DEADLINE_MS) {
        if (!add(&h->timeouts, 1))
            return false;
        lose(h);
        return true;
    }
    if (!pair)
        return true;
    /* No inferred ordering if raw evidence disagrees at the arrival edge. */
    if (!single(pair) || raw != pair)
        return ambiguity(h);
    bool delivered = pair != h->origin;
    if (!add(delivered ? &h->delivered : &h->returned, 1))
        return false;
    if (delivered) {
        if (!add(&h->sum_ms, h->elapsed))
            return false;
        if (h->elapsed > h->max_ms)
            h->max_ms = h->elapsed;
    }
    h->last_ms = h->elapsed;
    h->last_kind = delivered ? DELIVERED : RETURNED;
    /* Outcome counted above: do not call lose(), which would abandon it. */
    h->state = WAIT;
    h->origin = 0;
    h->elapsed = 0;
    h->baseline = 0;
    h->candidate = 0;
    return true;
}
void engine_sample(Engine *e, uint32_t now, unsigned raw)
{
    uint32_t delta = now - e->now;
    bool gap = e->initialized && delta != 1;
    e->now = now;
    e->initialized = true;
    if (raw > 15) {
        engine_fault(e, F_RAW);
        return;
    }
    if (e->halted)
        return;
    if (gap) {
        if (!add(&e->gaps, 1)) {
            engine_fault(e, F_EXHAUST);
            return;
        }
        for (unsigned i = 0; i < 2; ++i)
            lose(&e->holder[i]);
        reset_qualification(e);
        /* Preserve last qualified mask, but never treat it as currently known. */
        return;
    }
    qualify(e, raw);
    Engine next = *e;
    for (unsigned i = 0; i < 2; ++i) {
        unsigned shift = 2 * i;
        if (((e->known >> shift) & 3u) != 3u)
            continue;
        if (!update_holder(&next.holder[i], (e->mask >> shift) & 3u,
                           (raw >> shift) & 3u, now)) {
            /* Roll back all accounting, then abandon the old pending items. */
            engine_fault(e, F_EXHAUST);
            return;
        }
    }
    *e = next;
}
static bool holder_valid(const Holder *h)
{
    if (h->state > TRANSIT || h->origin > 2 || h->last_kind > RETURNED)
        return false;
    if ((uint64_t)h->started != (uint64_t)h->delivered + h->returned +
        h->abandoned + (h->state == TRANSIT))
        return false;
    if (h->state == WAIT && (h->origin || h->elapsed))
        return false;
    if (h->state != WAIT && !single(h->origin))
        return false;
    if (h->state != TRANSIT && h->elapsed)
        return false;
    if (h->elapsed >= DEADLINE_MS || h->last_ms >= DEADLINE_MS)
        return false;
    if (h->timeouts > h->abandoned)
        return false;
    if (h->delivered == 0 && (h->sum_ms || h->max_ms))
        return false;
    if (h->delivered && (!h->max_ms || h->sum_ms < h->max_ms))
        return false;
    if ((uint64_t)h->sum_ms > (uint64_t)h->delivered * h->max_ms)
        return false;
    if (h->delivered && (uint64_t)h->sum_ms < (uint64_t)h->max_ms + h->delivered - 1)
        return false;
    if (h->max_ms >= DEADLINE_MS)
        return false;
    if (!h->last_kind && (h->last_ms || h->delivered || h->returned))
        return false;
    if (h->last_kind && !h->last_ms)
        return false;
    if (h->last_kind == DELIVERED && (!h->delivered || h->last_ms > h->max_ms))
        return false;
    if (h->last_kind == RETURNED && !h->returned)
        return false;
    return true;
}
bool engine_valid(const Engine *e)
{
    if (e->mask > 15 || e->known > 15 || e->halted > 1 || e->fault > 15)
        return false;
    if (!!e->halted != !!e->fault)
        return false;
    for (unsigned i = 0; i < 2; ++i) {
        const Holder *h = &e->holder[i];
        unsigned pair = (e->mask >> (2*i)) & 3u;
        if (!holder_valid(h))
            return false;
        if (e->halted && h->state != WAIT)
            return false;
        if (h->state != WAIT && ((e->known >> (2*i)) & 3u) != 3u)
            return false;
        if (h->state == DOCKED && pair != h->origin)
            return false;
        if (h->state == TRANSIT && pair != 0)
            return false;
    }
    return true;
}
