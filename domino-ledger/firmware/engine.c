/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

/* All cumulative changes occur in a staged copy, including debounce state. */
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
    for (unsigned i = 0; i < 4; ++i)
        e->offset[i] = ABSENT;
}

void engine_fault(Engine *e, uint8_t reason)
{
    if (e->phase == HALT)
        return;
    /* starts bounds aborted, so one pending trial always has counter room. */
    if (e->phase == ACTIVE) {
        ++e->aborted;
        e->outcome = ABORTED;
    }
    e->phase = HALT;
    e->fault = reason;
    e->baseline = 0;
    e->quiet = 0;
    e->prior_up = false;
    e->prior_down = false;
}

void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx_errors))
        engine_fault(e, F_OVERFLOW);
}

static void qualify(Engine *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        uint8_t state = (raw & bit) != 0;
        if (!e->count[i] || e->candidate[i] != state) {
            e->candidate[i] = state;
            e->count[i] = 1;
        } else if (e->count[i] < QUALIFY_SAMPLES) {
            ++e->count[i];
        }
        if (e->count[i] == QUALIFY_SAMPLES) {
            e->valid |= bit;
            if (state)
                e->mask |= bit;
            else
                e->mask &= (uint8_t)~bit;
        }
    }
}

static void capture(Engine *e, uint8_t falls, uint8_t rises)
{
    /* Ties keep equal offsets; there is no fabricated channel ordering. */
    uint8_t fresh = falls & (uint8_t)~e->seen;
    for (unsigned i = 0; i < 4; ++i) {
        if (fresh & (1u << i))
            e->offset[i] = e->age;
    }
    e->seen |= fresh;
    e->rebound |= rises & e->seen;
}

static bool begin(Engine *e, uint8_t falls)
{
    if (!increment(&e->starts))
        return false;
    e->phase = ACTIVE;
    e->outcome = NONE;
    e->age = 0;
    e->seen = 0;
    e->rebound = 0;
    e->quiet = 0;
    e->baseline = 0;
    e->prior_up = false;
    e->prior_down = false;
    for (unsigned i = 0; i < 4; ++i)
        e->offset[i] = ABSENT;
    capture(e, falls, 0);
    return true;
}

static bool finish(Engine *e)
{
    if (!increment(&e->completed))
        return false;
    bool full = e->seen == 15;
    if (!increment(full ? &e->full : &e->partial))
        return false;
    if (e->rebound && !increment(&e->disturbed))
        return false;
    e->outcome = full ? CLEAN_FULL : CLEAN_PARTIAL;
    if (e->rebound)
        e->outcome += 2;
    e->phase = DONE;
    e->quiet = 0;
    e->prior_down = false;
    return true;
}

static void baseline(Engine *e)
{
    bool up = e->valid == 15 && e->mask == 15 && e->raw == 15;
    if (!up) {
        e->baseline = 0;
    } else if (e->prior_up && e->baseline < BASELINE_MS) {
        ++e->baseline;
    }
    e->prior_up = up;
    if (e->baseline == BASELINE_MS)
        e->phase = READY;
}

static bool advance(Engine *e, uint8_t previous)
{
    uint8_t falls = previous & (uint8_t)~e->mask;
    uint8_t rises = e->mask & (uint8_t)~previous;
    if (e->phase == WAIT || e->phase == DONE) {
        baseline(e);
        return true;
    }
    if (e->phase == READY) {
        if (!falls)
            return true;
        if (!begin(e, falls))
            return false;
    } else if (e->phase == ACTIVE) {
        ++e->age;
        /* The deadline has precedence over newly qualifying transitions. */
        if (e->age >= DEADLINE_MS)
            return finish(e);
        capture(e, falls, rises);
    }
    bool down = e->mask == 0 && e->raw == 0 && e->seen == 15;
    if (!down) {
        e->quiet = 0;
    } else if (e->prior_down) {
        ++e->quiet;
    }
    e->prior_down = down;
    if (e->quiet >= QUIET_MS)
        return finish(e);
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (raw > 15) {
        engine_fault(e, F_CONFIG);
        return;
    }
    if (e->clocked && now - e->now != 1u) {
        engine_fault(e, F_GAP);
        e->now = now;
        return;
    }
    e->clocked = true;
    e->now = now;
    if (e->phase == HALT)
        return;
    Engine next = *e;
    uint8_t previous = next.mask;
    next.raw = raw;
    if (!increment(&next.samples)) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    qualify(&next, raw);
    if (!advance(&next, previous)) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->phase > HALT || e->outcome > ABORTED || e->fault > F_WATCHDOG)
        return false;
    if ((e->phase == HALT) != (e->fault != F_NONE))
        return false;
    if (e->raw > 15 || e->mask > 15 || e->valid > 15 || e->seen > 15)
        return false;
    if (e->rebound & (uint8_t)~e->seen)
        return false;
    if ((uint64_t)e->full + e->partial != e->completed)
        return false;
    if ((uint64_t)e->completed + e->aborted + (e->phase == ACTIVE) != e->starts)
        return false;
    if (e->disturbed > e->completed || e->aborted > 1)
        return false;
    if (e->baseline > BASELINE_MS || e->quiet >= QUIET_MS || e->age > DEADLINE_MS)
        return false;
    if (e->phase == ACTIVE && (e->outcome != NONE || e->age >= DEADLINE_MS))
        return false;
    if ((e->starts != 0) != (e->seen != 0))
        return false;
    bool zero = false;
    for (unsigned i = 0; i < 4; ++i) {
        bool present = (e->seen & (1u << i)) != 0;
        if (present) {
            if (e->offset[i] > e->age || e->offset[i] >= DEADLINE_MS)
                return false;
            zero |= e->offset[i] == 0;
        } else if (e->offset[i] != ABSENT) {
            return false;
        }
    }
    if (e->seen && !zero)
        return false;
    if (e->outcome >= CLEAN_FULL && e->outcome <= DISTURBED_PARTIAL) {
        bool full = e->outcome == CLEAN_FULL || e->outcome == DISTURBED_FULL;
        if (full != (e->seen == 15) || (e->outcome >= DISTURBED_FULL) != (e->rebound != 0))
            return false;
        if (!e->completed || (full ? !e->full : !e->partial))
            return false;
    }
    if (e->outcome == ABORTED && (!e->aborted || e->phase != HALT))
        return false;
    return true;
}
