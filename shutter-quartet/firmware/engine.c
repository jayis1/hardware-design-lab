/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

static bool increment(uint32_t *value)
{
    if (*value == UINT32_MAX)
        return false;
    ++*value;
    return true;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

static void abandon(Engine *e)
{
    /* starts conservation reserves room for this abandonment. */
    if (e->state == ACTIVE)
        ++e->aborted;
    e->state = WAIT;
    e->age = 0;
    e->dark = 0;
    e->prior_dark = false;
}

void engine_fault(Engine *e, uint8_t flag)
{
    abandon(e);
    e->flags |= flag;
    e->state = HALTED;
}

void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx))
        engine_fault(e, F_OVERFLOW);
}

/* Qualify each optical input separately. A short threshold excursion on
 * one detector cannot postpone a stable edge on another detector. Time
 * is counted only across consecutive one-millisecond observations. */
static void qualify(Engine *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((e->candidate ^ raw) & bit) {
            e->candidate ^= bit;
            e->stable[i] = 0;
        } else if (e->stable[i] < QUALIFY_MS) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY_MS) {
            e->known |= bit;
            e->levels = (uint8_t)((e->levels & ~bit) | (raw & bit));
        }
    }
}

static void baseline(Engine *e)
{
    bool dark = e->known == 15 && e->raw == 0 && e->levels == 0;
    if (dark && e->prior_dark) {
        if (e->dark < DARK_MS)
            ++e->dark;
    } else {
        e->dark = 0;
    }
    e->prior_dark = dark;
}

static bool begin(Engine *e)
{
    if (!increment(&e->starts))
        return false;
    e->state = ACTIVE;
    e->age = 0;
    e->working_seen = 0;
    for (unsigned i = 0; i < 4; ++i) {
        e->opening[i] = 0;
        e->closing[i] = 0;
        e->exposure[i] = 0;
        e->segments[i] = 0;
    }
    return true;
}

static bool edges(Engine *e, uint8_t before)
{
    uint8_t rises = (uint8_t)(e->levels & ~before);
    uint8_t falls = (uint8_t)(before & ~e->levels);
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if (rises & bit) {
            if (!(e->working_seen & bit))
                e->opening[i] = e->age;
            e->working_seen |= bit;
            if (!increment(&e->segments[i]))
                return false;
        }
        if (falls & bit)
            e->closing[i] = e->age;
    }
    return true;
}

static bool finish(Engine *e)
{
    if (!increment(&e->completed))
        return false;
    e->seen = e->working_seen;
    e->span = 0;
    for (unsigned i = 0; i < 4; ++i) {
        e->first[i] = e->opening[i];
        e->last[i] = e->closing[i];
        e->lit[i] = e->exposure[i];
        e->pulses[i] = e->segments[i];
        if (e->last[i] > e->span)
            e->span = e->last[i];
    }
    e->state = READY;
    e->age = 0;
    return true;
}

static bool update(Engine *e, uint8_t raw)
{
    uint8_t before = e->levels;
    if (e->state == ACTIVE) {
        ++e->age;
        /* Account the observed interval before changing qualified levels.
         * Timeout discards all working evidence, including this interval. */
        for (unsigned i = 0; i < 4; ++i) {
            if ((before & (1u << i)) && !increment(&e->exposure[i]))
                return false;
        }
    }
    qualify(e, raw);
    baseline(e);
    if (e->state == ACTIVE && e->age >= LIMIT_MS) {
        if (!increment(&e->timeouts))
            return false;
        abandon(e);
        return true;
    }
    if (e->state == WAIT) {
        if (e->dark == DARK_MS)
            e->state = READY;
        return true;
    }
    if (e->state == READY) {
        if (!e->levels)
            return true;
        if (!begin(e))
            return false;
        /* Before READY the baseline guarantees every level was dark. */
        before = 0;
    }
    if (e->state == ACTIVE) {
        if (!edges(e, before))
            return false;
        if (e->dark == DARK_MS)
            return finish(e);
    }
    return true;
}

void engine_sample(Engine *e, uint32_t time, uint8_t raw)
{
    if (raw > 15) {
        engine_fault(e, F_CONFIG);
        return;
    }
    if (e->state == HALTED) {
        e->time = time;
        e->raw = raw;
        return;
    }
    Engine next = *e;
    uint32_t elapsed = time - e->time;
    next.time = time;
    next.raw = raw;
    if (!e->sampled || elapsed != 1) {
        if (e->sampled) {
            if (!increment(&next.gaps)) {
                e->time = time;
                e->raw = raw;
                engine_fault(e, F_OVERFLOW);
                return;
            }
            next.flags |= F_GAP;
            abandon(&next);
        }
        next.sampled = true;
        next.known = 0;
        next.candidate = raw;
        for (unsigned i = 0; i < 4; ++i)
            next.stable[i] = 0;
        *e = next;
        return;
    }
    if (!update(&next, raw)) {
        /* Roll back a multi-field sample, then abandon exactly once. */
        e->time = time;
        e->raw = raw;
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->state > HALTED || e->raw > 15 || e->levels > 15 ||
        e->known > 15 || e->seen > 15 || (e->flags & ~29u))
        return false;
    uint64_t outcomes = (uint64_t)e->completed + e->aborted;
    if (outcomes + (e->state == ACTIVE) != e->starts)
        return false;
    if (e->timeouts > e->aborted || e->age >= LIMIT_MS ||
        e->span >= LIMIT_MS || e->dark > DARK_MS)
        return false;
    if (e->state != ACTIVE && e->age)
        return false;
    if ((e->state == HALTED) != ((e->flags & ~F_GAP) != 0))
        return false;
    if ((e->gaps != 0) != ((e->flags & F_GAP) != 0))
        return false;
    if (e->state == READY && (e->levels || e->known != 15))
        return false;
    if (e->state == ACTIVE && e->known != 15)
        return false;
    if ((e->completed != 0) != (e->seen != 0))
        return false;
    uint32_t end = 0;
    bool zero_start = false;
    for (unsigned i = 0; i < 4; ++i) {
        if (!(e->seen & (1u << i))) {
            if (e->first[i] || e->last[i] || e->lit[i] || e->pulses[i])
                return false;
            continue;
        }
        if (!e->pulses[i] || e->first[i] >= e->last[i] ||
            e->last[i] > e->span || !e->lit[i])
            return false;
        uint32_t envelope = e->last[i] - e->first[i];
        if (e->lit[i] > envelope || e->pulses[i] > e->lit[i])
            return false;
        if (e->pulses[i] == 1 && e->lit[i] != envelope)
            return false;
        /* Each extra pulse needs at least one observed dark interval. */
        if (e->pulses[i] - 1 > envelope - e->lit[i])
            return false;
        if (!e->first[i])
            zero_start = true;
        if (e->last[i] > end)
            end = e->last[i];
    }
    if (end != e->span || (e->seen && !zero_start))
        return false;
    return true;
}
