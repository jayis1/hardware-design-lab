/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>
#include <stddef.h>

static uint32_t saturating_add(uint32_t a, uint32_t b)
{
    if (UINT32_MAX - a < b)
        return UINT32_MAX;
    return a + b;
}

/* Exhaustion is visible and terminal; wrapping counts would invent evidence. */
static void increment(Engine *e, uint32_t *value)
{
    if (*value == UINT32_MAX) {
        if ((e->state == FLIGHT || e->state == SETTLE) && e->rejected != UINT32_MAX)
            ++e->rejected;
        e->state = FAULT;
        e->reason = LIMIT;
        e->fault_mask |= 8u;
        e->last_route = 0;
        e->last_ms = 0;
    } else {
        ++*value;
    }
}

unsigned route_bin(uint32_t duration)
{
    static const uint32_t upper[] = {250, 500, 1000, 2000, 5000};
    for (unsigned i = 0; i < 5; ++i) {
        if (duration < upper[i])
            return i;
    }
    return 5;
}

uint32_t route_mean(const Route *r)
{
    if (!r || !r->count)
        return 0;
    return (uint32_t)(r->sum / r->count);
}

static void reset_gate(Gate *g, bool raw)
{
    g->candidate = raw;
    g->stable = raw;
    g->age = 0;
    g->blocked_ms = 0;
}

void engine_init(Engine *e, uint32_t now, uint8_t raw)
{
    *e = (Engine){0};
    e->time = now;
    e->state = DRAIN;
    e->levels = raw & 15u;
    for (unsigned i = 0; i < 4; ++i)
        reset_gate(&e->gate[i], (raw & (1u << i)) != 0);
}

static void reject(Engine *e, Reason reason)
{
    if (e->state == FAULT)
        return;
    if (e->state == FLIGHT || e->state == SETTLE)
        increment(e, &e->rejected);
    if (e->state != FAULT)
        e->state = DRAIN;
    e->reason = reason;
    e->clear_ms = 0;
    e->last_route = 0;
    e->last_ms = 0;
}

void engine_fault(Engine *e)
{
    reject(e, LIMIT);
    e->state = FAULT;
    e->fault_mask |= 16u;
}

static uint8_t qualify(Engine *e, uint8_t raw)
{
    uint8_t result = 0;
    for (unsigned i = 0; i < 4; ++i) {
        Gate *g = &e->gate[i];
        bool input = (raw & (1u << i)) != 0;
        if (g->candidate != input) {
            g->candidate = input;
            g->age = 1;
        } else if (g->age < QUALIFY_MS) {
            ++g->age;
        }
        if (g->age >= QUALIFY_MS)
            g->stable = g->candidate;
        if (g->stable)
            result |= (uint8_t)(1u << i);
    }
    return result;
}

static bool blocked(Engine *e)
{
    bool any = false;
    for (unsigned i = 0; i < 4; ++i) {
        Gate *g = &e->gate[i];
        if (!g->stable) {
            g->blocked_ms = 0;
            continue;
        }
        uint32_t before = g->blocked_ms;
        g->blocked_ms = saturating_add(g->blocked_ms, 1);
        if (before < BLOCK_MS && g->blocked_ms >= BLOCK_MS) {
            increment(e, &e->block_events);
            e->fault_mask |= 2u;
        }
        if (g->blocked_ms >= BLOCK_MS)
            any = true;
    }
    return any;
}

static unsigned population(uint8_t mask)
{
    unsigned count = 0;
    while (mask) {
        count += mask & 1u;
        mask >>= 1;
    }
    return count;
}

static void accept(Engine *e, unsigned route, uint32_t duration)
{
    Route *r = &e->route[route];
    if (r->count == UINT32_MAX || e->accepted == UINT32_MAX ||
        r->sum > UINT64_MAX - duration) {
        engine_fault(e);
        return;
    }
    if (!r->count || duration < r->minimum)
        r->minimum = duration;
    if (duration > r->maximum)
        r->maximum = duration;
    ++r->count;
    r->sum += duration;
    ++r->bins[route_bin(duration)];
    ++e->accepted;
    e->last_ms = duration;
    e->last_route = route + 1;
    e->reason = NONE;
    e->state = DRAIN;
    e->clear_ms = 0;
}

