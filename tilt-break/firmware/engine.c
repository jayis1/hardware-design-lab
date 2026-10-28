/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

/* Resistive acquisition is intentionally separate from geometry calibration.
 * The app can apply a measured ohms/mm slope without rewriting evidence. */
uint32_t engine_resistance(uint16_t adc)
{
    if (adc >= 4095u)
        return UINT32_MAX;
    uint32_t r = (100000u * adc) / (4095u - adc);
    return r > 1000u ? r - 1000u : 0;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->state = WAIT;
}

void engine_fault(Engine *e,uint32_t fault)
{
    e->faults |= fault;
    e->state = INVALID;
}

/* Three observations qualify independently. An intermediate voltage breaks
 * the candidate streak instead of being treated as continued stability. */
static bool qualify(Contact *c,uint16_t value,uint32_t now)
{
    bool closed;
    if (value <= 200u)
        closed = true;
    else if (value >= 3800u)
        closed = false;
    else {
        c->count = 0;
        return false;
    }
    if (!c->count || closed != c->candidate) {
        c->candidate = closed;
        c->count = 1;
        c->since = now;
    } else if (c->count < 3u) {
        ++c->count;
    }
    if (c->count == 3u && (!c->known || c->level != closed)) {
        bool changed = c->known;
        c->known = true;
        c->level = closed;
        return changed;
    }
    return false;
}

static bool baseline_inputs(const Engine *e)
{
    return e->contacts[0].known && e->contacts[0].level &&
           e->contacts[1].known && !e->contacts[1].level &&
           e->adc[1] <= 200u && e->adc[2] >= 3800u;
}

static void zero_begin(Engine *e,uint32_t r)
{
    e->state = ZERO;
    e->zero_count = 1;
    e->zero_min = r;
    e->zero_max = r;
}

static void zero_update(Engine *e,uint32_t r)
{
    /* Never tare on a reboot alone, or through a raw button release. */
    if (!baseline_inputs(e) || e->adc[3] > 200u) {
        e->state = WAIT;
        e->zero_count = 0;
        return;
    }
    if (r < e->zero_min)
        e->zero_min = r;
    if (r > e->zero_max)
        e->zero_max = r;
    if (e->zero_max - e->zero_min > 200u) {
        zero_begin(e,r);
        return;
    }
    if (++e->zero_count >= ZERO_SAMPLES) {
        e->baseline = (e->zero_max + e->zero_min) / 2u;
        e->height = 0;
        e->state = READY;
        e->released = false;
    }
}

static bool movement_valid(Engine *e,int32_t h)
{
    int32_t step = h - e->previous;
    if (step > 800 || step < -800) {
        engine_fault(e,F_SPEED);
        return false;
    }
    if (h < -200 || h > 88000) {
        engine_fault(e,F_RANGE);
        return false;
    }
    if (h < e->maximum - 200) {
        engine_fault(e,F_REVERSE);
        return false;
    }
    if (h > e->maximum)
        e->maximum = h;
    e->previous = h;
    return true;
}

static void ramp_begin(Engine *e)
{
    e->state = RAMP;
    e->start = e->now;
    e->elapsed = 0;
    e->previous = e->height;
    e->maximum = e->height;
    e->last_home = e->height;
    e->released = false;
}

