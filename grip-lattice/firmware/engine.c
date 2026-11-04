/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

/* No heap or persistent storage. A reset starts a new, explicitly volatile study. */
void engine_init(Engine *e)
{
    unsigned char *bytes = (unsigned char *)e;
    for (size_t i = 0; i < sizeof *e; ++i)
        bytes[i] = 0;
    e->phase = WAIT;
    e->bin = NO_BIN;
}

static bool increment(uint32_t *value)
{
    if (*value == UINT32_MAX)
        return false;
    ++*value;
    return true;
}

void engine_fault(Engine *e, uint8_t reason)
{
    if (e->phase == HALT)
        return;
    /* An active start reserves one representable completion/abandonment. */
    if (e->phase == ACTIVE)
        ++e->aborted;
    e->phase = HALT;
    e->fault = reason;
    e->baseline = 0;
    e->prior_low = false;
    e->bin = NO_BIN;
    e->streak = 0;
}

void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx_errors))
        engine_fault(e, F_OVERFLOW);
}

static void qualify(Engine *e, uint8_t raw)
{
    /* Independent channels prevent unrelated finger chatter delaying RECORD. */
    e->raw = raw;
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        uint8_t value = (uint8_t)((raw >> i) & 1u);
        if (!e->count[i] || value != e->candidate[i]) {
            e->candidate[i] = value;
            e->count[i] = 1;
        } else if (e->count[i] < QUALIFY_SAMPLES) {
            ++e->count[i];
        }
        if (e->count[i] == QUALIFY_SAMPLES) {
            e->valid |= bit;
            if (value)
                e->mask |= bit;
            else
                e->mask &= (uint8_t)~bit;
        }
    }
}

static void clear_trial(Engine *e)
{
    e->age = 0;
    e->unknown = 0;
    e->streak = 0;
    e->bin = NO_BIN;
    for (unsigned i = 0; i < 8; ++i) {
        e->hist[i] = 0;
        e->runs[i] = 0;
        e->longest[i] = 0;
    }
}

static void baseline(Engine *e)
{
    bool low = (e->valid & 8u) && !(e->mask & 8u) && !(e->raw & 8u);
    if (low && e->prior_low) {
        if (e->baseline < BASELINE_MS)
            ++e->baseline;
    } else {
        e->baseline = 0;
    }
    e->prior_low = low;
    if (e->baseline == BASELINE_MS)
        e->phase = READY;
}

static void interval(Engine *e, uint8_t previous_raw, uint8_t previous_mask,
                     uint8_t previous_valid)
{
    /* Credit only an interval whose two ends agree in every contact.
     * A raw glitch splits runs even when it never changes qualified occupancy.
     * Rejected/ambiguous time is retained, never normalized out of the trial. */
    ++e->age;
    bool known = previous_valid == 15u && e->valid == 15u;
    known = known && previous_raw == previous_mask;
    known = known && e->raw == e->mask && previous_mask == e->mask;
    known = known && (e->raw & 8u);
    if (!known) {
        ++e->unknown;
        e->bin = NO_BIN;
        e->streak = 0;
        return;
    }
    uint8_t bin = e->mask & 7u;
    ++e->hist[bin];
    if (e->bin != bin) {
        ++e->runs[bin];
        e->streak = 0;
        e->bin = bin;
    }
    ++e->streak;
    if (e->streak > e->longest[bin])
        e->longest[bin] = e->streak;
}

static bool update(Engine *e, uint32_t now, uint8_t raw)
{
    uint8_t previous_raw = e->raw;
    uint8_t previous_mask = e->mask;
    uint8_t previous_valid = e->valid;
    e->now = now;
    if (!increment(&e->samples))
        return false;
    qualify(e, raw);
    if (e->phase == ACTIVE) {
        interval(e, previous_raw, previous_mask, previous_valid);
        /* Deadline wins even when release qualifies on this exact sample. */
        if (e->age >= DEADLINE_MS) {
            engine_fault(e, F_TIMEOUT);
            return true;
        }
        if (!(e->mask & 8u)) {
            if (!increment(&e->completed))
                return false;
            e->phase = DONE;
            e->baseline = 0;
            e->prior_low = false;
            e->bin = NO_BIN;
            e->streak = 0;
        }
        return true;
    }
    if (e->phase == READY && (e->valid & 8u) && (e->mask & 8u)) {
        if (!increment(&e->starts))
            return false;
        clear_trial(e);
        e->phase = ACTIVE;
        e->baseline = 0;
        e->prior_low = false;
        return true;
    }
    baseline(e);
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->phase == HALT) {
        /* Status time is wall-clock progress, not valid sensor acquisition. */
        e->now = now;
        return;
    }
    if (raw > 15u) {
        e->now = now;
        engine_fault(e, F_CONFIG);
        return;
    }
    if (e->clocked && now - e->now != 1u) {
        e->now = now;
        engine_fault(e, F_GAP);
        return;
    }
    Engine next = *e;
    next.clocked = true;
    if (!update(&next, now, raw)) {
        /* Roll back all speculative counters before abandoning prior work. */
        e->now = now;
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->phase > HALT || e->fault > F_TIMEOUT)
        return false;
    if ((e->phase == HALT) != (e->fault != F_NONE))
        return false;
    if (e->raw > 15u || e->mask > 15u || e->valid > 15u)
        return false;
    if (e->age > DEADLINE_MS || e->unknown > e->age)
        return false;
    if (e->baseline > BASELINE_MS || e->bin > NO_BIN)
        return false;
    uint64_t starts = (uint64_t)e->completed + e->aborted + (e->phase == ACTIVE);
    if (starts != e->starts)
        return false;
    if (e->phase == ACTIVE && (e->age >= DEADLINE_MS || !(e->mask & 8u)))
        return false;

    if (e->phase == WAIT && e->starts)
        return false;
    if (e->phase == DONE && !e->completed)
        return false;
    if (e->phase == HALT && (e->baseline || e->streak || e->bin != NO_BIN))
        return false;
    uint64_t total = e->unknown;
    for (unsigned i = 0; i < 8; ++i) {
        uint32_t h = e->hist[i], r = e->runs[i], m = e->longest[i];
        total += h;
        if (h > e->age || r > h || m > h)
            return false;
        if ((h == 0) != (r == 0) || (h == 0) != (m == 0))
            return false;
        if (h && ((uint64_t)m + r - 1u > h || (uint64_t)m * r < h))
            return false;
    }
    if (total != e->age)
        return false;
    if (!e->starts && e->age)
        return false;
    if (e->bin == NO_BIN && e->streak)
        return false;
    if (e->bin < NO_BIN && (!e->streak || e->streak > e->longest[e->bin]))
        return false;
    return true;
}