static void acquisition_gap(Engine *e, uint8_t raw)
{
    increment(e, &e->gaps);
    if (e->state == FAULT)
        return;
    e->fault_mask |= 1u;
    reject(e, GAP);
    /* Stable levels survive. A post-gap glitch cannot establish clear. */
    for (unsigned i = 0; i < 4; ++i) {
        e->gate[i].candidate = (raw & (1u << i)) != 0;
        e->gate[i].age = 0;
        e->gate[i].blocked_ms = 0;
    }
}

static void begin_trial(Engine *e, uint8_t rising)
{
    increment(e, &e->entries);
    if (e->state == FAULT)
        return;
    e->state = FLIGHT;
    e->start = e->time;
    e->last_route = 0;
    e->last_ms = 0;
    e->reason = NONE;
    if (rising & 14u)
        reject(e, SIMULTANEOUS);
}

static void observe_flight(Engine *e, uint8_t rising)
{
    uint32_t elapsed = e->time - e->start;
    /* Deadline is exclusive: an exit at 30000ms is not accepted. */
    if (elapsed >= FLIGHT_MS) {
        reject(e, EXPIRED);
        return;
    }
    if (rising & 1u) {
        /* An entry while occupied invalidates this trial, not a second one. */
        reject(e, OVERLAP);
        return;
    }
    uint8_t outlets = rising & 14u;
    if (!outlets)
        return;
    if (population(outlets) != 1) {
        reject(e, MULTIPLE);
        return;
    }
    for (unsigned i = 0; i < 3; ++i) {
        if (outlets & (2u << i)) {
            e->pending_route = i;
            e->pending_ms = elapsed;
            e->clear_ms = 0;
            e->state = SETTLE;
            return;
        }
    }
}

static void observe_settle(Engine *e, uint8_t rising)
{
    if (rising & 1u) {
        reject(e, OVERLAP);
        return;
    }
    if (rising & 14u) {
        reject(e, MULTIPLE);
        return;
    }
    if (e->time - e->start >= FLIGHT_MS) {
        reject(e, EXPIRED);
        return;
    }
    if (e->levels) {
        e->clear_ms = 0;
        return;
    }
    ++e->clear_ms;
    if (e->clear_ms >= CLEAR_MS) {
        accept(e, e->pending_route, e->pending_ms);
        if (e->state != FAULT)
            e->state = READY;
    }
}

static void observe_drain(Engine *e, uint8_t rising)
{
    /* No new trial while draining. Stray outlet activity remains visible. */
    if (rising & 14u)
        increment(e, &e->orphans);
    if (e->levels) {
        e->clear_ms = 0;
        return;
    }
    e->clear_ms = saturating_add(e->clear_ms, 1);
    if (e->clear_ms >= CLEAR_MS && e->state != FAULT)
        e->state = READY;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - e->time;
    if (!delta)
        return;
    e->time = now;
    raw &= 15u;
    if (e->state == FAULT)
        return;
    if (delta != 1) {
        acquisition_gap(e, raw);
        return;
    }
    uint8_t previous = e->levels;
    e->levels = qualify(e, raw);
    uint8_t rising = (uint8_t)(e->levels & ~previous);
    if (blocked(e)) {
        reject(e, BLOCKED);
        return;
    }
    switch (e->state) {
    case DRAIN:
        observe_drain(e, rising);
        break;
    case READY:
        if (rising & 1u) {
            begin_trial(e, rising);
        } else if (rising & 14u) {
            increment(e, &e->orphans);
            if (e->state != FAULT)
                e->state = DRAIN;
            e->clear_ms = 0;
        }
        break;
    case FLIGHT:
        observe_flight(e, rising);
        break;
    case SETTLE:
        observe_settle(e, rising);
        break;
    case FAULT:
        break;
    }
}

uint32_t engine_flags(const Engine *e)
{
    return e->fault_mask;
}

bool engine_consistent(const Engine *e)
{
    uint64_t total = 0;
    for (unsigned i = 0; i < 3; ++i) {
        const Route *r = &e->route[i];
        uint64_t bins = 0;
        for (unsigned j = 0; j < HISTOGRAM_BINS; ++j)
            bins += r->bins[j];
        if (bins != r->count)
            return false;
        if (r->count && (r->minimum > r->maximum || r->maximum >= FLIGHT_MS))
            return false;
        if (r->sum < (uint64_t)r->minimum * r->count)
            return false;
        if (r->sum > (uint64_t)r->maximum * r->count)
            return false;
        total += r->count;
    }
    if (total != e->accepted)
        return false;
    if ((uint64_t)e->accepted + e->rejected + (e->state == FLIGHT || e->state == SETTLE) != e->entries)
        return false;
    return e->state <= FAULT && e->levels < 16;
}
