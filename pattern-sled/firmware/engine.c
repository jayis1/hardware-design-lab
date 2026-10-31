/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint8_t fault)
{
    /* Preserve the first explanation and all completed-word evidence. */
    if (!e->fault && fault) {
        e->fault = fault;
        e->state = HALTED;
    }
}

void engine_tx_error(Engine *e)
{
    if (e->tx != UINT32_MAX)
        ++e->tx;
    else
        engine_fault(e, F_OVERFLOW);
}

static uint32_t increment_capped(uint32_t value, uint32_t cap)
{
    return value < cap ? value + 1u : cap;
}

static void complete_word(Engine *e)
{
    /* Preflight every update. Counter exhaustion cannot partly update a slot. */
    if (e->words == UINT32_MAX) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    unsigned slot = e->words % PATTERN_SIZE;
    if (e->words < PATTERN_SIZE) {
        e->reference[slot] = e->captured;
    } else {
        bool different = e->reference[slot] != e->captured;
        if (different && (e->bad == UINT32_MAX || e->errors[slot] == UINT32_MAX)) {
            engine_fault(e, F_OVERFLOW);
            return;
        }
        e->latest[slot] = e->captured;
        if (different) {
            ++e->bad;
            ++e->errors[slot];
        }
    }
    ++e->words;
    e->state = WAIT;
    e->low_ms = 0;
    e->high_ms = 0;
}

static void begin_word(Engine *e)
{
    if (e->low_ms < REARM_MS) {
        engine_fault(e, F_REARM);
        return;
    }
    if (e->data_ms < SETUP_MS) {
        engine_fault(e, F_SETUP);
        return;
    }
    e->captured = e->raw >> 1;
    e->high_ms = 0;
    e->state = HIGH;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t elapsed = now - e->now;
    e->now = now;
    if (raw > 15u) {
        engine_fault(e, F_CONFIG);
        return;
    }
    e->raw = raw;
    if (e->fault)
        return;
    if (!e->initialized) {
        /* Attachment during a held clock does not invent its rising edge. */
        e->initialized = true;
        e->previous = raw;
        return;
    }
    if (elapsed != 1u) {
        engine_fault(e, F_GAP);
        return;
    }
    uint8_t old = e->previous;
    bool clock = (raw & 1u) != 0;
    bool was_clock = (old & 1u) != 0;
    bool stable_data = (raw >> 1) == (old >> 1);
    e->previous = raw;
    if (stable_data)
        e->data_ms = increment_capped(e->data_ms, SETUP_MS);
    else
        e->data_ms = 0;

    if (e->state == HIGH) {
        /* Hold includes the falling endpoint. Data and clock changing together
         * cannot demonstrate an ordered release and is rejected conservatively. */
        if ((raw >> 1) != e->captured) {
            engine_fault(e, F_HOLD);
            return;
        }
        e->high_ms = increment_capped(e->high_ms, MAX_HIGH_MS);
        /* Deadline wins over a simultaneous falling edge. */
        if (e->high_ms >= MAX_HIGH_MS) {
            engine_fault(e, F_LONG);
            return;
        }
        if (!clock) {
            if (e->high_ms < MIN_HIGH_MS)
                engine_fault(e, F_SHORT);
            else
                complete_word(e);
        }
        return;
    }

    if (!clock) {
        if (!was_clock)
            e->low_ms = increment_capped(e->low_ms, REARM_MS);
        else
            e->low_ms = 0;
        if (e->low_ms >= REARM_MS)
            e->state = READY;
        return;
    }
    if (!was_clock) {
        /* Measure falling-to-rising duration, including its final interval.
         * No interval before the first sampled low endpoint is credited. */
        e->low_ms = increment_capped(e->low_ms, REARM_MS);
        begin_word(e);
        return;
    }
    /* Initial high attachment remains WAIT until a complete low baseline. */
    e->low_ms = 0;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->raw > 15u || e->state > HALTED || e->fault > F_OVERFLOW)
        return false;
    if ((e->state == HALTED) != (e->fault != F_NONE))
        return false;
    if (e->high_ms > MAX_HIGH_MS)
        return false;
    if (e->state == HIGH && (!(e->raw & 1u) || e->high_ms >= MAX_HIGH_MS))
        return false;
    if ((e->state == READY || e->state == WAIT) && e->high_ms)
        return false;
    if (e->state == READY && (e->raw & 1u))
        return false;
    uint64_t total = 0;
    uint32_t compared = e->words > PATTERN_SIZE ? e->words - PATTERN_SIZE : 0;
    for (unsigned i = 0; i < PATTERN_SIZE; ++i) {
        if (e->reference[i] > 7u || e->latest[i] > 7u)
            return false;
        if (e->words <= i && e->reference[i])
            return false;
        uint32_t visits = compared / PATTERN_SIZE + (i < compared % PATTERN_SIZE);
        if (e->errors[i] > visits)
            return false;
        if (!visits && e->latest[i])
            return false;
        if (visits && e->latest[i] != e->reference[i] && !e->errors[i])
            return false;
        if (visits && e->latest[i] == e->reference[i] && e->errors[i] == visits)
            return false;
        total += e->errors[i];
    }
    return total == e->bad;
}
