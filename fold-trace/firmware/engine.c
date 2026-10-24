/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>
#include <string.h>

static void increment(uint32_t *value)
{
    if (*value != UINT32_MAX)
        ++*value;
}

void engine_init(Engine *e, uint32_t now)
{
    memset(e, 0, sizeof *e);
    e->state = CLEAR;
    e->previous = now;
    e->clear_start = now;
    for (unsigned i = 0; i < STATIONS; ++i)
        e->contact[i].since = now;
}

static void reject(Engine *e, Reason reason, uint32_t now)
{
    /* One failed part, not one failure per sample. */
    if (e->state == REJECT)
        return;
    increment(&e->bad);
    e->state = REJECT;
    e->reason = reason;
    e->duration = now - e->cycle_start;
    e->clearing = false;
}

static void qualify(Engine *e, uint32_t now, uint8_t raw)
{
    uint8_t mask = 0;
    for (unsigned i = 0; i < STATIONS; ++i) {
        Contact *c = &e->contact[i];
        bool value = (raw & (1u << i)) != 0;
        if (value != c->candidate) {
            c->candidate = value;
            c->since = now;
        }
        if (now - c->since >= DEBOUNCE_MS)
            c->stable = c->candidate;
        if (c->stable)
            mask |= (uint8_t)(1u << i);
    }
    e->mask = mask;
}

static void clear_cycle(Engine *e, uint32_t now)
{
    /* Raw as well as debounced contacts must remain open. */
    if (e->raw || e->mask) {
        e->clearing = false;
        return;
    }
    if (!e->clearing) {
        e->clearing = true;
        e->clear_start = now;
        return;
    }
    if (now - e->clear_start < CLEAR_MS)
        return;
    e->state = READY;
    e->reason = NONE;
    e->step = 0;
    e->clearing = false;
}

static bool multiple(uint8_t value)
{
    return value && (value & (value - 1u));
}

static void begin(Engine *e, uint32_t now)
{
    e->cycle_start = now;
    e->hold_start = now;
    e->duration = 0;
    e->reason = NONE;
    e->step = 0;
    for (unsigned i = 0; i < STATIONS; ++i)
        e->dwell[i] = 0;
    if (multiple(e->mask)) {
        reject(e, OVERLAP, now);
        return;
    }
    if (e->mask != 1u) {
        reject(e, ORDER, now);
        return;
    }
    e->state = HOLD;
}

static void held(Engine *e, uint32_t now)
{
    uint8_t expected = (uint8_t)(1u << e->step);
    if (multiple(e->mask)) {
        reject(e, OVERLAP, now);
        return;
    }
    if (e->mask && e->mask != expected) {
        reject(e, ORDER, now);
        return;
    }
    if (!e->mask) {
        e->dwell[e->step] = now - e->hold_start;
        if (e->dwell[e->step] < MIN_HOLD_MS) {
            reject(e, SHORT_HOLD, now);
            return;
        }
        ++e->step;
        if (e->step == STATIONS) {
            increment(&e->good);
            e->duration = now - e->cycle_start;
            e->state = CLEAR;
            e->clearing = false;
        } else {
            e->state = RELEASE;
        }
    }
}

static void next(Engine *e, uint32_t now)
{
    if (!e->mask)
        return;
    if (multiple(e->mask)) {
        reject(e, OVERLAP, now);
        return;
    }
    if (e->mask != (1u << e->step)) {
        reject(e, ORDER, now);
        return;
    }
    e->hold_start = now;
    e->state = HOLD;
}

void engine_sample(Engine *e, uint32_t now, uint8_t closed_mask)
{
    uint32_t elapsed = now - e->previous;
    e->previous = now;
    e->raw = closed_mask & 15u;
    if (elapsed > MAX_SAMPLE_MS) {
        increment(&e->gaps);
        /* A scheduling gap is unknown time, never stable qualification. */
        for (unsigned i = 0; i < STATIONS; ++i) {
            e->contact[i].since = now;
            e->contact[i].candidate = (e->raw & (1u << i)) != 0;
        }
        e->clearing = false;
        if (e->state == HOLD || e->state == RELEASE) {
            reject(e, SAMPLE_GAP, now);
        } else if (e->state == READY) {
            e->state = CLEAR;
        }
    }
    qualify(e, now, e->raw);
    if ((e->state == HOLD || e->state == RELEASE) &&
        now - e->cycle_start >= CYCLE_MS) {
        reject(e, TIMEOUT, now);
    }
    switch (e->state) {
    case CLEAR:
    case REJECT:
        clear_cycle(e, now);
        break;
    case READY:
        if (e->mask)
            begin(e, now);
        break;
    case HOLD:
        held(e, now);
        break;
    case RELEASE:
        next(e, now);
        break;
    }
}

const char *engine_state(State state)
{
    switch (state) {
    case CLEAR: return "CLEAR";
    case READY: return "READY";
    case HOLD: return "HOLD";
    case RELEASE: return "RELEASE";
    case REJECT: return "REJECT";
    }
    return "INVALID";
}

const char *engine_reason(Reason reason)
{
    switch (reason) {
    case NONE: return "NONE";
    case ORDER: return "ORDER";
    case OVERLAP: return "OVERLAP";
    case SHORT_HOLD: return "SHORT_HOLD";
    case TIMEOUT: return "TIMEOUT";
    case SAMPLE_GAP: return "SAMPLE_GAP";
    }
    return "INVALID";
}