static void active_update(Engine *e,const bool edge[3])
{
    e->elapsed = e->now - e->start;
    if (e->elapsed > MAX_RAMP_MS) {
        engine_fault(e,F_TIMEOUT);
        return;
    }
    if (!movement_valid(e,e->height))
        return;
    if (edge[2] && e->contacts[2].level) {
        engine_fault(e,F_CANCEL);
        return;
    }
    if (e->state == RAMP) {
        /* EXIT must remain physically open until a prior qualified departure.
         * Same-vector departure/arrival does not establish ordering. */
        if (e->adc[2] < 3800u) {
            engine_fault(e,F_ORDER);
            return;
        }
        Contact *home = &e->contacts[0];
        if (e->adc[1] <= 200u)
            e->last_home = e->height;
        if (home->count == 1u && !home->candidate) {
            e->candidate_lo = e->last_home;
            e->candidate_hi = e->height;
        }
        if (edge[0] && !home->level) {
            e->release_ms = home->since;
            e->release_lo = e->candidate_lo;
            e->release_hi = e->candidate_hi;
            if (e->release_lo > e->release_hi) {
                int32_t swap = e->release_lo;
                e->release_lo = e->release_hi;
                e->release_hi = swap;
            }
            e->have_release = true;
            e->state = SLIDE;
        }
        return;
    }
    if (e->now - e->release_ms > MAX_SLIDE_MS) {
        engine_fault(e,F_TIMEOUT);
        return;
    }
    if (edge[0] && e->contacts[0].level) {
        engine_fault(e,F_ORDER);
        return;
    }
    if (edge[1] && e->contacts[1].level) {
        e->transit_ms = e->contacts[1].since - e->release_ms;
        e->state = DONE;
    }
}

void engine_sample(Engine *e,uint32_t now,const uint16_t raw[4],bool valid)
{
    uint32_t dt = now - e->last;
    e->now = now;
    e->last = now;
    e->valid = valid && raw != NULL;
    if (e->sampled && (dt < PERIOD_MS || dt > PERIOD_MS + 5u))
        engine_fault(e,F_GAP);
    e->sampled = true;
    if (!e->valid) {
        engine_fault(e,F_ADC);
        return;
    }
    for (unsigned i = 0; i < 4; ++i) {
        e->adc[i] = raw[i];
        if (raw[i] > 4095u) {
            e->valid = false;
            engine_fault(e,F_ADC);
            return;
        }
    }
    if (raw[0] < 80u || raw[0] > 2300u) {
        e->valid = false;
        engine_fault(e,F_RANGE);
        return;
    }
    /* Terminal evidence is immutable; acquisition/fault status remains live. */
    if (e->state == DONE || e->state == INVALID)
        return;
    bool edge[3];
    for (unsigned i = 0; i < 3; ++i)
        edge[i] = qualify(&e->contacts[i],raw[i+1],now);
    uint32_t r = engine_resistance(raw[0]);
    if (e->state >= READY)
        e->height = (int32_t)r - (int32_t)e->baseline;
    bool arm_press = edge[2] && e->contacts[2].level;
    switch (e->state) {
    case WAIT:
        if (arm_press && baseline_inputs(e))
            zero_begin(e,r);
        break;
    case ZERO:
        zero_update(e,r);
        break;
    case READY:
        if (!e->contacts[2].level)
            e->released = true;
        if (arm_press && e->released) {
            if (!baseline_inputs(e) || e->height < -200 || e->height > 200)
                engine_fault(e,F_ORDER);
            else
                ramp_begin(e);
        }
        break;
    case RAMP:
    case SLIDE:
        active_update(e,edge);
        break;
    default:
        break;
    }
}

bool engine_consistent(const Engine *e)
{
    if (!e || e->state > INVALID || e->faults > 1023u)
        return false;
    if ((e->state == INVALID) != (e->faults != 0))
        return false;
    if (e->baseline > 128000u || e->elapsed > MAX_RAMP_MS + 25u)
        return false;
    if (e->have_release && (e->release_lo > e->release_hi ||
        e->release_lo < -200 || e->release_hi > 88000))
        return false;
    if ((e->state == SLIDE || e->state == DONE) && !e->have_release)
        return false;
    if (!e->have_release && (e->release_lo || e->release_hi || e->transit_ms))
        return false;
    if (e->transit_ms > MAX_SLIDE_MS)
        return false;
    for (unsigned i = 0; i < 4; ++i)
        if (e->adc[i] > 4095u)
            return false;
    return true;
}
