/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>
/* Every state-changing observation is transactional. Arithmetic exhaustion
 * preserves the previously accepted totals and abandons that active stroke. */
static bool add(uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount)
        return false;
    *value += amount;
    return true;
}
static bool increment(uint32_t *value)
{
    return add(value, 1u);
}
static unsigned endpoints(const Engine *e)
{
    return e->mask & (LEFT | RIGHT);
}
static unsigned face(const Engine *e)
{
    return !!(e->mask & FACE_B);
}
static bool endpoint(unsigned bits)
{
    return bits == LEFT || bits == RIGHT;
}
static void forget(Engine *e)
{
    e->state = WAIT;
    e->anchor = 0;
    e->face = 0;
    e->elapsed = 0;
    e->departed = 0;
}
static void abandon(Engine *e)
{
    /* Conservation reserves room: an active stroke already owns one start. */
    if (e->state == ACTIVE)
        ++e->aborted;
    forget(e);
}
void engine_init(Engine *e)
{
    *e = (Engine){0};
}
void engine_fault(Engine *e, uint32_t reason)
{
    abandon(e);
    e->fault |= reason;
    e->state = HALT;
}
void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx))
        engine_fault(e, F_OVERFLOW);
}
static void reset_qualification(Engine *e)
{
    e->known = 0;
    for (unsigned i = 0; i < 4; ++i)
        e->contact[i] = (Contact){0};
}
static void qualify(Engine *e, unsigned raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        Contact *c = &e->contact[i];
        unsigned bit = (raw >> i) & 1u;
        if (!c->age || c->candidate != bit) {
            c->candidate = (uint8_t)bit;
            c->age = 1;
        } else if (c->age < QUALIFY_MS) {
            ++c->age;
        }
        if (c->age == QUALIFY_MS) {
            c->stable = c->candidate;
            c->known = true;
        }
        if (c->known) {
            e->known |= 1u << i;
            if (c->stable)
                e->mask |= 1u << i;
            else
                e->mask &= ~(1u << i);
        }
    }
}
static void baseline(Engine *e)
{
    if ((e->mask & ENGAGED) && endpoint(endpoints(e))) {
        e->state = READY;
        e->anchor = endpoints(e);
        e->face = face(e);
        e->elapsed = 0;
    }
}
static bool rejected(Engine *e, uint32_t *reason)
{
    if (!increment(reason))
        return false;
    abandon(e);
    /* A new qualified endpoint may seed a later stroke, never this one. */
    baseline(e);
    return true;
}
static bool observe(Engine *e, unsigned previous)
{
    unsigned ends = endpoints(e);
    if (ends == (LEFT | RIGHT)) {
        if ((previous & 3u) != 3u && !increment(&e->collisions))
            return false;
        abandon(e);
        return true;
    }
    if (e->state == WAIT) {
        baseline(e);
        return true;
    }
    if (e->state == READY) {
        if (!(e->mask & ENGAGED) || face(e) != e->face) {
            forget(e);
            baseline(e);
            return true;
        }
        if (ends == e->anchor)
            return true;
        if (ends) {
            /* Direct endpoint exchange never proves an intervening stroke. */
            if (!increment(&e->collisions))
                return false;
            forget(e);
            baseline(e);
            return true;
        }
        if (!increment(&e->started))
            return false;
        e->state = ACTIVE;
        e->departed = e->now;
        e->elapsed = 0;
        return true;
    }
    e->elapsed = e->now - e->departed;
    /* Ordering is deliberate: lift, face change, deadline, then arrival.
     * A change simultaneous with arrival cannot earn a valid traversal. */
    if (!(e->mask & ENGAGED))
        return rejected(e, &e->lifts);
    if (face(e) != e->face)
        return rejected(e, &e->flips);
    if (e->elapsed >= MAX_STROKE_MS)
        return rejected(e, &e->timeouts);
    if (!ends)
        return true;
    if (ends == e->anchor)
        return rejected(e, &e->short_returns);
    uint32_t *count = e->face ? &e->full_b : &e->full_a;
    uint32_t *duration = e->face ? &e->ms_b : &e->ms_a;
    if (!increment(count) || !add(duration, e->elapsed))
        return false;
    e->state = READY;
    e->anchor = ends;
    e->elapsed = 0;
    e->departed = 0;
    return true;
}
void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->state == HALT) {
        e->now = now;
        return;
    }
    if (raw > 15u) {
        e->now = now;
        engine_fault(e, F_INPUT);
        return;
    }
    Engine next = *e;
    next.now = now;
    if (next.initialized && now - e->now != 1u) {
        abandon(&next);
        reset_qualification(&next);
        if (!increment(&next.gaps)) {
            e->now = now;
            engine_fault(e, F_OVERFLOW);
            return;
        }
        *e = next;
        return;
    }
    next.initialized = true;
    unsigned previous = next.mask;
    qualify(&next, raw);
    if (next.known == 15u && !observe(&next, previous)) {
        e->now = now;
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}
static bool duration_valid(uint32_t count, uint32_t duration)
{
    /* Qualification gives each completed traversal a positive duration.
     * 64-bit comparison avoids overflow in the upper bound itself. */
    if (!count)
        return duration == 0;
    return duration >= count &&
        (uint64_t)duration <= (uint64_t)count * (MAX_STROKE_MS - 1u);
}
bool engine_valid(const Engine *e)
{
    if (!e || e->state > HALT || e->mask > 15u || e->known > 15u)
        return false;
    if (e->face > 1u || e->fault > 15u)
        return false;
    if ((e->state == HALT) != (e->fault != 0))
        return false;
    uint64_t completed = (uint64_t)e->full_a + e->full_b;
    if ((uint64_t)e->started != completed + e->aborted + (e->state == ACTIVE))
        return false;
    if (!duration_valid(e->full_a, e->ms_a) || !duration_valid(e->full_b, e->ms_b))
        return false;
    uint64_t rejected_count = (uint64_t)e->short_returns + e->lifts + e->flips + e->timeouts;
    if (rejected_count > e->aborted)
        return false;
    if (e->state == WAIT || e->state == HALT) {
        if (e->anchor || e->face || e->elapsed)
            return false;
    } else {
        if (e->known != 15u || !(e->mask & ENGAGED))
            return false;
        if (!endpoint(e->anchor) || e->face != face(e))
            return false;
        if (e->state == READY && (endpoints(e) != e->anchor || e->elapsed))
            return false;
        if (e->state == ACTIVE && (endpoints(e) || e->elapsed >= MAX_STROKE_MS))
            return false;
    }
    return true;
}
