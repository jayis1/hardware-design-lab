/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

static bool active(const Engine *e)
{
    return e->phase == TRAVEL || e->phase == RETURN;
}

static bool increment(uint32_t *value)
{
    if (*value >= LIMIT)
        return false;
    ++*value;
    return true;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint32_t fault)
{
    /* Abandon once, preserving starts = completed + rejected + active. */
    if (active(e))
        ++e->rejected;
    e->fault |= fault;
    e->phase = HALT;
    e->quiet = 0;
}

void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx_errors))
        engine_fault(e, F_OVERFLOW);
}

static void qualify(Engine *e, uint8_t raw)
{
    /* Each channel is independent: latch chatter cannot hide a checkpoint. */
    for (unsigned channel = 0; channel < 4; ++channel) {
        uint8_t bit = (uint8_t)(1u << channel);
        uint8_t level = (raw & bit) != 0;
        if (!e->run[channel] || e->candidate[channel] != level) {
            e->candidate[channel] = level;
            e->run[channel] = 1;
        } else if (e->run[channel] < QUALIFY) {
            ++e->run[channel];
        }
        if (e->run[channel] == QUALIFY) {
            e->valid |= bit;
            e->qualified = (uint8_t)((e->qualified & ~bit) | (level ? bit : 0));
        }
    }
}

static bool reject(Engine *e)
{
    if (!increment(&e->rejected))
        return false;
    e->phase = BASELINE;
    e->quiet = 0;
    e->age = 0;
    e->seen = 0;
    e->disturbed = 0;
    return true;
}

static bool process(Engine *e, uint8_t previous_raw, uint8_t previous_q)
{
    const uint8_t position = e->raw & 7u;
    const uint8_t qposition = e->qualified & 7u;
    const bool home = position == 1 && qposition == 1;
    const bool old_home = (previous_raw & 7u) == 1 && (previous_q & 7u) == 1;
    const bool overlap = position && (position & (position - 1u));
    if (active(e)) {
        ++e->age;
        /* Rejection wins even when the return window would finish now. */
        if (e->age >= DEADLINE || overlap)
            return reject(e);
    }
    switch (e->phase) {
    case BASELINE:
        if (e->valid == 15 && home && old_home) {
            if (e->quiet < WINDOW)
                ++e->quiet;
            if (e->quiet == WINDOW)
                e->phase = READY;
        } else {
            e->quiet = 0;
        }
        break;
    case READY:
        if (overlap) {
            e->phase = BASELINE;
            e->quiet = 0;
        } else if (!(qposition & 1u) && !(position & 1u)) {
            if (!increment(&e->starts))
                return false;
            e->phase = TRAVEL;
            e->age = 0;
            e->quiet = 0;
            /* A checkpoint can qualify on the departure sample itself. */
            e->seen = (uint8_t)((position & qposition) >> 1);
            e->disturbed = 0;
        }
        break;
    case TRAVEL:
        /* Only actual independently qualified checkpoint presence is credited.
         * We deliberately do not infer intermediate positions from FAR. */
        if (position == 2 && (qposition & 2u))
            e->seen |= 1u;
        if (position == 4 && (qposition & 4u))
            e->seen |= 2u;
        if (home) {
            e->phase = RETURN;
            e->quiet = 0;
            e->latch = (e->qualified >> 3) & 1u;
            e->disturbed = ((e->raw >> 3) & 1u) != e->latch;
        }
        break;
    case RETURN:
        /* HOME must remain raw and qualified throughout this finite window. */
        if (!home)
            return reject(e);
        if (((e->raw >> 3) & 1u) != e->latch ||
            ((e->qualified >> 3) & 1u) != e->latch)
            e->disturbed = 1;
        ++e->quiet;
        if (e->quiet == WINDOW) {
            unsigned result = e->disturbed ? 2u : e->latch;
            if (!increment(&e->completed) || !increment(&e->bins[e->seen][result]))
                return false;
            e->phase = BASELINE;
            e->quiet = 0;
            e->age = 0;
            e->seen = 0;
            e->disturbed = 0;
        }
        break;
    default:
        break;
    }
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->fault) {
        /* Heartbeats remain available but no new acquisition is claimed. */
        e->now = now;
        return;
    }
    if (raw > 15) {
        e->now = now;
        engine_fault(e, F_CONFIG);
        return;
    }
    if (e->started && now - e->now != 1u) {
        e->now = now;
        ++e->gaps;
        engine_fault(e, F_GAP);
        return;
    }
    Engine next = *e;
    next.now = now;
    next.started = true;
    uint8_t old_raw = next.raw;
    uint8_t old_q = next.qualified;
    next.raw = raw;
    qualify(&next, raw);
    if (!increment(&next.sequence) || !process(&next, old_raw, old_q)) {
        /* Roll back the entire acquisition before abandoning an active trip. */
        e->now = now;
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (e->phase > HALT || e->raw > 15 || e->qualified > 15 || e->valid > 15)
        return false;
    if (e->seen > 3 || e->latch > 1 || e->disturbed > 1 || e->fault > 15)
        return false;
    if ((e->phase == HALT) != (e->fault != 0))
        return false;
    if (e->age >= DEADLINE || e->quiet > WINDOW || e->gaps > 1)
        return false;
    const uint32_t counters[] = {e->sequence, e->starts, e->completed,
                                e->rejected, e->tx_errors};
    for (unsigned i = 0; i < sizeof counters / sizeof counters[0]; ++i)
        if (counters[i] > LIMIT)
            return false;
    uint64_t total = 0;
    for (unsigned row = 0; row < 4; ++row) {
        for (unsigned col = 0; col < 3; ++col) {
            if (e->bins[row][col] > LIMIT)
                return false;
            total += e->bins[row][col];
        }
    }
    if (total != e->completed)
        return false;
    if ((uint64_t)e->completed + e->rejected + active(e) != e->starts)
        return false;
    if (active(e) && e->valid != 15)
        return false;
    if (e->phase == RETURN && (e->raw & 7u) != 1)
        return false;
    if (e->phase == READY && e->quiet != WINDOW)
        return false;
    return true;
}
