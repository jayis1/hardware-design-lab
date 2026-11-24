/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

/* Counters latch an overflow fault rather than silently becoming plausible
 * small numbers. No partial reference is ever published. */
static bool add(Engine *e, uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount) {
        e->fault |= F_OVERFLOW;
        e->active = 0;
        e->armed = 0;
        return false;
    }
    *value += amount;
    return true;
}

void engine_init(Engine *e)
{
    if (!e)
        return;
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint32_t fault)
{
    if (!e)
        return;
    e->fault |= fault;
    e->active = 0;
    e->armed = 0;
    e->result = RESULT_ABORT;
}

void engine_tx_error(Engine *e)
{
    if (e)
        (void)add(e, &e->tx_errors, 1);
}

static void abort_capture(Engine *e)
{
    if (e->active)
        (void)add(e, &e->aborted, 1);
    e->active = 0;
    e->armed = 0;
    e->result = RESULT_ABORT;
    e->released = false;
}

static void begin(Engine *e, uint32_t word)
{
    if (!e->armed || (word & PAD_MASK))
        return;
    e->recording = (word & RECORD_MASK) != 0;
    if (!e->recording && !e->reference_count) {
        /* An empty reference is not an empty matching performance. */
        e->armed = 0;
        e->result = RESULT_ABORT;
        return;
    }
    e->active = 1;
    e->armed = 0;
    e->started = e->now;
    e->last_onset = e->now;
    e->age = 0;
    e->count = 0;
    e->wrong = 0;
    e->max_error = 0;
    e->result = RESULT_NONE;
    e->released = true;
    e->envelope = 0;
    for (unsigned i = 0; i < CAPACITY; ++i)
        e->events[i] = (Event){0};
}

static uint32_t distance(uint32_t a, uint32_t b)
{
    return a > b ? a - b : b - a;
}

static void compare(Engine *e)
{
    e->wrong = distance(e->count, e->reference_count);
    e->max_error = 0;
    for (unsigned i = 0; i < e->count && i < e->reference_count; ++i) {
        bool bad = e->events[i].pad != e->reference[i].pad;
        if (i) {
            uint32_t error = distance(e->events[i].interval,
                                      e->reference[i].interval);
            uint32_t tolerance = e->reference[i].interval / 5u;
            if (tolerance < 30u)
                tolerance = 30u;
            if (error > e->max_error)
                e->max_error = error;
            if (error > tolerance)
                bad = true;
        }
        if (bad)
            ++e->wrong;
    }
}

static void finish(Engine *e, uint32_t word)
{
    if ((word & PAD_MASK) || !e->released || e->count < 2) {
        abort_capture(e);
        return;
    }
    if (e->recording) {
        if (!add(e, &e->references, 1))
            return;
        for (unsigned i = 0; i < CAPACITY; ++i)
            e->reference[i] = e->events[i];
        e->reference_count = e->count;
        e->result = RESULT_REFERENCE;
    } else {
        compare(e);
        /* Preflight both counters so a failed increment cannot publish half
         * of a successful comparison. */
        if (e->trials == UINT32_MAX || (!e->wrong && e->matches == UINT32_MAX)) {
            engine_fault(e, F_OVERFLOW);
            return;
        }
        ++e->trials;
        if (!e->wrong) {
            ++e->matches;
            e->result = RESULT_MATCH;
        } else {
            e->result = RESULT_MISMATCH;
        }
    }
    e->active = 0;
    e->armed = 0;
}

static void onset(Engine *e, uint32_t pads)
{
    if (!pads) {
        e->released = true;
        e->envelope = 0;
        return;
    }
    if (!e->released) {
        /* A-to-B without an observed all-clear is not two distinct taps. */
        if (e->count && e->events[e->count - 1].pad != pads)
            abort_capture(e);
        return;
    }
    if (pads == PAD_MASK || e->count == CAPACITY) {
        abort_capture(e);
        return;
    }
    Event event = {pads, e->count ? e->now - e->last_onset : 0};
    e->events[e->count++] = event;
    e->last_onset = e->now;
    e->released = false;
}

static void qualified_word(Engine *e, uint32_t word)
{
    uint32_t previous = e->qualified;
    e->qualified = word;
    e->valid = 1;
    if (!e->active) {
        if (!(word & (PAD_MASK | GATE_MASK)))
            e->armed = 1;
        else if ((word & GATE_MASK) && !(previous & GATE_MASK))
            begin(e, word);
        else if (word & PAD_MASK)
            e->armed = 0;
        return;
    }
    if (!(word & GATE_MASK)) {
        finish(e, word);
        return;
    }
    onset(e, word & PAD_MASK);
}

void engine_sample(Engine *e, uint32_t now, uint32_t raw)
{
    if (!e)
        return;
    uint32_t elapsed = now - e->now;
    e->now = now;
    e->raw = raw;
    if (!add(e, &e->samples, 1))
        return;
    if (raw > 15u) {
        engine_fault(e, F_INPUT);
        return;
    }
    if (e->fault)
        return;
    if (!e->seeded) {
        e->seeded = true;
        e->candidate = raw;
        e->stable = 0;
        return;
    }
    if (elapsed != 1u) {
        /* A duplicate timestamp, backward clock, or missed observation loses
         * continuity. No interpolation is allowed to invent tap events. */
        if (elapsed > 0u && elapsed < 0x80000000u)
            (void)add(e, &e->missed, elapsed - 1u);
        else
            engine_fault(e, F_TIME);
        abort_capture(e);
        e->valid = 0;
        e->candidate = raw;
        e->stable = 0;
        return;
    }
    if (e->active) {
        e->age = now - e->started;
        bool changed_mode = ((raw & RECORD_MASK) != 0) != (e->recording != 0);
        e->envelope |= raw & PAD_MASK;
        bool both = e->envelope == PAD_MASK;
        if (changed_mode || both) {
            abort_capture(e);
        } else if (e->age >= DEADLINE) {
            (void)add(e, &e->timeouts, 1);
            abort_capture(e);
        }
    }
    if (raw != e->candidate) {
        e->candidate = raw;
        e->stable = 0;
        return;
    }
    if (e->stable < QUALIFY)
        ++e->stable;
    if (e->stable == QUALIFY)
        qualified_word(e, raw);
}

bool engine_valid(const Engine *e)
{
    if (!e || e->reference_count > CAPACITY || e->count > CAPACITY)
        return false;
    if (e->raw > 15u && !(e->fault & F_INPUT))
        return false;
    if (e->qualified > 15u || e->result > RESULT_ABORT)
        return false;
    if (e->active > 1u || e->armed > 1u || e->recording > 1u || e->valid > 1u)
        return false;
    if ((e->active && e->armed) || (e->fault && e->active))
        return false;
    if (e->matches > e->trials || e->wrong > CAPACITY)
        return false;
    for (unsigned i = 0; i < CAPACITY; ++i) {
        if (i < e->reference_count) {
            if (e->reference[i].pad != 1u && e->reference[i].pad != 2u)
                return false;
        } else if (e->reference[i].pad || e->reference[i].interval) {
            return false;
        }
        if (i < e->count) {
            if (e->events[i].pad != 1u && e->events[i].pad != 2u)
                return false;
        } else if (e->events[i].pad || e->events[i].interval) {
            return false;
        }
        if (e->reference[i].interval > DEADLINE || e->events[i].interval > DEADLINE)
            return false;
    }
    return true;
}
