/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

/* Bounded counters are transactional: an unrepresentable observation does
 * not publish partially credited identities or outcomes. */
static bool add(uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount)
        return false;
    *value += amount;
    return true;
}

unsigned engine_identity(unsigned word)
{
    static const uint8_t codes[6] = {3, 5, 6, 9, 10, 12};
    for (unsigned i = 0; i < 6; ++i) {
        if (word == codes[i])
            return i + 1;
    }
    return 0;
}

void engine_init(Engine *e)
{
    unsigned char *p = (unsigned char *)e;
    for (size_t i = 0; i < sizeof *e; ++i)
        p[i] = 0;
    e->state = WAIT_EMPTY;
}

static void discard_active(Engine *e)
{
    if (e->state == ACTIVE) {
        /* starts = completed + abandoned + active proves headroom here. */
        ++e->abandoned;
    }
    e->identity = 0;
    e->changed_here = 0;
    e->invalid_here = 0;
    e->age = 0;
}

void engine_fault(Engine *e, unsigned fault)
{
    if (e->fault)
        return;
    discard_active(e);
    e->fault = (uint8_t)fault;
    e->state = HALT;
    e->valid = 0;
}

void engine_tx_error(Engine *e)
{
    if (e->fault)
        return;
    if (!add(&e->tx_errors, 1))
        engine_fault(e, F_OVERFLOW);
}

/* Returning true means the current raw vector has occupied six successive
 * observations separated by exactly one tick. Old qualification is retained
 * while a new vector settles, but it cannot generate repeated operations. */
static bool qualify(Engine *e, unsigned raw)
{
    e->raw = (uint8_t)raw;
    if (raw != e->candidate) {
        e->candidate = (uint8_t)raw;
        e->stable = 0;
        return false;
    }
    if (e->stable < 5)
        ++e->stable;
    if (e->stable != 5)
        return false;
    e->qualified = (uint8_t)raw;
    e->valid = 1;
    return true;
}

static bool admit(Engine *e, unsigned word)
{
    if (!add(&e->starts, 1))
        return false;
    e->identity = (uint8_t)engine_identity(word);
    e->age = 0;
    e->changed_here = 0;
    e->invalid_here = 0;
    if (e->identity) {
        if (!add(&e->seats[e->identity - 1], 1))
            return false;
    } else {
        if (!add(&e->unidentified, 1) || !add(&e->invalid, 1))
            return false;
        e->invalid_here = 1;
    }
    e->state = ACTIVE;
    return true;
}

static bool observe_active(Engine *e, unsigned word)
{
    if (!word) {
        if (!add(&e->completed, 1))
            return false;
        e->latest_age = e->age;
        e->age = 0;
        e->identity = 0;
        e->changed_here = 0;
        e->invalid_here = 0;
        e->state = READY;
        return true;
    }
    unsigned identity = engine_identity(word);
    if (!identity && !e->invalid_here) {
        if (!add(&e->invalid, 1))
            return false;
        e->invalid_here = 1;
    }
    /* A malformed first observation never acquires an identity retroactively.
     * Different valid codewords are evidence of substitution, not new seats. */
    if (identity && e->identity && identity != e->identity && !e->changed_here) {
        if (!add(&e->changed, 1))
            return false;
        e->changed_here = 1;
    }
    return true;
}

void engine_sample(Engine *e, uint32_t now, unsigned raw)
{
    if (e->fault)
        return;
    if (raw > 15) {
        engine_fault(e, F_INPUT);
        return;
    }
    uint32_t dt = now - e->now;
    if (e->started && (!dt || dt >= 0x80000000u)) {
        engine_fault(e, F_TIME);
        return;
    }
    Engine next = *e;
    next.now = now;
    if (!add(&next.samples, 1))
        goto overflow;
    if (!e->started || dt != 1) {
        if (e->started) {
            if (!add(&next.gaps, 1) || !add(&next.missed, dt - 1))
                goto overflow;
            discard_active(&next);
        }
        next.started = 1;
        next.raw = next.candidate = (uint8_t)raw;
        next.valid = 0;
        next.stable = 0;
        next.state = WAIT_EMPTY;
        *e = next;
        return;
    }
    if (next.state == ACTIVE && !add(&next.age, 1))
        goto overflow;
    if (qualify(&next, raw)) {
        switch (next.state) {
        case WAIT_EMPTY:
            if (!raw)
                next.state = READY;
            break;
        case READY:
            if (raw && !admit(&next, raw))
                goto overflow;
            break;
        case ACTIVE:
            if (!observe_active(&next, raw))
                goto overflow;
            break;
        default:
            engine_fault(e, F_CONFIG);
            return;
        }
    }
    *e = next;
    return;
overflow:
    engine_fault(e, F_OVERFLOW);
}

bool engine_valid(const Engine *e)
{
    if (e->raw > 15 || e->qualified > 15 || e->candidate > 15)
        return false;
    if (e->valid > 1 || e->stable > 5 || e->started > 1)
        return false;
    if (e->state > HALT || e->fault > F_WATCHDOG || e->identity > 6)
        return false;
    if ((e->state == HALT) != (e->fault != 0))
        return false;
    if (e->fault && e->valid)
        return false;
    if (e->changed_here > 1 || e->invalid_here > 1)
        return false;
    if (e->gaps > e->missed || e->gaps > e->samples)
        return false;
    if ((uint64_t)e->completed + e->abandoned + (e->state == ACTIVE) != e->starts)
        return false;
    uint64_t identified = 0;
    for (unsigned i = 0; i < 6; ++i)
        identified += e->seats[i];
    if (identified + e->unidentified != e->starts)
        return false;
    if (e->changed > identified || e->invalid > e->starts)
        return false;
    if (e->invalid < e->unidentified)
        return false;
    if (e->state != ACTIVE && (e->identity || e->age || e->changed_here || e->invalid_here))
        return false;
    if (e->state == ACTIVE) {
        if (!e->valid || !e->qualified)
            return false;
        unsigned current = engine_identity(e->qualified);
        if (!current && !e->invalid_here)
            return false;
        if (current && e->identity && current != e->identity && !e->changed_here)
            return false;
        if (e->identity && !e->seats[e->identity - 1])
            return false;
        if (!e->identity && (!e->unidentified || !e->invalid_here))
            return false;
        if (e->changed_here && (!e->changed || !e->identity))
            return false;
        if (e->invalid_here && !e->invalid)
            return false;
    }
    if (e->state == READY && (!e->valid || e->qualified))
        return false;
    if (!e->completed && e->latest_age)
        return false;
    return true;
}
