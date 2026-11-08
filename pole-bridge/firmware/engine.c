/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

static bool endpoint(uint32_t mask)
{
    return mask == 5u || mask == 10u;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint32_t fault)
{
    if (e->state == HALT)
        return;
    /* A started capture already reserved its counter credit. Faults never
     * replace the last fully classified result with an unfinished capture. */
    if (e->state == ACTIVE)
        ++e->aborted;
    e->work = (Capture){0};
    e->settle = 0;
    e->target = 0;
    e->anchor = 0;
    e->state = HALT;
    e->fault = fault;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors < LIMIT)
        ++e->tx_errors;
}

static void qualify(Engine *e, uint8_t raw)
{
    /* Independent channels: chatter on pole two cannot postpone pole one. */
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t level = (raw >> i) & 1u;
        if (!e->stable[i] || e->candidate[i] != level) {
            e->candidate[i] = level;
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY) {
            uint32_t bit = 1u << i;
            e->valid |= bit;
            if (level)
                e->qualified |= bit;
            else
                e->qualified &= ~bit;
        }
    }
}

static bool settled_endpoint(const Engine *e)
{
    return e->valid == 15 && e->raw == e->qualified && endpoint(e->raw);
}

static void settlement(Engine *e, const Engine *previous)
{
    /* Credit only intervals with identical raw AND qualified endpoints.
     * The first qualifying observation starts at zero, not at one. */
    if (!settled_endpoint(e)) {
        e->settle = 0;
        e->target = 0;
        return;
    }
    if (settled_endpoint(previous) && previous->raw == e->raw &&
        previous->target == e->raw) {
        if (e->settle < SETTLE)
            ++e->settle;
    } else {
        e->settle = 0;
    }
    e->target = e->raw;
}

static void integrate(Engine *e, const Engine *previous)
{
    ++e->work.age;
    for (unsigned pole = 0; pole < 2; ++pole) {
        unsigned shift = 2u * pole;
        unsigned before = (previous->raw >> shift) & 3u;
        unsigned after = (e->raw >> shift) & 3u;
        bool known = before == after &&
            ((previous->qualified >> shift) & 3u) == before &&
            ((e->qualified >> shift) & 3u) == after;
        if (known)
            ++e->work.bins[4u * pole + after];
        else
            ++e->work.unknown[pole];
    }
    /* Cross-pole source disagreement is NOT same-pole bridging. The former
     * has exactly one closed throw on each pole, but different source labels. */
    if ((e->raw == 6 || e->raw == 9) && e->raw == previous->raw &&
        e->qualified == e->raw && previous->qualified == e->raw)
        ++e->work.mixed;
}

static void finish(Engine *e, uint32_t outcome)
{
    e->work.outcome = outcome;
    e->last = e->work;
    if (outcome == TRANSFER)
        ++e->completed;
    else if (outcome == RETURN)
        ++e->returned;
    else
        ++e->timeouts;
    e->work = (Capture){0};
    e->settle = 0;
    e->target = 0;
    e->anchor = 0;
    /* Requalify a stable baseline, even when completion ended on it. */
    e->state = WAIT;
}

static bool account(Engine *e, const Engine *previous)
{
    if (e->state == WAIT) {
        settlement(e, previous);
        if (e->settle == SETTLE) {
            e->anchor = e->target;
            e->target = 0;
            e->settle = 0;
            e->state = READY;
        }
        return true;
    }
    if (e->state == READY) {
        if (e->qualified == e->anchor)
            return true;
        if (e->starts == LIMIT)
            return false;
        ++e->starts;
        e->work = (Capture){.origin = e->anchor};
        e->state = ACTIVE;
        settlement(e, previous);
        return true;
    }
    integrate(e, previous);
    /* Reject the deadline before considering a tied settlement completion. */
    if (e->work.age >= DEADLINE) {
        finish(e, TIMEOUT);
        return true;
    }
    settlement(e, previous);
    if (e->settle == SETTLE)
        finish(e, e->target == e->work.origin ? RETURN : TRANSFER);
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - e->now;
    if (e->state == HALT) {
        e->now = now;
        return;
    }
    if (raw > 15) {
        e->now = now;
        engine_fault(e, F_CONFIG);
        return;
    }
    if (e->seen && delta != 1) {
        e->now = now;
        e->gaps = 1;
        engine_fault(e, F_GAP);
        return;
    }
    if (e->sequence == UINT32_MAX) {
        e->now = now;
        engine_fault(e, F_COUNTER);
        return;
    }
    /* Transactional update prevents exhaustion from exposing a partial trial. */
    Engine next = *e;
    next.now = now;
    next.seen = true;
    ++next.sequence;
    next.raw = raw;
    qualify(&next, raw);
    if (!account(&next, e)) {
        e->now = now;
        engine_fault(e, F_COUNTER);
        return;
    }
    *e = next;
}

