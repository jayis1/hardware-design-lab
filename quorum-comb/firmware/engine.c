/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

static bool add(uint32_t *value, uint32_t n)
{
    if (UINT32_MAX - *value < n)
        return false;
    *value += n;
    return true;
}

static unsigned vote(unsigned word)
{
    unsigned count = 0;
    for (unsigned i = 0; i < 3; ++i)
        count += (word >> i) & 1u;
    return count >= 2;
}

static void clear_runs(Engine *e)
{
    for (unsigned i = 0; i < 5; ++i)
        e->run[i] = 0;
}

void engine_init(Engine *e)
{
    /* Byte initialization avoids a freestanding libc dependency. */
    unsigned char *p = (unsigned char *)e;
    for (size_t i = 0; i < sizeof *e; ++i)
        p[i] = 0;
}

void engine_fault(Engine *e, unsigned fault)
{
    if (!e->fault)
        e->fault = (uint8_t)fault;
    e->valid = 0;
    clear_runs(e);
}

void engine_tx_error(Engine *e)
{
    if (e->fault)
        return;
    if (!add(&e->tx_errors, 1))
        engine_fault(e, F_OVERFLOW);
}

/* Return true only when both interval endpoints are fully trustworthy for
 * this subset. Unrelated reference chatter cannot hide voter disagreement. */
static bool clean(const Engine *before, const Engine *after, unsigned mask)
{
    return (before->valid & mask) == mask &&
           (after->valid & mask) == mask &&
           ((before->raw ^ before->qualified) & mask) == 0 &&
           ((after->raw ^ after->qualified) & mask) == 0 &&
           ((before->raw ^ after->raw) & mask) == 0;
}

static void qualify(Engine *e, unsigned raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        unsigned bit = 1u << i;
        if ((raw ^ e->candidate) & bit) {
            e->candidate ^= bit;
            e->age[i] = 0;
        } else if (e->age[i] < 5) {
            ++e->age[i];
        }
        if (e->age[i] == 5) {
            e->qualified = (uint8_t)((e->qualified & ~bit) | (raw & bit));
            e->valid |= bit;
        }
    }
    e->raw = (uint8_t)raw;
}

static bool credit(Engine *e, unsigned i, bool active)
{
    if (!active) {
        e->run[i] = 0;
        return true;
    }
    if (!e->run[i] && !add(&e->segments[i], 1))
        return false;
    if (!add(&e->total[i], 1) || !add(&e->run[i], 1))
        return false;
    if (e->run[i] > e->peak[i])
        e->peak[i] = e->run[i];
    return true;
}

void engine_sample(Engine *e, uint32_t now, unsigned raw)
{
    /* Terminal evidence is immutable, including time. Reconnect/reset is
     * deliberate; a new advancing packet cannot revive halted acquisition. */
    if (e->fault)
        return;
    if (raw > 15) {
        engine_fault(e, F_INPUT);
        return;
    }
    uint32_t dt = now - e->now;
    if (e->started && (!dt || dt >= 0x80000000u)) {
        engine_fault(e, F_TIME);
        return;
    }
    Engine next = *e;
    next.now = now;
    if (!add(&next.samples, 1))
        goto overflow;
    if (!e->started || dt != 1) {
        if (e->started) {
            if (!add(&next.gaps, 1) || !add(&next.missed, dt - 1))
                goto overflow;
        }
        next.started = 1;
        next.raw = next.candidate = (uint8_t)raw;
        next.valid = 0;
        for (unsigned i = 0; i < 4; ++i)
            next.age[i] = 0;
        clear_runs(&next);
        *e = next;
        return;
    }
    if (!add(&next.intervals, 1))
        goto overflow;
    qualify(&next, raw);
    bool voters = clean(e, &next, 7);
    bool reference = clean(e, &next, 15);
    if (voters && !add(&next.voter_known, 1))
        goto overflow;
    if (reference && !add(&next.compare_known, 1))
        goto overflow;
    unsigned majority = vote(raw);
    for (unsigned i = 0; i < 5; ++i) {
        bool active;
        if (i < 3)
            active = voters && (((raw >> i) & 1u) != majority);
        else if (i == 3)
            active = reference && majority && !(raw & 8u);
        else
            active = reference && !majority && (raw & 8u);
        if (!credit(&next, i, active))
            goto overflow;
    }
    *e = next;
    return;
overflow:
    /* Roll back every speculative counter, then abandon only current runs. */
    engine_fault(e, F_OVERFLOW);
}

bool engine_valid(const Engine *e)
{
    if (e->raw > 15 || e->qualified > 15 || e->valid > 15 ||
        e->fault > F_WATCHDOG || (e->fault && e->valid))
        return false;
    if (e->intervals > e->samples || e->gaps > e->missed ||
        (uint64_t)e->intervals + e->gaps + (e->samples != 0) != e->samples)
        return false;
    if (e->compare_known > e->voter_known || e->voter_known > e->intervals)
        return false;
    uint64_t minority = 0, mismatch = 0;
    for (unsigned i = 0; i < 5; ++i) {
        uint32_t total = e->total[i], count = e->segments[i];
        uint32_t peak = e->peak[i], run = e->run[i];
        if ((!total) != (!count) || (!total) != (!peak))
            return false;
        if (run > peak || peak > total || count > total || (e->fault && run))
            return false;
        if (run) {
            unsigned mask = i < 3 ? 7u : 15u;
            if ((e->valid & mask) != mask ||
                ((e->raw ^ e->qualified) & mask))
                return false;
            unsigned majority = vote(e->raw);
            bool active = i < 3 ? (((e->raw >> i) & 1u) != majority) :
                (i == 3 ? majority && !(e->raw & 8u) : !majority && (e->raw & 8u));
            if (!active)
                return false;
        }
        if (count && ((uint64_t)peak + count - 1 > total ||
                      (uint64_t)peak * count < total))
            return false;
        if (i < 3)
            minority += total;
        else
            mismatch += total;
    }
    return minority <= e->voter_known && mismatch <= e->compare_known;
}
