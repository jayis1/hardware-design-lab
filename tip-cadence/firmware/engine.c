/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>

/* No sample can partly commit multiple counters: the caller stages a copy. */
static bool add(uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount)
        return false;
    *value += amount;
    return true;
}

static uint32_t age_next(uint32_t age)
{
    return age < MAX_PERIOD_MS + 1u ? age + 1u : age;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint32_t fault)
{
    e->faults |= fault;
}

void engine_tx_error(Engine *e)
{
    if (!add(&e->tx_errors, 1))
        engine_fault(e, F_OVERFLOW);
}

static bool break_chain(Lane *l)
{
    /* A broken anchor is one censored interval, not one missing tip. */
    if (l->anchored && !add(&l->breaks, 1))
        return false;
    l->anchored = false;
    l->parity = 0;
    l->previous_period = 0;
    l->since_tip = 0;
    l->pulse_age = 0;
    l->phase = WAIT_CLEAR;
    return true;
}

static bool reject(Lane *l)
{
    return add(&l->rejected, 1) && break_chain(l);
}

static bool complete(Lane *l)
{
    if (l->pulse_age < MIN_PULSE_MS || l->pulse_age > MAX_PULSE_MS)
        return reject(l);
    if (!add(&l->tips, 1))
        return false;
    if (l->anchored) {
        uint32_t period = l->since_tip;
        if (period < MIN_PERIOD_MS || period > MAX_PERIOD_MS) {
            /* The pulse is valid, but cannot support a cadence interval. */
            if (!break_chain(l))
                return false;
        } else {
            uint8_t bin = l->parity;
            if (!add(&l->intervals, 1) || !add(&l->bins[bin], 1) ||
                !add(&l->sum[bin], period))
                return false;
            if (!l->minimum || period < l->minimum)
                l->minimum = period;
            if (period > l->maximum)
                l->maximum = period;
            if (l->previous_period) {
                uint32_t diff = period > l->previous_period ?
                    period - l->previous_period : l->previous_period - period;
                if (diff > l->jitter)
                    l->jitter = diff;
            }
            l->previous_period = period;
            l->parity ^= 1u;
        }
    }
    l->anchored = true;
    l->since_tip = 0;
    l->pulse_age = 0;
    l->phase = READY;
    return true;
}

static void filter_step(Filter *f, uint8_t raw)
{
    if (raw != f->candidate) {
        f->candidate = raw;
        f->age = 1;
    } else if (f->age < QUALIFY_MS) {
        ++f->age;
    }
    if (f->age == QUALIFY_MS)
        f->stable = raw;
}

static bool lane_step(Lane *l, Filter *f, uint8_t tip, bool handled)
{
    if (l->anchored)
        l->since_tip = age_next(l->since_tip);
    if (l->phase == PULSE)
        ++l->pulse_age;
    /* Handling is conservative raw inhibition, never delayed by debounce. */
    if (handled) {
        if (!break_chain(l))
            return false;
        l->handled = true;
        *f = (Filter){0};
        return true;
    }
    if (l->handled) {
        l->handled = false;
        *f = (Filter){0};
    }
    filter_step(f, tip);
    if (l->anchored && l->since_tip > MAX_PERIOD_MS) {
        if (!break_chain(l))
            return false;
    }
    if (l->phase == WAIT_CLEAR) {
        /* Stable low must be newly observed after startup or invalidation. */
        if (!tip && f->age == QUALIFY_MS && !f->stable)
            l->phase = READY;
        return true;
    }
    if (l->phase == READY) {
        if (f->stable) {
            l->phase = PULSE;
            l->pulse_age = 0;
        }
        return true;
    }
    if (l->pulse_age > MAX_PULSE_MS)
        return reject(l);
    if (!f->stable)
        return complete(l);
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->initialized && now == e->now)
        return;
    Engine next = *e;
    next.now = now;
    next.raw = raw & 15u;
    if (raw & ~15u) {
        e->now = now;
        engine_fault(e, F_INPUT);
        return;
    }
    if (e->faults) {
        e->now = now;
        e->raw = raw;
        return;
    }
    bool gap = e->initialized && now - e->now != 1u;
    next.initialized = true;
    bool ok = true;
    if (gap) {
        ok = add(&next.gaps, 1);
        for (unsigned i = 0; i < 2; ++i) {
            ok = break_chain(&next.lane[i]) && ok;
            next.filter[i] = (Filter){0};
        }
    }
    for (unsigned i = 0; i < 2 && ok; ++i)
        ok = lane_step(&next.lane[i], &next.filter[i],
                       (raw >> i) & 1u, (raw & (4u << i)) != 0);
    if (ok) {
        *e = next;
    } else {
        /* Retain all prior evidence. Terminal fault stops acquisition. */
        e->now = now;
        e->raw = raw;
        engine_fault(e, F_OVERFLOW);
    }
}

bool engine_valid(const Engine *e)
{
    if (e->raw > 15u || (e->faults & ~15u))
        return false;
    for (unsigned i = 0; i < 2; ++i) {
        const Lane *l = &e->lane[i];
        if (l->phase > PULSE || l->parity > 1u)
            return false;
        if (l->bins[0] > l->intervals || l->bins[1] != l->intervals-l->bins[0])
            return false;
        if (l->intervals && l->intervals >= l->tips)
            return false;
        if (l->minimum > l->maximum || l->maximum > MAX_PERIOD_MS)
            return false;
        if (!l->intervals && (l->minimum || l->maximum || l->jitter))
            return false;
        if (l->intervals && l->minimum < MIN_PERIOD_MS)
            return false;
        if (l->jitter > l->maximum-l->minimum)
            return false;
        for (unsigned b = 0; b < 2; ++b) {
            uint64_t sum = l->sum[b], count = l->bins[b];
            if (sum < count * l->minimum || sum > count * l->maximum)
                return false;
        }
    }
    return true;
}