static bool capture_valid(const Capture *c, bool active)
{
    if (!c->origin) {
        if (c->age || c->outcome || c->mixed)
            return false;
    } else if (!endpoint(c->origin)) {
        return false;
    }
    if (c->age > DEADLINE || c->outcome > TIMEOUT)
        return false;
    if (active && (c->outcome || !c->origin || c->age >= DEADLINE))
        return false;
    if (!active && c->origin && !c->outcome)
        return false;
    if (c->outcome == TIMEOUT && c->age != DEADLINE)
        return false;
    if ((c->outcome == TRANSFER || c->outcome == RETURN) &&
        (c->age < SETTLE || c->age >= DEADLINE))
        return false;
    for (unsigned p = 0; p < 2; ++p) {
        uint64_t sum = c->unknown[p];
        for (unsigned k = 0; k < 4; ++k)
            sum += c->bins[p * 4 + k];
        if (sum != c->age)
            return false;
        if (c->mixed > c->bins[p * 4 + 1] + c->bins[p * 4 + 2])
            return false;
        if (c->outcome == TRANSFER || c->outcome == RETURN) {
            unsigned mask = c->outcome == RETURN ? c->origin : 15u - c->origin;
            unsigned code = (mask >> (p * 2)) & 3u;
            if (c->bins[p * 4 + code] < SETTLE)
                return false;
        }
    }
    return true;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->state > HALT)
        return false;
    if (e->fault != 0 && e->fault != F_CONFIG && e->fault != F_WATCHDOG &&
        e->fault != F_GAP && e->fault != F_COUNTER)
        return false;
    if ((e->state == HALT) != (e->fault != 0))
        return false;
    if (e->raw > 15 || e->qualified > 15 || e->valid > 15)
        return false;
    if (e->gaps != (e->fault == F_GAP) || e->tx_errors > LIMIT)
        return false;
    if (e->starts > LIMIT || e->completed > LIMIT || e->returned > LIMIT ||
        e->timeouts > LIMIT || e->aborted > 1)
        return false;
    if ((uint64_t)e->completed + e->returned + e->timeouts + e->aborted +
        (e->state == ACTIVE) != e->starts)
        return false;
    if (e->aborted && e->state != HALT)
        return false;
    if (e->state == READY || e->state == ACTIVE) {
        if (!endpoint(e->anchor) || e->valid != 15)
            return false;
    } else if (e->anchor) {
        return false;
    }
    if (e->state == READY && e->qualified != e->anchor)
        return false;
    if (e->settle > SETTLE || (e->target && !endpoint(e->target)))
        return false;
    if (e->settle && (!e->target || e->target != e->raw || e->raw != e->qualified))
        return false;
    if ((e->state == READY || e->state == HALT) && (e->settle || e->target))
        return false;
    if (!capture_valid(&e->work, e->state == ACTIVE) || !capture_valid(&e->last, false))
        return false;
    if ((e->state == ACTIVE) != (e->work.origin != 0))
        return false;
    if (e->state == ACTIVE && e->work.origin != e->anchor)
        return false;
    if ((e->last.outcome != 0) != (e->completed + e->returned + e->timeouts != 0))
        return false;
    if ((e->last.outcome == TRANSFER && !e->completed) ||
        (e->last.outcome == RETURN && !e->returned) ||
        (e->last.outcome == TIMEOUT && !e->timeouts))
        return false;
    return true;
}
