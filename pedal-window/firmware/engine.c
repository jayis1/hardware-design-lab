/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* Counters saturate rather than making old evidence appear to disappear. */
static void increment(uint32_t *value)
{
    if (*value != UINT32_MAX) {
        ++*value;
    }
}

void engine_init(Engine *e, uint32_t now)
{
    memset(e, 0, sizeof *e);
    e->now = now;
    e->previous = now;
    for (unsigned i = 0; i < 4; ++i) {
        e->candidate_since[i] = now;
    }
    e->phase = WAIT_BASELINE;
}

void engine_transport_loss(Engine *e)
{
    increment(&e->transport_losses);
}

uint32_t engine_flags(const Engine *e)
{
    uint32_t flags = e->flags;
    if (!e->qualified) {
        flags |= FLAG_UNQUALIFIED;
    }
    return flags;
}

static bool contradictory(uint8_t levels)
{
    return (levels & (P_REST | P_DOWN)) == (P_REST | P_DOWN);
}

static void clear_capture(Engine *e)
{
    e->seen_a_off = false;
    e->seen_b_on = false;
    e->seen_rest = false;
    e->seen_down = false;
    e->a_off = 0;
    e->b_on = 0;
    e->pedal_rest = 0;
    e->pedal_down = 0;
}

static void begin_capture(Engine *e)
{
    clear_capture(e);
    e->flags = 0;
    e->started = e->now;
    e->phase = CAPTURE;
    increment(&e->attempts);
}

/* All offsets are relative to this bounded trial's origin, avoiding signed
 * conversion of a wrapped absolute clock. Five seconds fits int32_t. */
static int32_t difference(uint32_t left, uint32_t right)
{
    return (int32_t)left - (int32_t)right;
}

static void finish_capture(Engine *e, bool complete)
{
    e->last.id = e->attempts;
    e->last.outcome = complete ? RESULT_COMPLETE : RESULT_REJECTED;
    e->last.flags = e->flags;
    e->last.key_gap = 0;
    e->last.release_delta = 0;
    e->last.down_delta = 0;
    if (complete) {
        e->last.key_gap = difference(e->b_on, e->a_off);
        e->last.release_delta = difference(e->pedal_rest, e->b_on);
        e->last.down_delta = difference(e->pedal_down, e->b_on);
        increment(&e->completed);
    } else {
        increment(&e->rejected);
    }
    e->phase = WAIT_BASELINE;
}

static void fail_capture(Engine *e, uint32_t reason)
{
    e->flags |= reason;
    if (e->phase == CAPTURE) {
        finish_capture(e, false);
    } else {
        e->phase = WAIT_BASELINE;
    }
}

static void inspect_capture(Engine *e, uint8_t before, uint8_t after)
{
    uint8_t rising = (uint8_t)(after & (uint8_t)~before);
    uint8_t falling = (uint8_t)(before & (uint8_t)~after);
    uint32_t elapsed = e->now - e->started;
    unsigned events = 0;
    if (e->seen_a_off && (rising & KEY_A)) {
        fail_capture(e, FLAG_REATTACK);
        return;
    }
    if (e->seen_b_on && (falling & KEY_B)) {
        fail_capture(e, FLAG_KEY_RELEASE);
        return;
    }
    if (!e->seen_a_off && (falling & KEY_A)) {
        e->seen_a_off = true;
        e->a_off = elapsed;
        ++events;
    }
    if (!e->seen_b_on && (rising & KEY_B)) {
        e->seen_b_on = true;
        e->b_on = elapsed;
        ++events;
    }
    if (!e->seen_rest && (rising & P_REST)) {
        e->seen_rest = true;
        e->pedal_rest = elapsed;
        ++events;
    }
    /* A full pedal cycle is required, not just a momentarily open down
     * contact. Both endpoints are independently observed. */
    if (e->seen_rest && (rising & P_DOWN)) {
        if (!e->seen_b_on || !e->seen_a_off) {
            fail_capture(e, FLAG_REATTACK);
            return;
        }
        e->seen_down = true;
        e->pedal_down = elapsed;
        ++events;
    }
    if (events > 1u) {
        e->flags |= FLAG_SIMULTANEOUS;
    }
    if (e->seen_down && e->seen_b_on && e->seen_a_off) {
        finish_capture(e, true);
    }
}

static void qualified_vector(Engine *e, uint8_t before, uint8_t after)
{
    if (contradictory(after)) {
        fail_capture(e, FLAG_CONTRADICTION);
        return;
    }
    switch (e->phase) {
    case WAIT_BASELINE:
        if (after == P_REST) {
            e->phase = READY;
            e->flags = 0;
        }
        break;
    case READY:
        /* Reject B-first practice, but permit pedal/key A in either order. */
        if (after & KEY_B) {
            e->phase = WAIT_BASELINE;
            break;
        }
        if (after == (KEY_A | P_DOWN)) {
            begin_capture(e);
        }
        break;
    case CAPTURE:
        inspect_capture(e, before, after);
        break;
    default:
        e->phase = WAIT_BASELINE;
        break;
    }
}

void engine_sample(Engine *e, uint32_t now, uint8_t closed)
{
    uint32_t delta = now - e->previous;
    e->now = now;
    if (!delta) {
        return;
    }
    e->previous = now;
    closed &= 15u;
    if (delta > MAX_SAMPLE_MS) {
        increment(&e->gaps);
        fail_capture(e, FLAG_GAP);
        /* Never qualify the first vector after an acquisition gap. */
        e->qualified = false;
        e->candidate = closed;
        e->qualified_mask = 0;
        for (unsigned i = 0; i < 4; ++i) {
            e->candidate_since[i] = now;
        }
        return;
    }
    if (e->phase == CAPTURE && now - e->started >= TRIAL_MS) {
        fail_capture(e, FLAG_TIMEOUT);
    }
    uint8_t before = e->levels;
    bool was_qualified = e->qualified;
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((closed ^ e->candidate) & bit) {
            e->candidate ^= bit;
            e->candidate_since[i] = now;
        } else if (now - e->candidate_since[i] >= DEBOUNCE_MS) {
            e->qualified_mask |= bit;
            e->levels = (uint8_t)((e->levels & (uint8_t)~bit) | (closed & bit));
        }
    }
    e->qualified = e->qualified_mask == 15u;
    if (e->qualified && (!was_qualified || before != e->levels)) {
        qualified_vector(e, before, e->levels);
    }
    /* A failure can occur in the vector that reaches baseline, or a timeout
     * can occur while baseline is held. Recovery requires no extra edge. */
    if (e->qualified && e->phase == WAIT_BASELINE && e->levels == P_REST) {
        e->phase = READY;
        e->flags = 0;
    }
}
