/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>
/* Convert the 100k pullup / 1k series network to external resistance.
 * R = 100000*N/(4095-N)-1000. This is nominal, not calibration. */
uint32_t resistance(uint16_t adc)
{
    if (adc >= 4095u)
        return UINT32_MAX;
    uint32_t total = 100000u * adc / (4095u - adc);
    return total > 1000u ? total - 1000u : 0u;
}
static void lane_init(Lane *p)
{
    p->state = BASELINE;
    p->baseline = 0;
    p->sum = 0;
    p->low = UINT32_MAX;
    p->high = 0;
    p->hold_ms = 0;
    p->recovery_ms = 0;
    p->residual = NEVER;
    p->baseline_count = 0;
    p->streak = 0;
    p->candidate = 0;
    p->contact = 0;
    p->milestones = 0;
    p->recovery_marks = 0;
    for (unsigned i = 0; i < 3; ++i) {
        p->loaded[i] = NEVER;
        p->recovered[i] = NEVER;
    }
}
void engine_init(Engine *e)
{
    e->now = 0;
    e->previous = 0;
    e->faults = 0;
    e->initialized = false;
    e->acquired = false;
    for (unsigned i = 0; i < 4; ++i)
        e->adc[i] = 0;
    for (unsigned i = 0; i < 2; ++i) {
        e->ohms[i] = 0;
        lane_init(&e->lane[i]);
    }
}
void engine_fault(Engine *e,uint8_t flag)
{
    e->faults |= flag;
    for (unsigned i = 0; i < 2; ++i)
        e->lane[i].state = INVALID;
}
/* A hysteresis band never counts toward a consecutive qualification. */
static void contact_sample(Lane *p,uint16_t adc)
{
    if (adc > 200u && adc < 3800u) {
        p->streak = 0;
        return;
    }
    uint8_t candidate = adc <= 200u;
    if (!p->streak || candidate != p->candidate) {
        p->candidate = candidate;
        p->streak = 1;
    } else if (p->streak < QUALIFY) {
        ++p->streak;
    }
    if (p->streak == QUALIFY)
        p->contact = p->candidate;
}
static void reset_baseline(Lane *p)
{
    p->sum = 0;
    p->low = UINT32_MAX;
    p->high = 0;
    p->baseline_count = 0;
}
static void collect_baseline(Lane *p,uint32_t value)
{
    if (p->contact || p->streak < QUALIFY) {
        reset_baseline(p);
        return;
    }
    if (value < p->low)
        p->low = value;
    if (value > p->high)
        p->high = value;
    /* 500 ohm peak-to-peak stability gate; restart rather than average drift. */
    if (p->high - p->low > 500u) {
        reset_baseline(p);
        return;
    }
    p->sum += value;
    ++p->baseline_count;
    if (p->baseline_count == BASELINE_SAMPLES) {
        p->baseline = p->sum / BASELINE_SAMPLES;
        p->state = READY;
    }
}
static void milestones(uint32_t elapsed,uint32_t value,uint32_t values[3],uint8_t *mask)
{
    static const uint32_t times[3] = {1000u,10000u,60000u};
    for (unsigned i = 0; i < 3; ++i) {
        if (!(*mask & (1u << i)) && elapsed >= times[i]) {
            values[i] = value;
            *mask |= 1u << i;
        }
    }
}
static void lane_step(Lane *p,uint32_t value,uint32_t dt)
{
    switch (p->state) {
    case BASELINE:
        collect_baseline(p,value);
        break;
    case READY:
        if (p->contact) {
            /* Capture baseline only while unloaded; no automatic retare. */
            p->state = LOADED;
            p->hold_ms = 0;
        }
        break;
    case LOADED:
        p->hold_ms += dt;
        if (p->hold_ms > HOLD_MAX) {
            p->state = INVALID;
            break;
        }
        /* Release at the exact deadline is conservative: record duration,
         * but do not award a loaded milestone from an unloaded vector. */
        if (!p->contact) {
            if (p->hold_ms < HOLD_MIN || p->milestones != 7u) {
                p->state = INVALID;
            } else {
                p->state = RECOVERY;
                p->recovery_ms = 0;
            }
        } else {
            static const uint32_t deadlines[3] = {1000u,10000u,60000u};
            for (unsigned i = 0; i < 3; ++i) {
                if (!(p->milestones & (1u << i)) && p->hold_ms >= deadlines[i]
                    && (!p->candidate || p->streak < QUALIFY)) {
                    p->state = INVALID;
                    return;
                }
            }
            milestones(p->hold_ms,value,p->loaded,&p->milestones);
        }
        break;
    case RECOVERY:
        if (p->contact || p->candidate || p->streak < QUALIFY) {
            p->state = INVALID;
            break;
        }
        p->recovery_ms += dt;
        if (p->recovery_ms > RECOVERY_MS)
            p->recovery_ms = RECOVERY_MS;
        milestones(p->recovery_ms,value,p->recovered,&p->recovery_marks);
        if (p->recovery_ms == RECOVERY_MS) {
            p->residual = value;
            p->state = DONE;
        }
        break;
    case DONE:
    case INVALID:
        break;
    }
}
void engine_sample(Engine *e,uint32_t now,const uint16_t adc[4],bool valid)
{
    uint32_t dt = e->initialized ? now - e->previous : PERIOD_MS;
    e->now = now;
    e->previous = now;
    e->initialized = true;
    e->acquired = false;
    if (!valid || !adc) {
        engine_fault(e,F_ADC);
        return;
    }
    for (unsigned i = 0; i < 4; ++i) {
        if (adc[i] > 4095u) {
            engine_fault(e,F_ADC);
            return;
        }
    }
    for (unsigned i = 0; i < 4; ++i)
        e->adc[i] = adc[i];
    if (adc[0] < 80u || adc[0] > 2300u || adc[1] < 80u || adc[1] > 2300u) {
        engine_fault(e,F_RANGE);
        return;
    }
    e->acquired = true;
    for (unsigned i = 0; i < 2; ++i)
        e->ohms[i] = resistance(adc[i]);
    if (dt < PERIOD_MS || dt > PERIOD_MS + 5u) {
        engine_fault(e,F_GAP);
        return;
    }
    if (e->faults)
        return;
    for (unsigned i = 0; i < 2; ++i) {
        contact_sample(&e->lane[i],adc[i+2]);
        lane_step(&e->lane[i],e->ohms[i],dt);
    }
}
static bool valid_marks(uint32_t elapsed,uint8_t mask,const uint32_t values[3])
{
    static const uint32_t times[3] = {1000u,10000u,60000u};
    if (mask != 0u && mask != 1u && mask != 3u && mask != 7u)
        return false;
    for (unsigned i = 0; i < 3; ++i) {
        if (mask & (1u << i)) {
            if (elapsed < times[i] || values[i] > resistance(2300))
                return false;
        } else if (values[i] != NEVER) {
            return false;
        }
    }
    return true;
}
bool engine_valid(const Engine *e)
{
    if (!e || e->faults > 31u)
        return false;
    for (unsigned i = 0; i < 4; ++i) {
        if (e->adc[i] > 4095u)
            return false;
    }
    for (unsigned i = 0; i < 2; ++i) {
        const Lane *p = &e->lane[i];
        if (p->state > INVALID || p->contact > 1u)
            return false;
        if (e->faults && p->state != INVALID)
            return false;
        if (p->baseline > resistance(2300))
            return false;
        if (p->hold_ms > HOLD_MAX + PERIOD_MS + 5u)
            return false;
        if (p->recovery_ms > RECOVERY_MS)
            return false;
        if (!valid_marks(p->hold_ms,p->milestones,p->loaded))
            return false;
        if (!valid_marks(p->recovery_ms,p->recovery_marks,p->recovered))
            return false;
        if (p->state == DONE && (p->recovery_marks != 7u || p->residual != p->recovered[2]))
            return false;
        if ((p->state == RECOVERY || p->state == DONE)
            && (p->milestones != 7u || p->hold_ms > HOLD_MAX))
            return false;
        if (p->state == READY && p->contact)
            return false;
    }
    return true;
}
