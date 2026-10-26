/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

/* A single coupon per reset prevents retained moisture from being counted as
 * a second trial. All event times are relative to qualified upstream arrival. */
void engine_init(Engine *e)
{
    e->state = BASELINE;
    e->now = 0;
    e->previous = 0;
    e->started = 0;
    e->elapsed = 0;
    e->candidate = 0;
    e->wet = 0;
    e->faults = 0;
    e->baseline = 0;
    e->initialized = false;
    for (unsigned i = 0; i < CHANNELS; ++i) {
        e->first[i] = NEVER;
        e->adc[i] = 4095;
        e->streak[i] = 0;
    }
}

void engine_fault(Engine *e, uint8_t flag)
{
    e->faults |= flag;
    e->state = INVALID;
}

static bool inputs_valid(const uint16_t adc[CHANNELS])
{
    if (!adc)
        return false;
    for (unsigned i = 0; i < CHANNELS; ++i) {
        if (adc[i] > 4095)
            return false;
    }
    return true;
}

/* Hysteresis alone cannot establish evidence. A channel must remain beyond
 * the applicable threshold for five complete acquisition vectors. Mid-band
 * samples retain the accepted level, but cancel any pending transition. */
static void qualify(Engine *e, const uint16_t adc[CHANNELS])
{
    for (unsigned i = 0; i < CHANNELS; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        bool target;
        e->adc[i] = adc[i];
        if (adc[i] <= WET_MAX) {
            target = true;
        } else if (adc[i] >= DRY_MIN) {
            target = false;
        } else {
            e->streak[i] = 0;
            continue;
        }
        bool accepted = (e->wet & bit) != 0;
        if (accepted == target) {
            e->streak[i] = 0;
            continue;
        }
        if (((e->candidate & bit) != 0) != target) {
            if (target)
                e->candidate |= bit;
            else
                e->candidate &= (uint8_t)~bit;
            e->streak[i] = 1;
        } else if (e->streak[i] < QUALIFY) {
            ++e->streak[i];
        }
        if (e->streak[i] >= QUALIFY) {
            if (target)
                e->wet |= bit;
            else
                e->wet &= (uint8_t)~bit;
            e->streak[i] = 0;
        }
    }
}

static bool all_dry(const uint16_t adc[CHANNELS])
{
    for (unsigned i = 0; i < CHANNELS; ++i) {
        if (adc[i] < DRY_MIN)
            return false;
    }
    return true;
}

static void record_arrivals(Engine *e)
{
    for (unsigned i = 0; i < CHANNELS; ++i) {
        if ((e->wet & (1u << i)) && e->first[i] == NEVER)
            e->first[i] = e->elapsed;
    }
}

void engine_sample(Engine *e, uint32_t now,
                   const uint16_t adc[CHANNELS], bool valid)
{
    e->now = now;
    if (!valid || !inputs_valid(adc)) {
        engine_fault(e, F_ADC);
        return;
    }
    if (e->initialized && (uint32_t)(now - e->previous) > 25u)
        engine_fault(e, F_GAP);
    e->previous = now;
    e->initialized = true;
    qualify(e, adc);
    if (e->state == INVALID)
        return;
    if (e->state == BASELINE) {
        if (!all_dry(adc)) {
            e->baseline = 0;
            return;
        }
        if (e->baseline < BASELINE_SAMPLES)
            ++e->baseline;
        if (e->baseline == BASELINE_SAMPLES)
            e->state = READY;
        return;
    }
    if (e->state == READY) {
        /* Simultaneous upstream/downstream does not prove ordered transit. */
        if (e->wet & 14u) {
            engine_fault(e, (e->wet & 12u) ? F_BYPASS : F_ORDER);
            return;
        }
        if (e->wet & 1u) {
            e->started = now;
            e->first[0] = 0;
            e->state = ACTIVE;
        }
        return;
    }
    if (e->state == EXPIRED)
        return;
    /* COMPLETE remains provisional: later edge wetting revokes the result.
     * Do not invent an arrival timestamp against the frozen trial clock. */
    if (e->state == COMPLETE) {
        if (e->wet & 12u)
            engine_fault(e, F_BYPASS);
        return;
    }
    if (e->state == ACTIVE) {
        uint32_t dt = now - e->started;
        e->elapsed = dt > TRIAL_MS ? TRIAL_MS : dt;
    }
    record_arrivals(e);
    if (e->wet & 12u) {
        engine_fault(e, F_BYPASS);
        return;
    }

    if (!(e->wet & 1u)) {
        engine_fault(e, F_DRY);
        return;
    }
    /* The inclusive timeout has priority over a coincident downstream edge. */
    if (e->elapsed >= TRIAL_MS) {
        e->state = EXPIRED;
        return;
    }
    if (e->wet & 2u)
        e->state = COMPLETE;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->state > INVALID || e->wet > 15u || e->faults > 127u)
        return false;
    if (e->elapsed > TRIAL_MS)
        return false;
    if ((e->state == INVALID) != (e->faults != 0))
        return false;
    for (unsigned i = 0; i < CHANNELS; ++i) {
        if (e->adc[i] > 4095)
            return false;
        if (e->first[i] != NEVER && e->first[i] > e->elapsed)
            return false;
    }
    if (e->first[0] != NEVER && e->first[0] != 0)
        return false;
    if (e->state == BASELINE || e->state == READY) {
        if (e->elapsed != 0)
            return false;
        for (unsigned i = 0; i < CHANNELS; ++i) {
            if (e->first[i] != NEVER)
                return false;
        }
    }
    if (e->state == ACTIVE || e->state == COMPLETE || e->state == EXPIRED) {
        if (e->first[0] != 0)
            return false;
        if (e->first[2] != NEVER || e->first[3] != NEVER)
            return false;
    }
    if (e->state == ACTIVE && (e->wet != 1u || e->first[1] != NEVER))
        return false;
    if (e->state == COMPLETE) {
        if (e->wet & 12u)
            return false;
        if (e->first[1] == NEVER || e->first[1] != e->elapsed)
            return false;
        if (!e->elapsed || e->elapsed >= TRIAL_MS)
            return false;
    }
    if (e->state == EXPIRED && e->elapsed != TRIAL_MS)
        return false;
    return true;
}
