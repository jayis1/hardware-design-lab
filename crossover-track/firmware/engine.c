/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

/* Counters never wrap back to a seemingly fresh session. */
static void increment(uint32_t *value)
{
    if (*value != UINT32_MAX) {
        ++*value;
    }
}

static void clear_lane(Engine *e, unsigned lane)
{
    e->start[lane] = UNKNOWN;
    e->finish[lane] = UNKNOWN;
    e->started[lane] = false;
    e->finished[lane] = false;
}

void engine_init(Engine *e, uint32_t now)
{
    *e = (Engine){0};
    e->now = now;
    e->phase = WAITING;
    for (unsigned lane = 0; lane < 2; ++lane) {
        clear_lane(e, lane);
        e->last.start[lane] = UNKNOWN;
        e->last.finish[lane] = UNKNOWN;
    }
}

/* Freeze one terminal result until the next terminal result. Streaming
 * status therefore survives late attachment, but is not an event queue. */
static void close_trial(Engine *e, Reason reason)
{
    if (e->phase != RUNNING) {
        return;
    }
    e->last.id = e->trial;
    e->last.ok = reason == NONE;
    e->last.reason = reason;
    for (unsigned lane = 0; lane < 2; ++lane) {
        e->last.start[lane] = e->start[lane];
        e->last.finish[lane] = e->finish[lane];
    }
    if (reason == NONE) {
        increment(&e->good);
    } else {
        increment(&e->bad);
    }
    e->phase = WAITING;
    e->baseline = 0;
}

static void lose_continuity(Engine *e, Reason reason)
{
    close_trial(e, reason);
    e->phase = WAITING;
    e->baseline = 0;
    e->qualified = 0;
    for (unsigned i = 0; i < 4; ++i) {
        e->stable[i] = 0;
    }
}

void engine_transport_loss(Engine *e)
{
    increment(&e->loss);
    lose_continuity(e, LINK);
}

/* Four independent filters preserve the ordering of unrelated channels.
 * Timestamp the qualification, not the first untrusted bounce. */
static void qualify(Engine *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        bool sample = (raw & bit) != 0;
        bool candidate = (e->candidates & bit) != 0;
        if (sample != candidate) {
            e->candidates ^= bit;
            e->stable[i] = 0;
        } else if (e->stable[i] < QUALIFY_MS) {
            ++e->stable[i];
        }
        if (e->stable[i] >= QUALIFY_MS) {
            if (sample) {
                e->levels |= bit;
            } else {
                e->levels &= (uint8_t)~bit;
            }
            e->qualified |= bit;
        }
    }
}

static bool baseline_present(const Engine *e)
{
    return e->qualified == 15u && e->levels == 5u;
}

static void wait_for_baseline(Engine *e)
{
    if (!baseline_present(e)) {
        e->baseline = 0;
        return;
    }
    if (e->trial == UINT32_MAX) {
        /* No ambiguous duplicate trial identifiers after saturation. */
        return;
    }
    if (e->baseline < BASELINE_MS) {
        ++e->baseline;
    }
    if (e->baseline == BASELINE_MS) {
        e->phase = ARMED;
        for (unsigned lane = 0; lane < 2; ++lane) {
            clear_lane(e, lane);
        }
    }
}

static void open_trial(Engine *e)
{
    increment(&e->trial);
    e->origin = e->now;
    e->phase = RUNNING;
    e->baseline = 0;
}

static bool lane_start(uint8_t mask, unsigned lane)
{
    return (mask & (1u << (2u * lane))) != 0;
}

static bool lane_finish(uint8_t mask, unsigned lane)
{
    return (mask & (2u << (2u * lane))) != 0;
}

static void process_lanes(Engine *e, uint8_t before)
{
    uint32_t elapsed = e->now - e->origin;
    for (unsigned lane = 0; lane < 2; ++lane) {
        bool was_start = lane_start(before, lane);
        bool at_start = lane_start(e->levels, lane);
        bool was_finish = lane_finish(before, lane);
        bool at_finish = lane_finish(e->levels, lane);
        bool departure = was_start && !at_start;
        bool arrival = !was_finish && at_finish;
        /* Arrival simultaneous with departure cannot prove travel order. */
        if (arrival && (!e->started[lane] || departure)) {
            close_trial(e, ORDER);
            return;
        }
        if (departure && !e->started[lane]) {
            e->started[lane] = true;
            e->start[lane] = elapsed;
        }
        if (e->started[lane] && at_start) {
            close_trial(e, RETURNED);
            return;
        }
        if (arrival && !e->finished[lane]) {
            e->finished[lane] = true;
            e->finish[lane] = elapsed;
        }
    }
    if (e->started[0] && e->started[1]) {
        uint32_t left = e->start[0];
        uint32_t right = e->start[1];
        uint32_t skew = left > right ? left - right : right - left;
        if (skew > MAX_SKEW_MS) {
            close_trial(e, SKEW);
            return;
        }
    } else if (elapsed > MAX_SKEW_MS) {
        close_trial(e, SKEW);
        return;
    }
    if (elapsed > MAX_TRIAL_MS) {
        close_trial(e, TIMEOUT);
        return;
    }
    if (e->finished[0] && e->finished[1]) {
        close_trial(e, NONE);
    }
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t step = now - e->now;
    if (!step) {
        return;
    }
    e->now = now;
    if (step != 1u) {
        increment(&e->gaps);
        lose_continuity(e, GAP);
        e->candidates = raw & 15u;
        return;
    }
    uint8_t before = e->levels;
    qualify(e, raw & 15u);
    if (e->qualified != 15u) {
        return;
    }
    if (e->phase == WAITING) {
        wait_for_baseline(e);
        return;
    }
    if (e->phase == ARMED) {
        if (e->levels == 5u) {
            return;
        }
        /* A finish hit before any departure invalidates this attempt too. */
        open_trial(e);
    }
    process_lanes(e, before);
}
