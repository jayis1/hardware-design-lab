/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>
static bool active(const Engine *e)
{
    return e->state == WATCH || e->state == PAUSED;
}
/* Every increment is staged by engine_sample; no half-published update. */
static bool increment(uint32_t *v)
{
    if (*v == UINT32_MAX)
        return false;
    ++*v;
    return true;
}
static void close_pause(Engine *e)
{
    if (e->pause_age > e->pause_max)
        e->pause_max = e->pause_age;
    e->pause_age = 0;
    e->episode_taps = 0;
}
static void abort_run(Engine *e)
{
    /* Conservation guarantees these increments have headroom: an active
     * episode already owns one of the bounded starts/pauses counters. */
    if (e->state == PAUSED) {
        ++e->abandoned;
        close_pause(e);
    }
    if (active(e))
        ++e->aborted;
    e->state = WAIT;
    e->silent = 0;
    e->pulse_active = false;
    e->pulse_age = 0;
}
void engine_init(Engine *e)
{
    *e = (Engine){0};
}
void engine_fault(Engine *e, uint32_t reason)
{
    abort_run(e);
    e->fault |= reason;
    e->state = HALT;
}
void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx))
        engine_fault(e, F_OVERFLOW);
}
static void forget_contacts(Engine *e)
{
    e->known = 0;
    e->mask = 0;
    for (unsigned i = 0; i < CHANNELS; ++i)
        e->contact[i] = (Contact){0};
}
static void qualify(Engine *e, uint8_t mask)
{
    e->known = 0;
    e->mask = 0;
    for (unsigned i = 0; i < CHANNELS; ++i) {
        Contact *c = &e->contact[i];
        uint8_t bit = (mask >> i) & 1u;
        if (!c->age || c->candidate != bit) {
            c->candidate = bit;
            c->age = 1;
        } else if (c->age < QUALIFY_MS) {
            ++c->age;
        }
        if (c->age == QUALIFY_MS) {
            c->stable = bit;
            c->known = true;
        }
        if (c->known) {
            e->known |= 1u << i;
            e->mask |= (uint32_t)c->stable << i;
        }
    }
}
static bool stop(Engine *e, bool normal)
{
    if (!normal) {
        abort_run(e);
        return true;
    }
    if (e->state == PAUSED) {
        ++e->abandoned;
        close_pause(e);
    }
    ++e->finished;
    e->state = READY;
    e->silent = 0;
    e->pulse_active = false;
    e->pulse_age = 0;
    return true;
}
static bool update(Engine *e, uint32_t previous)
{
    uint32_t rise = e->mask & ~previous;
    uint32_t fall = previous & ~e->mask;
    bool enabled = (e->mask & 8u) != 0;
    bool supply = (e->mask & 2u) != 0;
    if (e->state == HALT)
        return true;
    if (e->known != 15u)
        return true;
    if (e->state == WAIT) {
        if (!enabled)
            e->state = READY;
        return true;
    }
    if (e->state == READY) {
        if (!(rise & 8u))
            return true;
        /* Held discharge or intervention is ambiguous at arming. */
        if (!supply || (e->mask & 5u)) {
            e->state = WAIT;
            return true;
        }
        if (!increment(&e->runs))
            return false;
        e->state = WATCH;
        e->silent = 0;
        return true;
    }
    /* Stop/declaration withdrawal wins over coincident discharge. */
    if (!enabled || !supply)
        return stop(e, !enabled);
    if (e->state == PAUSED) {
        if (!increment(&e->pause_age) || !increment(&e->pause_total))
            return false;
        if (e->pause_age > e->pause_max)
            e->pause_max = e->pause_age;
    }
    if (e->pulse_active) {
        if (!increment(&e->pulse_age))
            return false;
        /* At the inclusive timeout, even a falling edge is invalid. */
        if (e->pulse_age >= MAX_PULSE_MS) {
            if (!increment(&e->bad_pulses))
                return false;
            abort_run(e);
            return true;
        }
    }
    if (rise & 1u) {
        e->pulse_active = true;
        e->pulse_age = 0;
    }
    if ((fall & 1u) && e->pulse_active) {
        if (!increment(&e->pulses))
            return false;
        e->pulse_active = false;
        e->pulse_age = 0;
        if (e->state == PAUSED) {
            if (!increment(&e->recovered))
                return false;
            /* A tap qualifying with recovery cannot establish causality or
             * even temporal precedence; only prior observations qualify. */
            if (e->episode_taps && !increment(&e->assisted))
                return false;
            close_pause(e);
            e->state = WATCH;
        }
        e->silent = 0;
        return true;
    }
    if (e->silent < SILENCE_MS)
        ++e->silent;
    if (e->state == WATCH && e->silent == SILENCE_MS) {
        if (!increment(&e->pauses))
            return false;
        e->state = PAUSED;
        e->pause_age = 0;
        e->episode_taps = 0;
        /* An intervention simultaneous with pause onset is not prior. */
        return true;
    }
    if (e->state == PAUSED && (rise & 4u)) {
        if (!increment(&e->taps) || !increment(&e->episode_taps))
            return false;
    }
    return true;
}
void engine_sample(Engine *e, uint32_t now, uint8_t mask)
{
    if (e->sampled && now - e->now != 1u) {
        /* Never turn an acquisition gap into observed no-flow duration. */
        if (e->state != HALT)
            abort_run(e);
        if (!increment(&e->gaps))
            engine_fault(e, F_OVERFLOW);
        forget_contacts(e);
    }
    e->sampled = true;
    e->now = now;
    if (e->state == HALT)
        return;
    Engine next = *e;
    uint32_t previous = next.mask;
    qualify(&next, mask & 15u);
    if (!update(&next, previous)) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}
bool engine_valid(const Engine *e)
{
    if (!e || e->state > HALT || e->mask > 15u || e->known > 15u)
        return false;
    if (e->fault > 7u || ((e->state == HALT) != (e->fault != 0)))
        return false;
    if ((uint64_t)e->finished + e->aborted + active(e) != e->runs)
        return false;
    if ((uint64_t)e->recovered + e->abandoned + (e->state == PAUSED) != e->pauses)
        return false;
    if (e->assisted > e->recovered || e->assisted > e->taps)
        return false;
    if (e->recovered > e->pulses || e->episode_taps > e->taps)
        return false;
    if (e->bad_pulses > e->aborted || (e->pauses && !e->runs))
        return false;
    if ((uint64_t)e->pauses > (uint64_t)e->pulses + e->runs)
        return false;
    if (e->pause_age > e->pause_max || e->pause_max > e->pause_total)
        return false;
    if ((uint64_t)e->pause_total > (uint64_t)e->pauses * e->pause_max)
        return false;
    if (!e->pauses && (e->pause_total || e->taps))
        return false;
    if (e->silent > SILENCE_MS || e->pulse_age >= MAX_PULSE_MS)
        return false;
    if (e->state != PAUSED && (e->pause_age || e->episode_taps))
        return false;
    if (e->state == PAUSED && e->silent != SILENCE_MS)
        return false;
    if (!active(e) && (e->silent || e->pulse_active || e->pulse_age))
        return false;
    if (active(e) && (e->known != 15u || (e->mask & 10u) != 10u))
        return false;
    return true;
}
