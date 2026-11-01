/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>

void engine_init(Engine *e)
{
    unsigned char *p = (unsigned char *)e;
    for (unsigned i = 0; i < sizeof *e; ++i)
        p[i] = 0;
    e->state = WAIT;
}

static bool one_face(uint32_t value)
{
    return value == 1u || value == 2u;
}

static void retain(Engine *e, uint32_t result)
{
    e->last_faces = e->faces;
    e->last_strokes = e->strokes;
    e->last_age = e->age;
    e->last_result = result;
}

static void abandon(Engine *e)
{
    if (e->state == ACTIVE) {
        /* A counted start reserves one outcome slot. */
        ++e->aborted;
        retain(e, 3);
    }
    e->state = WAIT;
}

void engine_fault(Engine *e, uint32_t fault)
{
    abandon(e);
    e->faults |= fault;
    e->state = HALT;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors == UINT32_MAX)
        engine_fault(e, F_OVERFLOW);
    else
        ++e->tx_errors;
}

static void lose_continuity(Engine *e)
{
    abandon(e);
    e->valid = 0;
    e->levels = 0;
    for (unsigned i = 0; i < 4; ++i)
        e->stable[i] = 0;
    if (e->gaps == UINT32_MAX)
        engine_fault(e, F_OVERFLOW);
    else
        ++e->gaps;
}

/* Independent qualifiers prevent a chattering detent hiding latch release. */
static void qualify(Engine *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t value = (uint8_t)((raw >> i) & 1u);
        uint32_t bit = 1u << i;
        if (!e->stable[i] || value != e->candidate[i]) {
            e->candidate[i] = value;
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY) {
            e->valid |= bit;
            if (value)
                e->levels |= bit;
            else
                e->levels &= ~bit;
        }
    }
}

static void begin(Engine *e)
{
    if (e->starts == UINT32_MAX) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    ++e->starts;
    e->faces = 0;
    e->strokes = 0;
    e->stroke_face = 0;
    e->age = 0;
    e->tainted = 0;
    e->state = ACTIVE;
}

static void finish(Engine *e)
{
    if (!e->tainted && e->faces == 3u && e->strokes == 2u) {
        ++e->good;
        retain(e, 1);
    } else {
        ++e->bad;
        retain(e, 2);
    }
    e->state = WAIT;
}

static void observe_press(Engine *e, uint32_t previous, uint8_t raw)
{
    uint32_t face = e->levels & FACE_MASK;
    uint32_t old_face = previous & FACE_MASK;
    /* A raw contradiction during the entire press envelope is sticky.
     * Include the release endpoint and even glitches rejected by debounce.
     * Before the qualified rise, setup is relative to prior detent evidence. */
    if ((raw | e->levels | previous) & PRESS) {
        uint32_t required = (previous & PRESS) ? e->stroke_face : old_face;
        if (!one_face(required) || face != required ||
            (raw & FACE_MASK) != required || !(raw & LATCH))
            e->tainted = 1;
    }
    if ((e->levels & PRESS) && !(previous & PRESS)) {
        if (e->strokes == UINT32_MAX) {
            engine_fault(e, F_OVERFLOW);
            return;
        }
        ++e->strokes;
        e->stroke_face = face;
        if (!one_face(face) || face != old_face ||
            (raw & FACE_MASK) != face || !(raw & LATCH)) {
            e->tainted = 1;
            return;
        }
        if (e->faces & face)
            e->tainted = 1;
        e->faces |= face;
    }
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->initialized && now == e->stamp)
        return;
    uint32_t elapsed = now - e->stamp;
    bool gap = e->initialized && elapsed != 1u;
    e->initialized = true;
    e->stamp = now;
    e->now = now;
    if (e->state == HALT)
        return;
    if (raw > 15u) {
        engine_fault(e, F_CONFIG);
        return;
    }
    if (gap) {
        lose_continuity(e);
        return;
    }
    uint32_t previous = e->levels;
    qualify(e, raw);
    if (e->valid != 15u)
        return;
    if (e->state == ACTIVE) {
        ++e->age;
        /* Timeout precedes all endpoint interpretation. */
        if (e->age >= MAX_TRIAL_MS) {
            abandon(e);
            return;
        }
        observe_press(e, previous, raw);
        if (e->state == HALT)
            return;
        if (!(e->levels & LATCH)) {
            if ((raw | previous | e->levels) & PRESS)
                e->tainted = 1;
            finish(e);
        }
    } else if (e->state == READY) {
        /* Only a newly closed latch with no press can start a trial.
         * Orientation is allowed to settle later, before the first stroke. */
        if ((e->levels & LATCH) && !(previous & LATCH) &&
            !((raw | previous | e->levels) & PRESS) && (raw & LATCH))
            begin(e);
        else if (((raw | e->levels) & PRESS) || (e->levels & LATCH))
            e->state = WAIT;
    }
    /* Face switches may be active on an empty holder. */
    if (e->state == WAIT && !((e->levels | raw) & (PRESS | LATCH)))
        e->state = READY;
}

static bool coverage_valid(uint32_t faces, uint32_t strokes)
{
    if (faces > 3u)
        return false;
    unsigned population = (faces & 1u) + ((faces >> 1) & 1u);
    return strokes >= population;
}

bool engine_valid(const Engine *e)
{
    if (e->state > HALT || e->levels > 15u || e->valid > 15u)
        return false;
    if (e->faults > 7u || e->tainted > 1u || e->last_result > 3u)
        return false;
    if ((e->state == HALT) != (e->faults != 0u))
        return false;
    uint64_t outcomes = (uint64_t)e->good + e->bad + e->aborted;
    if (outcomes + (e->state == ACTIVE) != e->starts)
        return false;
    if (e->age > MAX_TRIAL_MS || e->last_age > MAX_TRIAL_MS)
        return false;
    if (!coverage_valid(e->faces, e->strokes) ||
        !coverage_valid(e->last_faces, e->last_strokes))
        return false;
    if (e->state == ACTIVE && (e->valid != 15u || !(e->levels & LATCH)))
        return false;
    if (e->state == READY && (e->valid != 15u || (e->levels & (LATCH | PRESS))))
        return false;
    if ((outcomes == 0) != (e->last_result == 0))
        return false;
    if (e->last_result == 1 && (!e->good || e->last_faces != 3u || e->last_strokes != 2u))
        return false;
    if (e->last_result == 2 && !e->bad)
        return false;
    if (e->last_result == 3 && !e->aborted)
        return false;
    return true;
}
