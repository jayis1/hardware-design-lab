/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

/* AB is qualified as a pair: independent phase filters could invent a
 * direction. HOME and PAWL are independent so chatter cannot mask a stroke. */
static uint8_t qualify(Qualifier *q, uint8_t value, uint8_t required)
{
    if (q->candidate != value) {
        q->candidate = value;
        q->count = 1;
    } else if (q->count < required) {
        ++q->count;
    }
    if (q->count >= required)
        q->accepted = value;
    return q->accepted;
}

static void restart_qualification(Engine *e, uint8_t raw)
{
    e->encoder_anchored = false;
    e->quadrature.candidate = raw & 3u;
    e->home.candidate = (raw >> 2) & 1u;
    e->pawl.candidate = (raw >> 3) & 1u;
    e->quadrature.count = 0;
    e->home.count = 0;
    e->pawl.count = 0;
    e->baseline = 0;
}

void engine_init(Engine *e)
{
    /* Explicit initialization also works in the freestanding target. */
    unsigned char *p = (unsigned char *)e;
    for (size_t i = 0; i < sizeof *e; ++i)
        p[i] = 0;
    e->phase = WAIT;
}

static void abandon(Engine *e)
{
    /* Conservation guarantees room: an active start has not yet been
     * represented by completed or aborted. */
    if (e->phase == ACTIVE)
        ++e->aborted;
    e->phase = WAIT;
    e->baseline = 0;
}

void engine_fault(Engine *e, uint32_t fault)
{
    e->faults |= fault;
    abandon(e);
    e->phase = HALTED;
}

void engine_tx_error(Engine *e)
{
    e->faults |= F_TX;
    if (e->tx_errors == UINT32_MAX) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    ++e->tx_errors;
}

static int direction(uint8_t before, uint8_t after)
{
    static const int8_t table[16] = {
        0, 1, -1, 2,
        -1, 0, 2, 1,
        1, 2, 0, -1,
        2, -1, 1, 0
    };
    return table[(before << 2) | after];
}

static bool start_trial(Engine *e)
{
    if (e->starts == UINT32_MAX) {
        engine_fault(e, F_OVERFLOW);
        return false;
    }
    ++e->starts;
    e->phase = ACTIVE;
    e->duration = 0;
    e->forward = 0;
    e->reverse = 0;
    e->seated_reverse = 0;
    e->rollback = 0;
    e->position = 0;
    e->peak = 0;
    e->baseline = 0;
    return true;
}

static bool record_step(Engine *e, int step, bool seated)
{
    if (step == 0)
        return true;
    /* Bounded position and counts avoid signed overflow and prevent
     * an unlimited trial from turning corrupted wiring into huge angles. */
    if (e->forward + e->reverse >= EDGE_LIMIT) {
        engine_fault(e, F_OVERFLOW);
        return false;
    }
    if (step > 0) {
        ++e->forward;
        ++e->position;
    } else {
        ++e->reverse;
        --e->position;
        if (seated)
            ++e->seated_reverse;
    }
    if (e->position > e->peak)
        e->peak = e->position;
    uint32_t drawdown = (uint32_t)(e->peak - e->position);
    if (drawdown > e->rollback)
        e->rollback = drawdown;
    return true;
}

static void baseline(Engine *e, uint8_t previous, uint8_t raw)
{
    if (e->phase == READY) {
        if (!(e->mask & 8u) || ((previous ^ e->mask) & 3u)) {
            e->phase = WAIT;
            e->baseline = 0;
        }
        return;
    }
    /* Re-arm requires both contacts and stationary qualified AB. Any raw
     * contact disagreement also invalidates this conservative baseline. */
    if ((e->mask & 12u) != 12u || raw != e->mask ||
        ((previous ^ e->mask) & 3u)) {
        e->baseline = 0;
        if (e->phase == READY && (e->mask & 4u))
            e->phase = WAIT;
        return;
    }
    if (e->baseline < 100u)
        ++e->baseline;
    if (e->baseline == 100u)
        e->phase = READY;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    raw &= 15u;
    if (!e->initialized) {
        e->initialized = true;
        e->now = now;
        restart_qualification(e, raw);
        return;
    }
    uint32_t elapsed = now - e->now;
    if (elapsed == 0)
        return;
    e->now = now;
    if (e->phase == HALTED)
        return;
    if (elapsed != 1u) {
        e->faults |= F_GAP;
        abandon(e);
        restart_qualification(e, raw);
        if (e->gaps == UINT32_MAX) {
            engine_fault(e, F_OVERFLOW);
            return;
        }
        ++e->gaps;
        return;
    }
    uint8_t previous = e->mask;
    uint8_t ab = qualify(&e->quadrature, raw & 3u, 3);
    uint8_t home = qualify(&e->home, (raw >> 2) & 1u, 10);
    uint8_t pawl = qualify(&e->pawl, (raw >> 3) & 1u, 10);
    e->mask = (uint8_t)(ab | (home << 2) | (pawl << 3));
    int step = direction(previous & 3u, ab);
    if (!e->encoder_anchored) {
        step = 0;
        if (e->quadrature.count >= 3u)
            e->encoder_anchored = true;
    }
    if (step == 2) {
        e->faults |= F_QUADRATURE;
        abandon(e);
        return;
    }
    /* Deadline precedence: completion at exactly the limit is rejected. */
    if (e->phase == ACTIVE) {
        ++e->duration;
        if (e->duration >= TRIAL_LIMIT) {
            e->faults |= F_TIMEOUT;
            abandon(e);
            return;
        }
    }
    if (e->phase == READY && (previous & 4u) && !home) {
        if (!start_trial(e))
            return;
    }
    if (e->phase == ACTIVE) {
        bool seated = (previous & e->mask & raw & 8u) != 0;
        if (!record_step(e, step, seated))
            return;
        if (!(previous & 4u) && home) {
            ++e->completed;
            e->phase = WAIT;
            e->baseline = 0;
        }
    } else {
        baseline(e, previous, raw);
    }
}

bool engine_valid(const Engine *e)
{
    if (!e || e->phase > HALTED || e->mask > 15u)
        return false;
    uint64_t accounted = (uint64_t)e->completed + e->aborted;
    if (e->phase == ACTIVE)
        ++accounted;
    if (accounted != e->starts)
        return false;
    if (e->duration > TRIAL_LIMIT ||
        (e->phase == ACTIVE && e->duration == TRIAL_LIMIT))
        return false;
    if ((uint64_t)e->forward + e->reverse > EDGE_LIMIT)
        return false;
    if (e->seated_reverse > e->reverse || e->rollback > e->reverse)
        return false;
    if (e->reverse && !e->rollback)
        return false;
    /* Net negative travel alone supplies a lower bound on drawdown. */
    if (e->reverse > e->forward && e->rollback < e->reverse - e->forward)
        return false;
    if (!e->starts && (e->duration || e->forward || e->reverse))
        return false;
    if (e->phase == READY && (e->mask & 12u) != 12u)
        return false;
    if (e->phase == ACTIVE && (e->mask & 4u))
        return false;
    if (e->faults & ~127u)
        return false;
    if (e->phase == HALTED && !(e->faults & (F_CONFIG | F_WATCHDOG | F_OVERFLOW)))
        return false;
    if (e->gaps && !(e->faults & F_GAP))
        return false;
    if (e->tx_errors && !(e->faults & F_TX))
        return false;
    return true;
}
