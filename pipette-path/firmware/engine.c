/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

static bool active(const Engine *e)
{
    return e->phase >= APPROACH;
}

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

/* Conservation guarantees room for one abort whenever active is true. */
static void abandon(Engine *e, enum Reason reason)
{
    if (active(e)) {
        ++e->aborted;
        e->last_ms = e->elapsed;
        e->last_reason = reason;
    }
    e->elapsed = 0;
    e->phase = WAIT;
}

void engine_fault(Engine *e, uint32_t flag)
{
    abandon(e, E_HARDWARE);
    e->faults |= flag;
}

void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx_errors))
        engine_fault(e, F_OVERFLOW);
}

/* Channels qualify independently. Unknown time must never count as stability. */
static void qualify(Engine *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw ^ e->raw) & bit)
            e->stable[i] = 1;
        else if (e->stable[i] < QUALIFY_MS)
            ++e->stable[i];
        if (e->stable[i] == QUALIFY_MS) {
            e->qualified |= bit;
            e->levels = (uint8_t)((e->levels & ~bit) | (raw & bit));
        }
    }
    e->raw = raw;
}

static bool valid_position(unsigned position)
{
    /* No contact in transit, home, first, or first+second. */
    return position == 0 || position == 1 || position == 2 || position == 6;
}

static void reject(Engine *e, enum Reason reason)
{
    abandon(e, reason);
}

static bool advance(Engine *e, uint8_t previous)
{
    unsigned position = e->levels & 7u;
    unsigned old_position = previous & 7u;
    bool source = (e->levels & 8u) != 0;
    bool old_source = (previous & 8u) != 0;
    bool entered = position != old_position;

    if (active(e)) {
        ++e->elapsed;
        if (e->elapsed >= TIMEOUT_MS) {
            reject(e, E_TIMEOUT);
            return true;
        }
    }
    if (e->phase == WAIT) {
        if (e->levels == 1u)
            e->phase = READY;
        return true;
    }
    if (e->phase == READY) {
        if (position == 1u) {
            /* Immersion while home disables readiness until dry home. */
            if (source)
                e->phase = WAIT;
            return true;
        }
        if (!increment(&e->starts))
            return false;
        e->phase = APPROACH;
        e->elapsed = 0;
        if (source || old_source) {
            reject(e, E_SOURCE);
            return true;
        }
    }
    if (!valid_position(position)) {
        reject(e, E_CODE);
        return true;
    }
    switch (e->phase) {
    case APPROACH:
        if (source)
            reject(e, E_SOURCE);
        else if (position == 2u)
            e->phase = FIRST;
        else if (position != 0u)
            reject(e, E_ORDER);
        break;
    case FIRST:
        if (source) {
            /* Pedal must be asserted while already at the first stop.
             * Simultaneous first-stop/pedal qualification is ambiguous. */
            if (position == 2u && old_position == 2u && !old_source)
                e->phase = IMMERSED;
            else
                reject(e, E_SOURCE);
        } else if (position != 2u) {
            reject(e, E_ORDER);
        }
        break;
    case IMMERSED:
        if (!source)
            reject(e, E_SOURCE);
        else if (position == 1u)
            e->phase = ASPIRATED;
        else if (position == 6u)
            reject(e, E_ORDER);
        break;
    case ASPIRATED:
        /* Removal must occur while still at home, before departure. */
        if (position != 1u)
            reject(e, E_SOURCE);
        else if (!source)
            e->phase = REMOVED;
        break;
    case REMOVED:
        if (source) {
            reject(e, E_SOURCE);
            break;
        }
        if (position == 6u)
            reject(e, E_ORDER);
        else if (position == 2u) {
            /* Source removal must precede pressing again. */
            if (source || old_source)
                reject(e, E_SOURCE);
            else
                e->phase = DISPENSE;
        } else if (position == 0u && source) {
            reject(e, E_SOURCE);
        }
        break;
    case DISPENSE:
        if (source)
            reject(e, E_SOURCE);
        else if (position == 6u)
            e->phase = BLOWOUT;
        else if (entered && position == 1u)
            reject(e, E_ORDER);
        break;
    case BLOWOUT:
        if (source) {
            reject(e, E_SOURCE);
        } else if (position == 1u) {
            if (!increment(&e->completed))
                return false;
            e->last_ms = e->elapsed;
            e->last_reason = E_NONE;
            e->elapsed = 0;
            e->phase = READY;
        }
        break;
    default:
        break;
    }
    return true;
}

static bool step(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - e->now;
    e->now = now;
    if (e->faults)
        return true;
    if (!e->initialized || delta != 1u) {
        if (e->initialized) {
            if (!increment(&e->gaps))
                return false;
            abandon(e, E_GAP);
        }
        e->initialized = true;
        e->qualified = 0;
        for (unsigned i = 0; i < 4; ++i)
            e->stable[i] = 0;
        e->raw = raw;
    }
    uint8_t previous = e->levels;
    qualify(e, raw);
    if (e->qualified != 15u)
        return true;
    return advance(e, previous);
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (raw > 15u) {
        engine_fault(e, F_CONFIG);
        return;
    }
    /* Stage changes: overflow cannot partially publish a new trial. */
    Engine next = *e;
    if (step(&next, now, raw)) {
        *e = next;
    } else {
        engine_fault(e, F_OVERFLOW);
        e->now = now;
    }
}

bool engine_valid(const Engine *e)
{
    if (!e || e->phase > REMOVED || e->faults > 7u)
        return false;
    if (e->levels > 15u || e->qualified > 15u)
        return false;
    if (e->last_reason > E_HARDWARE)
        return false;
    if (e->elapsed >= TIMEOUT_MS || e->last_ms > TIMEOUT_MS)
        return false;
    uint64_t accounted = (uint64_t)e->completed + e->aborted + active(e);
    if (accounted != e->starts)
        return false;
    if (e->faults && e->phase != WAIT)
        return false;
    if (!active(e) && e->elapsed)
        return false;
    if (e->phase != WAIT && e->qualified != 15u)
        return false;
    if (e->phase == READY && e->levels != 1u)
        return false;
    /* Every accepted active phase has a bounded contact-code set. */
    if (active(e) && !valid_position(e->levels & 7u))
        return false;
    if (e->phase == APPROACH && e->levels != 0u)
        return false;
    if (e->phase == FIRST && e->levels != 2u)
        return false;
    if (e->phase == IMMERSED && e->levels != 8u && e->levels != 10u)
        return false;
    if (e->phase == ASPIRATED && e->levels != 9u)
        return false;
    if (e->phase == REMOVED && e->levels != 0u && e->levels != 1u)
        return false;
    if (e->phase == DISPENSE && e->levels != 0u && e->levels != 2u)
        return false;
    if (e->phase == BLOWOUT && e->levels != 0u && e->levels != 2u && e->levels != 6u)
        return false;
    if (!e->completed && !e->aborted && (e->last_ms || e->last_reason))
        return false;
    if (e->last_reason && !e->aborted)
        return false;
    return true;
}
