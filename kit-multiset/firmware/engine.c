/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>

/* Clearing the struct explicitly avoids a freestanding libc dependency. */
void engine_init(Engine *e)
{
    unsigned char *p = (unsigned char *)e;
    for (unsigned i = 0; i < sizeof *e; ++i)
        p[i] = 0;
    e->state = WAIT;
}

static void retain(Engine *e, uint32_t result)
{
    for (unsigned i = 0; i < 3; ++i)
        e->last[i] = e->count[i];
    e->last_result = result;
    e->last_age = e->age;
}

static void abandon(Engine *e)
{
    if (e->state == ACTIVE) {
        /* starts includes this batch, so aborted has headroom. */
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

/* Qualify each contact independently. A held input emits only one rise. */
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

static bool recipe(const uint32_t *counts)
{
    return counts[0] == 2 && counts[1] == 1 && counts[2] == 1;
}

static void begin(Engine *e)
{
    if (e->starts == UINT32_MAX) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    ++e->starts;
    for (unsigned i = 0; i < 3; ++i)
        e->count[i] = 0;
    e->age = 0;
    e->tainted = 0;
    e->state = ACTIVE;
}

static void finish(Engine *e)
{
    if (!e->tainted && recipe(e->count)) {
        ++e->good;
        retain(e, 1);
    } else {
        ++e->bad;
        retain(e, 2);
    }
    e->state = WAIT;
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
        /* The deadline wins over a tray removal at the same tick. */
        if (e->age >= MAX_BATCH_MS) {
            abandon(e);
            return;
        }
        /* A raw tray glitch with a paddle down loses attribution even
         * when the glitch is too short to qualify as a tray removal. */
        if (!(raw & 8u) && ((raw | e->levels) & 7u))
            e->tainted = 1;
        if (!(e->levels & 8u)) {
            finish(e);
        } else {
            uint32_t rises = e->levels & ~previous & 7u;
            if (rises && !(raw & 8u))
                e->tainted = 1;
            /* Check every increment before mutating any of them. */
            for (unsigned i = 0; i < 3; ++i) {
                if ((rises & (1u << i)) && e->count[i] == UINT32_MAX) {
                    engine_fault(e, F_OVERFLOW);
                    return;
                }
            }
            for (unsigned i = 0; i < 3; ++i) {
                if (rises & (1u << i))
                    ++e->count[i];
            }
        }
    } else if (e->state == READY) {
        /* Docking and bin activation in one observation is not ordered. */
        if (e->levels == 8u && previous == 0u && raw == 8u)
            begin(e);
        else if (e->levels || (raw & 7u))
            e->state = WAIT;
    }
    if (e->state == WAIT && e->levels == 0u && raw == 0u)
        e->state = READY;
}

/* Encoder checks accounting before putting evidence on the wire. */
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
    if (e->age > MAX_BATCH_MS || e->last_age > MAX_BATCH_MS)
        return false;
    if (e->state == ACTIVE && (e->valid != 15u || !(e->levels & 8u)))
        return false;
    if (e->state == READY && (e->valid != 15u || e->levels != 0u))
        return false;
    if ((outcomes == 0) != (e->last_result == 0))
        return false;
    if (e->last_result == 1 && (!e->good || !recipe(e->last)))
        return false;
    if (e->last_result == 2 && !e->bad)
        return false;
    if (e->last_result == 3 && !e->aborted)
        return false;
    return true;
}
