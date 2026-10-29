/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

/* All changes on one acquisition tick commit together. If any counter would
 * overflow, discard the speculative tick and abandon the old active slots. */
static bool increment(uint32_t *value)
{
    if (*value == UINT32_MAX)
        return false;
    ++*value;
    return true;
}

static void abandon(Slot *s)
{
    if (s->state == OCCUPIED)
        ++s->aborted; /* starts conservation guarantees room here */
    s->state = WAIT;
    s->age = 0;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint32_t reason)
{
    e->fault |= reason;
    for (unsigned i = 0; i < CHANNELS; ++i)
        abandon(&e->slot[i]);
}

void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx))
        engine_fault(e, F_OVERFLOW);
}

/* Unknown initial levels are not transitions. A pocket first observed closed
 * cannot establish when the coupon was placed; it must become empty first. */
static void qualify(Contact *c, uint8_t raw)
{
    if (raw != c->candidate) {
        c->candidate = raw;
        c->age = 0;
    } else if (c->age < QUALIFY_MS) {
        ++c->age;
    }
    if (c->age >= QUALIFY_MS) {
        c->stable = raw;
        c->known = true;
    }
}

static bool finish(Slot *s)
{
    uint32_t duration = s->age;
    if (UINT32_MAX - s->total < duration)
        return false;
    uint32_t *classification = duration >= GOAL_MS ? &s->rested : &s->early;
    if (!increment(classification))
        return false;
    s->last = duration;
    if (!s->minimum || duration < s->minimum)
        s->minimum = duration;
    if (duration > s->maximum)
        s->maximum = duration;
    s->total += duration;
    s->age = 0;
    s->state = READY;
    return true;
}

static bool update_slot(Slot *s, const Contact *c)
{
    if (!c->known)
        return true;
    switch (s->state) {
    case WAIT:
        if (!c->stable)
            s->state = READY;
        break;
    case READY:
        if (c->stable) {
            if (!increment(&s->starts))
                return false;
            s->state = OCCUPIED;
            s->age = 0;
        }
        break;
    case OCCUPIED:
        /* Credit the preceding qualified interval, including the final one.
         * Timeout wins over removal on the same tick. */
        ++s->age;
        if (s->age >= LIMIT_MS) {
            if (!increment(&s->timeouts))
                return false;
            abandon(s);
            if (!c->stable)
                s->state = READY;
        } else if (!c->stable) {
            return finish(s);
        }
        break;
    default:
        return false;
    }
    return true;
}

static void reset_qualification(Engine *e, uint8_t mask)
{
    for (unsigned i = 0; i < CHANNELS; ++i) {
        e->contact[i] = (Contact){0};
        e->contact[i].candidate = (mask >> i) & 1u;
    }
}

void engine_sample(Engine *e, uint32_t now, uint8_t mask)
{
    if (e->sampled && now == e->now)
        return;
    uint32_t elapsed = now - e->now;
    e->now = now;
    if (mask > 15u) {
        engine_fault(e, F_CONFIG);
        return;
    }
    if (e->fault)
        return;
    if (!e->sampled) {
        e->sampled = true;
        reset_qualification(e, mask);
        return;
    }
    if (elapsed != 1u) {
        if (!increment(&e->gaps)) {
            engine_fault(e, F_OVERFLOW);
            return;
        }
        for (unsigned i = 0; i < CHANNELS; ++i)
            abandon(&e->slot[i]);
        reset_qualification(e, mask);
        return;
    }
    Engine next = *e;
    for (unsigned i = 0; i < CHANNELS; ++i) {
        qualify(&next.contact[i], (mask >> i) & 1u);
        if (!update_slot(&next.slot[i], &next.contact[i])) {
            engine_fault(e, F_OVERFLOW);
            return;
        }
    }
    *e = next;
}

/* Necessary cross-field constraints; shared encoder and host tests refuse
 * impossible records rather than dressing corruption up as measurements. */
static bool slot_valid(const Slot *s)
{
    if (s->state > OCCUPIED)
        return false;
    uint64_t completed = (uint64_t)s->early + s->rested;
    uint64_t accounted = completed + s->aborted + (s->state == OCCUPIED);
    if (accounted != s->starts)
        return false;
    if (s->timeouts > s->aborted)
        return false;
    if (s->state != OCCUPIED && s->age)
        return false;
    if (s->age >= LIMIT_MS)
        return false;
    if (!completed)
        return !(s->last || s->minimum || s->maximum || s->total);
    if (!s->minimum || s->minimum > s->last || s->last > s->maximum)
        return false;
    if (s->maximum >= LIMIT_MS)
        return false;
    if ((uint64_t)s->minimum * completed > s->total)
        return false;
    if ((uint64_t)s->maximum * completed < s->total)
        return false;
    if (s->total < (uint64_t)s->maximum + s->minimum * (completed - 1))
        return false;
    if (!s->rested && s->maximum >= GOAL_MS)
        return false;
    if (!s->early && s->minimum < GOAL_MS)
        return false;
    if (s->early && s->minimum >= GOAL_MS)
        return false;
    if (s->rested && s->maximum < GOAL_MS)
        return false;
    uint64_t lower = (uint64_t)s->rested * GOAL_MS + s->early;
    uint64_t upper = (uint64_t)s->rested * (LIMIT_MS - 1u)
                   + (uint64_t)s->early * (GOAL_MS - 1u);
    if (s->total < lower || s->total > upper)
        return false;
    return true;
}

bool engine_valid(const Engine *e)
{
    if (!e || (e->fault & ~7u))
        return false;
    for (unsigned i = 0; i < CHANNELS; ++i) {
        if (!slot_valid(&e->slot[i]))
            return false;
        if (e->fault && e->slot[i].state != WAIT)
            return false;
    }
    return true;
}
