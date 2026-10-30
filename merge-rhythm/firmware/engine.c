/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

static bool add(uint32_t *value, uint32_t amount)
{
    if (amount > COUNTER_MAX || *value > COUNTER_MAX - amount)
        return false;
    *value += amount;
    return true;
}

static unsigned sources(uint8_t rises)
{
    return (rises & 1u) + ((rises >> 1) & 1u);
}

/* Loss never guesses an empty chute. Only a new operator baseline can arm. */
static void lose(Engine *e)
{
    e->abandoned += e->pending;
    e->pending = 0;
    e->head = 0;
    e->state = WAIT;
    e->last = 2;
    e->streak = 0;
    e->arm_ms = 0;
    e->clear_ready = false;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->last = 2;
}

void engine_fault(Engine *e, uint32_t reason)
{
    lose(e);
    e->state = HALT;
    e->fault |= reason;
}

void engine_tx_error(Engine *e)
{
    if (!add(&e->tx, 1))
        engine_fault(e, F_OVERFLOW);
}

/* Independent qualification prevents a noisy second gate hiding the first.
 * A stable count is consecutive samples, not elapsed time across gaps. */
static void qualify(Engine *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw ^ e->candidate) & bit) {
            e->candidate ^= bit;
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY) {
            e->known |= bit;
            e->mask = (uint8_t)((e->mask & ~bit) | (raw & bit));
        }
    }
}

static bool baseline(Engine *e, uint8_t raw)
{
    if (e->known != 15u) {
        e->arm_ms = 0;
        return true;
    }
    if (e->mask == 0 && raw == 0) {
        e->clear_ready = true;
        e->arm_ms = 0;
    } else if (e->mask == 8u && raw == 8u && e->clear_ready) {
        if (++e->arm_ms >= ARM_MS) {
            if (!add(&e->segments, 1))
                return false;
            e->state = ARMED;
            e->clear_ready = false;
            e->arm_ms = 0;
        }
    } else {
        e->arm_ms = 0;
        /* Clear itself needs qualification time; only gate activity revokes
         * the prior empty baseline while that press is being qualified. */
        if ((e->mask | raw) & 7u)
            e->clear_ready = false;
    }
    return true;
}

static bool retire(Engine *e)
{
    uint8_t tag = e->tags[e->head];
    uint32_t latency = e->now - e->entered[e->head];
    if (!latency || latency >= TIMEOUT_MS)
        return false;
    if (!add(tag ? &e->right : &e->left, 1))
        return false;
    if (!add(&e->latency_sum, latency))
        return false;
    if (latency > e->latency_max)
        e->latency_max = latency;
    if (tag == e->last) {
        if (!add(&e->streak, 1))
            return false;
    } else {
        if (e->last != 2u && !add(&e->alternations, 1))
            return false;
        e->streak = 1;
    }
    if (e->streak > e->longest)
        e->longest = e->streak;
    e->last = tag;
    e->head = (uint8_t)((e->head + 1u) % QUEUE_CAP);
    --e->pending;
    return true;
}

static bool append(Engine *e, uint8_t tag)
{
    if (!add(&e->admitted, 1))
        return false;
    unsigned tail = (e->head + e->pending) % QUEUE_CAP;
    e->tags[tail] = tag;
    e->entered[tail] = e->now;
    ++e->pending;
    return true;
}

static bool process(Engine *e, uint8_t rises)
{
    unsigned incoming = sources(rises);
    /* Clear and timeout take precedence over accepting an outlet edge. */
    if (rises & 8u) {
        if (!add(&e->rejected, incoming))
            return false;
        lose(e);
        return true;
    }
    if (e->pending && e->now - e->entered[e->head] >= TIMEOUT_MS) {
        if (!add(&e->timeouts, 1) || !add(&e->rejected, incoming))
            return false;
        lose(e);
        return true;
    }
    if (e->mask & 8u) {
        /* A feed before release invalidates the operator baseline. */
        if (e->mask & 7u) {
            if (!add(&e->rejected, incoming))
                return false;
            lose(e);
        }
        return true;
    }
    if (incoming == 2) {
        if (!add(&e->ambiguous, 1) || !add(&e->rejected, 2))
            return false;
        lose(e);
        return true;
    }
    if (rises & 4u) {
        if (!e->pending) {
            if (!add(&e->orphans, 1) || !add(&e->rejected, incoming))
                return false;
            lose(e);
            return true;
        }
        if (!retire(e))
            return false;
    }
    if (incoming) {
        if (e->pending == QUEUE_CAP) {
            if (!add(&e->full, 1) || !add(&e->rejected, 1))
                return false;
            lose(e);
            return true;
        }
        if (!append(e, (rises & 2u) ? 1u : 0u))
            return false;
    }
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->initialized && now == e->now)
        return;
    uint32_t delta = now - e->now;
    e->now = now;
    if (e->state == HALT)
        return;
    if (e->initialized && delta != 1u) {
        if (!add(&e->gaps, 1)) {
            engine_fault(e, F_OVERFLOW);
            return;
        }
        lose(e);
        e->known = 0;
        for (unsigned i=0; i<4; ++i)
            e->stable[i] = 0;
    }
    e->initialized = true;
    raw &= 15u;
    uint8_t previous = e->mask;
    qualify(e, raw);
    /* Stage all coupled counters. Exhaustion must not publish half an
     * outlet-plus-inlet transaction or lose the old queue accounting. */
    Engine next = *e;
    bool ok = next.state == WAIT ? baseline(&next, raw) :
              process(&next, (uint8_t)(next.mask & ~previous));
    if (!ok) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    if (!engine_valid(&next)) {
        engine_fault(e, F_INTERNAL);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    uint64_t delivered = (uint64_t)e->left + e->right;
    if (e->state > HALT || e->mask > 15 || e->known > 15 ||
        e->pending > QUEUE_CAP || e->head >= QUEUE_CAP || e->last > 2)
        return false;
    if ((e->state == HALT) != (e->fault != 0) || e->fault > 15)
        return false;
    if (e->state != ARMED && (e->pending || e->streak || e->last != 2))
        return false;
    if (e->state == ARMED && (e->known != 15 || !e->segments))
        return false;
    if ((uint64_t)e->admitted != delivered + e->abandoned + e->pending)
        return false;
    if (e->longest > delivered || e->streak > e->longest)
        return false;
    if (delivered && (!e->longest || e->alternations >= delivered))
        return false;
    if (!delivered && (e->alternations || e->latency_sum || e->latency_max))
        return false;
    if (e->latency_max >= TIMEOUT_MS || e->latency_sum < delivered ||
        e->latency_sum < e->latency_max ||
        (uint64_t)e->latency_sum > delivered * e->latency_max)
        return false;
    if (e->last == 2 && e->streak)
        return false;
    if (e->last != 2 && !e->streak)
        return false;
    const uint32_t counters[] = {
        e->admitted, e->left, e->right, e->abandoned, e->rejected,
        e->alternations, e->longest, e->streak, e->latency_sum, e->latency_max,
        e->segments, e->ambiguous, e->orphans, e->full, e->timeouts, e->gaps, e->tx
    };
    for (unsigned i=0; i<sizeof counters/sizeof counters[0]; ++i)
        if (counters[i] > COUNTER_MAX)
            return false;
    return true;
}
