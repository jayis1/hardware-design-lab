/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

/* Every accepted sample is staged. Counter exhaustion must not expose half of
 * an inventory update. HALT preserves the last completed record permanently. */
void engine_init(Engine *e)
{
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint32_t fault)
{
    if (e->phase == ACTIVE) {
        /* starts is bounded, so there is room for its one outstanding abort. */
        ++e->aborted;
    }
    e->phase = HALT;
    e->fault |= fault;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors == LIMIT)
        engine_fault(e, F_OVERFLOW);
    else
        ++e->tx_errors;
}

static void qualify(Engine *e)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        uint8_t value = (e->raw & bit) != 0;
        if (!e->run[i] || value != e->candidate[i]) {
            e->candidate[i] = value;
            e->run[i] = 1;
        } else if (e->run[i] < QUALIFY) {
            ++e->run[i];
        }
        if (e->run[i] == QUALIFY) {
            e->valid |= bit;
            if (value)
                e->qualified |= bit;
            else
                e->qualified &= (uint8_t)~bit;
        }
    }
}

static bool closed_stable(const Engine *e)
{
    return e->valid == 15 && e->raw == e->qualified && (e->raw & 8u);
}

static void discard(Engine *e)
{
    ++e->aborted;
    e->phase = BASELINE;
    e->quiet = 0;
    e->age = 0;
    e->touched = 0;
    e->absent = 0;
    e->opened = 0;
}

static void complete(Engine *e)
{
    uint8_t after = e->qualified & 7u;
    ++e->completed;
    e->last_before = e->reference;
    e->last_after = after;
    e->last_touched = e->touched;
    e->last_returned = e->reference & after & e->absent;
    e->last_age = e->age;
    e->reference = after;
    e->phase = READY;
    e->age = 0;
    e->quiet = 0;
    e->touched = 0;
    e->absent = 0;
    e->opened = 0;
}

static void step(Engine *e, const Engine *old)
{
    if (old->phase == READY) {
        /* Start at raw lid departure: pocket movement during lid debounce is
         * still inside the session, not an unexplained sealed-case change. */
        if (!(e->raw & 8u)) {
            if (e->starts == LIMIT) {
                engine_fault(e, F_OVERFLOW);
                return;
            }
            ++e->starts;
            e->phase = ACTIVE;
            e->age = 0;
            e->quiet = 0;
            e->opened = 0;
            e->touched = (old->qualified ^ e->qualified) & 7u;
            e->absent = (uint8_t)(~e->qualified & 7u);
        } else if ((e->raw & 7u) != e->reference) {
            /* Do not silently learn a changed pocket behind a closed lid. */
            engine_fault(e, F_SEALED);
        }
        return;
    }
    if (old->phase == ACTIVE) {
        ++e->age;
        /* Rejecting deadline wins even if this tick could finish settlement. */
        if (e->age >= DEADLINE) {
            discard(e);
            return;
        }
        e->touched |= (old->qualified ^ e->qualified) & 7u;
        e->absent |= (uint8_t)(~e->qualified & 7u);
        if (!(e->qualified & 8u))
            e->opened = 1;
    }
    /* Interval credit requires agreement at both endpoints, including raw
     * inventory. A rejected subqualification glitch still restarts quiet. */
    if (closed_stable(e) && closed_stable(old) && e->raw == old->raw) {
        if (e->quiet < WINDOW)
            ++e->quiet;
    } else {
        e->quiet = 0;
    }
    if (e->quiet < WINDOW)
        return;
    if (e->phase == BASELINE) {
        e->reference = e->qualified & 7u;
        e->phase = READY;
        e->quiet = 0;
    } else if (e->opened) {
        complete(e);
    } else {
        /* The lid only glitched: keep a rejected session, never a completion. */
        discard(e);
    }
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->phase == HALT) {
        e->now = now;
        return;
    }
    if (raw > 15) {
        e->now = now;
        engine_fault(e, F_CONFIG);
        return;
    }
    if (e->started && now - e->now != 1u) {
        e->now = now;
        if (e->gaps < LIMIT)
            ++e->gaps;
        engine_fault(e, F_GAP);
        return;
    }
    if (e->sequence == LIMIT) {
        e->now = now;
        engine_fault(e, F_OVERFLOW);
        return;
    }
    Engine next = *e;
    next.now = now;
    next.started = true;
    next.raw = raw;
    ++next.sequence;
    qualify(&next);
    step(&next, e);
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->phase > HALT || e->fault > 31)
        return false;
    if ((e->phase == HALT) != (e->fault != 0))
        return false;
    if (e->raw > 15 || e->qualified > 15 || e->valid > 15)
        return false;
    const uint32_t counts[] = {
        e->sequence, e->starts, e->completed, e->aborted, e->gaps, e->tx_errors
    };
    for (unsigned i = 0; i < sizeof counts / sizeof counts[0]; ++i)
        if (counts[i] > LIMIT)
            return false;
    if ((uint64_t)e->completed + e->aborted + (e->phase == ACTIVE) != e->starts)
        return false;
    if (e->reference > 7 || e->touched > 7 || e->absent > 7 || e->opened > 1)
        return false;
    if (e->last_before > 7 || e->last_after > 7 || e->last_touched > 7 || e->last_returned > 7)
        return false;
    if ((e->last_before ^ e->last_after) & ~e->last_touched)
        return false;
    if (e->last_returned & ~(e->last_before & e->last_after & e->last_touched))
        return false;
    if (e->quiet > WINDOW || e->age >= DEADLINE || e->last_age >= DEADLINE)
        return false;
    if (e->phase == READY && (e->valid != 15 || !closed_stable(e) ||
                             e->reference != (e->qualified & 7u)))
        return false;
    if (e->phase == ACTIVE && e->valid != 15)
        return false;
    if (!e->completed && (e->last_before || e->last_after || e->last_touched ||
                          e->last_returned || e->last_age))
        return false;
    /* Minimum: raw opening, qualification, raw closure, qualification and
     * 100 stable closed intervals. Completion age is at least 109 samples. */
    if (e->completed && e->last_age < QUALIFY * 2u - 1u + WINDOW)
        return false;
    return true;
}
