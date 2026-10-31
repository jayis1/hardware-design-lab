/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

/* Starts reserve one eventual outcome. Pulses reserve exactly one bucket,
 * so abandoning the active record cannot overflow either cumulative count. */
static void abandon(Engine *e)
{
    if (e->state != ACTIVE)
        return;
    ++e->aborted;
    e->abandoned += e->active_pulses;
    e->active_pulses = 0;
    e->active_ms = 0;
    e->active_bad = 0;
    e->state = WAIT;
}

void engine_fault(Engine *e, uint8_t flag)
{
    abandon(e);
    e->flags |= flag;
    e->state = HALTED;
}

void engine_tx_error(Engine *e)
{
    if (e->tx == UINT32_MAX)
        engine_fault(e, F_OVERFLOW);
    else
        ++e->tx;
}

/* Each switch gets its own qualification interval. Chatter in the tension
 * flag must not postpone the spool index or the traverse endpoint. */
static void qualify(Engine *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((e->candidate ^ raw) & bit) {
            e->candidate ^= bit;
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY_MS) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY_MS) {
            e->levels = (e->levels & (uint8_t)~bit) | (raw & bit);
            e->known |= bit;
        }
    }
}

static bool unique_side(uint8_t side)
{
    return side == 1 || side == 2;
}

static bool baseline(const Engine *e)
{
    uint8_t side = e->levels & 3u;
    return e->known == 15 && unique_side(side) &&
           (e->raw & 3u) == side &&
           (e->levels & 12u) == 8 && (e->raw & 12u) == 8;
}

static bool increment(uint32_t *value)
{
    if (*value == UINT32_MAX)
        return false;
    ++*value;
    return true;
}

static bool begin(Engine *e)
{
    if (!increment(&e->starts))
        return false;
    e->state = ACTIVE;
    e->active_ms = 0;
    e->active_pulses = 0;
    e->active_bad = !(e->raw & 8u);
    return true;
}

static bool complete(Engine *e, uint8_t destination)
{
    if (!increment(&e->completed))
        return false;
    if (e->active_bad && !increment(&e->bad_total))
        return false;
    e->last_pulses = e->active_pulses;
    e->last_ms = e->active_ms;
    e->last_bad = e->active_bad;
    e->last_end = destination;
    e->completed_pulses += e->active_pulses;
    e->active_pulses = 0;
    e->active_ms = 0;
    e->active_bad = 0;
    e->anchor = destination;
    e->state = baseline(e) ? READY : WAIT;
    return true;
}

/* The caller stages this operation. Any numeric failure discards all of
 * this sample's accounting before the original active record is abandoned. */
static bool advance(Engine *e, uint8_t previous)
{
    uint8_t side = e->levels & 3u;
    bool rising = (e->levels & 4u) && !(previous & 4u);
    if ((e->raw & 3u) == 3 || side == 3) {
        engine_fault(e, F_ENDPOINTS);
        return true;
    }
    if (e->known != 15)
        return true;
    if (e->state == WAIT) {
        if (baseline(e)) {
            e->anchor = side;
            e->state = READY;
        }
        return true;
    }
    if (e->state == READY) {
        if (side == e->anchor)
            return true;
        if (side || (e->raw & 7u) || (e->levels & 4u)) {
            e->state = WAIT;
            return true;
        }
        /* An index already active at departure cannot be assigned to a
         * traverse. A fresh endpoint baseline is then required. */
        return begin(e);
    }
    if (e->state != ACTIVE)
        return true;
    ++e->active_ms;
    if (!(e->raw & 8u) || !(e->levels & 8u))
        e->active_bad = 1;
    /* Deadline takes precedence over arrival. No partial sample is added
     * after the defined maximum observation window. */
    if (e->active_ms >= MAX_TRAVERSE_MS) {
        abandon(e);
        return true;
    }
    if (rising) {
        if (side || (e->raw & 3u)) {
            abandon(e);
            return true;
        }
        if (!increment(&e->pulses) || !increment(&e->active_pulses))
            return false;
    }
    if (side) {
        /* A pulse straddling either boundary cannot be assigned reliably.
         * Same-end returns are attempts, never completed traverses. */
        if (side == e->anchor || (e->raw & 4u) || (e->levels & 4u)) {
            abandon(e);
            return true;
        }
        return complete(e, side);
    }
    return true;
}

void engine_sample(Engine *e, uint32_t time, uint8_t raw)
{
    uint32_t delta = time - e->time;
    if (e->sampled && delta == 0)
        return;
    e->time = time;
    e->raw = raw & 15u;
    if (e->state == HALTED)
        return;
    if (e->sampled && delta != 1) {
        if (e->gaps != UINT32_MAX)
            ++e->gaps;
        else
            e->flags |= F_OVERFLOW;
        e->known = 0;
        engine_fault(e, F_GAP);
        return;
    }
    e->sampled = true;
    uint8_t previous = e->levels;
    qualify(e, e->raw);
    Engine next = *e;
    if (!advance(&next, previous)) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}

/* These are serialization invariants, not proof that mechanics behaved as
 * intended. Use widened arithmetic even when testing corrupted snapshots. */
bool engine_valid(const Engine *e)
{
    uint64_t outcomes = (uint64_t)e->completed + e->aborted;
    uint64_t counted = (uint64_t)e->completed_pulses + e->abandoned;
    if (e->state > HALTED || e->raw > 15 || e->levels > 15 || e->known > 15)
        return false;
    if (e->flags > 31 || (e->state == HALTED) != (e->flags != 0))
        return false;
    if (e->anchor > 2 || e->last_end > 2)
        return false;
    if (e->last_bad > 1 || e->active_bad > 1)
        return false;
    if (e->state == ACTIVE)
        ++outcomes;
    counted += e->active_pulses;
    if (outcomes != e->starts || counted != e->pulses)
        return false;
    if (e->bad_total > e->completed)
        return false;
    if (e->state != ACTIVE && (e->active_pulses || e->active_ms || e->active_bad))
        return false;
    if (e->active_ms >= MAX_TRAVERSE_MS)
        return false;
    if ((e->state == ACTIVE || e->state == READY) && !unique_side(e->anchor))
        return false;
    if (!e->completed) {
        if (e->last_pulses || e->last_ms || e->last_bad || e->last_end ||
            e->completed_pulses)
            return false;
    } else {
        if (!unique_side(e->last_end) || !e->last_ms || e->last_ms >= MAX_TRAVERSE_MS)
            return false;
        if (e->last_pulses > e->completed_pulses || e->last_bad > e->bad_total)
            return false;
        if (e->completed == 1 && (e->last_pulses != e->completed_pulses ||
                                  e->last_bad != e->bad_total))
            return false;
    }
    if (e->gaps && !(e->flags & F_GAP))
        return false;
    return true;
}
