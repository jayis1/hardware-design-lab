/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

static bool active(const Engine *e)
{
    return e->state >= PUSH && e->state <= HOLD;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->reached_ms = ABSENT;
    e->released_ms = ABSENT;
    e->finish_ms = ABSENT;
}

static void finish(Engine *e, uint8_t result)
{
    e->result = result;
    e->finish_ms = e->age;
    e->state = DONE;
}

void engine_fault(Engine *e, uint8_t fault)
{
    /* Preserve an already completed measurement even if transport later fails. */
    if (active(e)) {
        e->result = ABORT;
        e->finish_ms = e->age;
    }
    e->fault |= fault;
    e->state = HALT;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors == UINT32_MAX)
        engine_fault(e, F_EXHAUST);
    else
        ++e->tx_errors;
}

static void qualify(Engine *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw ^ e->candidate) & bit) {
            e->candidate ^= bit;
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY_MS) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY_MS) {
            e->valid |= bit;
            e->qualified = (uint8_t)((e->qualified & ~bit) | (raw & bit));
        }
    }
}

static bool unloaded_endpoint(uint8_t raw, uint8_t qualified, uint8_t valid)
{
    return valid == 15 && raw == qualified && (raw == 1 || raw == 2);
}

static void baseline(Engine *e, const Engine *before)
{
    if (!unloaded_endpoint(e->raw, e->qualified, e->valid)) {
        e->baseline = 0;
        e->origin = 0;
        return;
    }
    /* Credit intervals, not the first newly qualified endpoint. */
    if (unloaded_endpoint(before->raw, before->qualified, before->valid) &&
        e->raw == before->raw) {
        if (e->baseline < BASELINE_MS)
            ++e->baseline;
    } else {
        e->baseline = 0;
    }
    if (e->baseline == BASELINE_MS) {
        e->origin = e->raw;
        e->state = READY;
    }
}

static void advance(Engine *e, const Engine *before)
{
    if (e->state == WAIT) {
        baseline(e, before);
        return;
    }
    uint8_t target = (uint8_t)(3u ^ e->origin);
    uint8_t pressure = e->origin == 1 ? 4 : 8;
    uint8_t opposite = e->origin == 1 ? 8 : 4;
    if (e->state == READY) {
        /* Require the starting endpoint throughout paddle qualification. */
        if ((e->raw & 3) != e->origin || (e->raw & opposite)) {
            e->state = WAIT;
            e->origin = 0;
            e->baseline = 0;
            return;
        }
        if ((e->qualified & pressure) && !(before->qualified & pressure)) {
            e->state = PUSH;
            e->age = 0;
        }
        return;
    }
    /* Deadline wins over all ordinary observations at the same sample. */
    ++e->age;
    if (e->age >= DEADLINE_MS) {
        finish(e, TIMEOUT);
        return;
    }
    if ((e->raw & 3) == 3 || (e->raw & opposite)) {
        finish(e, CONFLICT);
        return;
    }
    if (e->state == PUSH) {
        if (!(e->qualified & pressure)) {
            finish(e, EARLY);
            return;
        }
        /* A raw release cannot be hidden by the qualification delay. */
        if ((e->qualified & 3) == target && (e->raw & 3) == target &&
            (e->raw & pressure)) {
            e->reached_ms = e->age;
            e->state = LOADED;
        }
        return;
    }
    if (e->state == LOADED) {
        if ((e->qualified & 3) != target) {
            finish(e, REBOUND);
            return;
        }
        if (!(e->qualified & 12)) {
            if (e->raw != target) {
                finish(e, REBOUND);
                return;
            }
            e->released_ms = e->age;
            e->hold = 0;
            e->state = HOLD;
        }
        return;
    }
    /* HOLD is deliberately raw-sensitive: noise is inconclusive retention,
     * not permission to infer continuous unobserved endpoint residence. */
    if (e->raw != target || e->qualified != target) {
        finish(e, REBOUND);
        return;
    }
    ++e->hold;
    if (e->hold == HOLD_MS)
        finish(e, RETAINED);
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    Engine before = *e;
    uint32_t dt = now - before.now;
    e->now = now;
    if (before.have_sample && dt == 0)
        return;
    if (e->sequence == UINT32_MAX) {
        engine_fault(e, F_EXHAUST);
        return;
    }
    ++e->sequence;
    if (raw > 15) {
        engine_fault(e, F_INPUT);
        return;
    }
    if (before.state == DONE || before.state == HALT)
        return; /* Freeze input evidence; now is transport time, not acquisition. */
    if (before.have_sample && dt != 1) {
        if (e->gaps != UINT32_MAX)
            ++e->gaps;
        engine_fault(e, F_GAP);
        return;
    }
    e->have_sample = true;
    e->raw = raw;
    qualify(e, raw);
    advance(e, &before);
}

bool engine_valid(const Engine *e)
{
    if (!e || e->state > HALT || e->result > ABORT || e->fault > 31)
        return false;
    if (e->raw > 15 || e->qualified > 15 || e->valid > 15 || e->origin > 2)
        return false;
    if (e->baseline > BASELINE_MS || e->hold > HOLD_MS || e->age > DEADLINE_MS)
        return false;
    if ((e->state == HALT) != (e->fault != 0))
        return false;
    if (e->state == WAIT && (e->origin || e->age))
        return false;
    if (e->state >= READY && e->state <= DONE && !e->origin)
        return false;
    if (e->state <= READY && (e->result || e->age))
        return false;
    if (active(e) && (e->result || e->age >= DEADLINE_MS))
        return false;
    if (e->result) {
        if (e->state < DONE || e->finish_ms != e->age)
            return false;
    } else if (e->finish_ms != ABSENT || e->state == DONE) {
        return false;
    }
    if (e->reached_ms != ABSENT && e->reached_ms > e->age)
        return false;
    if (e->released_ms != ABSENT) {
        if (e->reached_ms == ABSENT || e->released_ms <= e->reached_ms ||
            e->released_ms > e->age || e->hold > e->age - e->released_ms)
            return false;
    } else if (e->hold) {
        return false;
    }
    if (e->state == LOADED && e->reached_ms == ABSENT)
        return false;
    if (e->state == HOLD && (e->released_ms == ABSENT || e->hold >= HOLD_MS))
        return false;
    if (e->result == RETAINED && (e->hold != HOLD_MS || e->age >= DEADLINE_MS))
        return false;
    if (e->result == TIMEOUT && e->age != DEADLINE_MS)
        return false;
    if (e->result == ABORT && e->state != HALT)
        return false;
    if (e->state <= PUSH && (e->reached_ms != ABSENT || e->released_ms != ABSENT))
        return false;
    if (e->state == LOADED && e->released_ms != ABSENT)
        return false;
    if (e->state >= READY && e->state <= DONE &&
        (e->valid != 15 || e->baseline != BASELINE_MS))
        return false;
    if ((e->state == HOLD || e->result == RETAINED) &&
        (e->released_ms == ABSENT || e->hold != e->age - e->released_ms ||
         e->raw != (3u ^ e->origin) || e->qualified != e->raw))
        return false;
    if (e->result == EARLY && e->reached_ms != ABSENT)
        return false;
    return true;
}
