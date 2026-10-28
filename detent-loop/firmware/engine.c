/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

uint32_t engine_ohms(uint16_t counts)
{
    if (counts >= 4095u)
        return UINT32_MAX;
    uint32_t r = 100000u * counts / (4095u - counts);
    return r > 1000u ? r - 1000u : 0u;
}

static int32_t magnitude(int32_t value)
{
    /* Inputs are differences of bounded nominal positions, never INT32_MIN. */
    return value < 0 ? -value : value;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->state = WAIT;
}

void engine_fault(Engine *e, uint32_t fault)
{
    e->faults |= fault;
    e->state = INVALID;
    e->acquired = false;
}

static void contact(Contact *c, uint16_t raw)
{
    if (raw > 200u && raw < 3800u) {
        c->count = 0;
        c->known = false;
        return;
    }
    bool value = raw <= 200u;
    if (!c->count || value != c->candidate) {
        c->candidate = value;
        c->count = 1;
    } else if (c->count < 3u) {
        ++c->count;
    }
    if (c->count == 3u) {
        c->value = value;
        c->known = true;
    }
}

static void zero_reset(Engine *e)
{
    e->zero_count = 0;
    for (unsigned i = 0; i < 2; ++i) {
        e->zero_sum[i] = 0;
        e->zero_low[i] = UINT32_MAX;
        e->zero_high[i] = 0;
    }
}

static void zero_sample(Engine *e)
{
    uint32_t resistance[2];
    for (unsigned i = 0; i < 2; ++i) {
        resistance[i] = engine_ohms(e->adc[i]);
        if (resistance[i] < e->zero_low[i])
            e->zero_low[i] = resistance[i];
        if (resistance[i] > e->zero_high[i])
            e->zero_high[i] = resistance[i];
    }
    if (e->zero_high[0] - e->zero_low[0] > 200u ||
        e->zero_high[1] - e->zero_low[1] > 200u) {
        zero_reset(e);
        return;
    }
    for (unsigned i = 0; i < 2; ++i)
        e->zero_sum[i] += resistance[i];
    ++e->zero_count;
    if (e->zero_count == ZERO_SAMPLES) {
        for (unsigned i = 0; i < 2; ++i)
            e->base[i] = e->zero_sum[i] / ZERO_SAMPLES;
        e->released = false;
        e->state = READY;
    }
}

static bool position(Engine *e)
{
    int32_t r0 = (int32_t)engine_ohms(e->adc[0]) - (int32_t)e->base[0];
    int32_t r1 = (int32_t)engine_ohms(e->adc[1]) - (int32_t)e->base[1];
    e->x_um = r0 * 1000 / OHMS_PER_MM;
    e->force_mn = (int32_t)((int64_t)r1 * SPRING_MN_PER_MM / OHMS_PER_MM);
    if (e->x_um < -500 || e->x_um > 20000 || magnitude(e->force_mn) > 20000) {
        engine_fault(e, F_RANGE);
        return false;
    }
    return true;
}

static bool integrate(Engine *e, uint32_t dt)
{
    e->elapsed += dt;
    if (e->elapsed > 120000u) {
        engine_fault(e, F_TIMEOUT);
        return false;
    }
    int32_t dx = e->x_um - e->last_x;
    if (magnitude(dx) > 2000) {
        engine_fault(e, F_SPEED);
        return false;
    }
    if (e->state == OUT) {
        if (e->x_um > e->extreme)
            e->extreme = e->x_um;
        if (e->extreme - e->x_um > 500) {
            engine_fault(e, F_DIRECTION);
            return false;
        }
    } else {
        if (e->x_um < e->extreme)
            e->extreme = e->x_um;
        if (e->x_um - e->extreme > 500) {
            engine_fault(e, F_DIRECTION);
            return false;
        }
    }
    /* Trapezoidal signed line integral. mN * um = nJ. Keep twice nJ
     * to avoid per-sample rounding; divide only in the wire encoder.
     * 120 s / 20 ms * 40000 mN * 2000 um fits int64 comfortably. */
    int64_t piece = (int64_t)(e->force_mn + e->last_force) * dx;
    if (e->state == OUT) {
        e->out_nj2 += piece;
        if (e->force_mn > e->peak_out_mn)
            e->peak_out_mn = e->force_mn;
        if (e->x_um > e->span_um)
            e->span_um = e->x_um;
    } else {
        e->back_nj2 += piece;
        if (e->force_mn < e->peak_back_mn)
            e->peak_back_mn = e->force_mn;
    }
    e->last_x = e->x_um;
    e->last_force = e->force_mn;
    return true;
}

void engine_sample(Engine *e, uint32_t now, const uint16_t adc[4], bool valid)
{
    uint32_t dt = now - e->now;
    e->now = now;
    e->acquired = false;
    if (!valid || !adc) {
        engine_fault(e, F_ADC);
        return;
    }
    for (unsigned i = 0; i < 4; ++i) {
        if (adc[i] > 4095u) {
            engine_fault(e, F_RANGE);
            return;
        }
        e->adc[i] = adc[i];
    }
    if (e->started && (dt < PERIOD_MS || dt > PERIOD_MS + 5u)) {
        engine_fault(e, F_GAP);
        return;
    }
    e->started = true;
    if (adc[0] < 80u || adc[0] > 2300u || adc[1] < 80u || adc[1] > 2300u) {
        engine_fault(e, F_RANGE);
        return;
    }
    /* Terminal records freeze experiment values but still expose ADC health. */
    e->acquired = true;
    if (e->state == DONE || e->state == INVALID)
        return;
    bool old_arm = e->arm.known && e->arm.value;
    bool old_turn = e->turn.known && e->turn.value;
    contact(&e->arm, adc[2]);
    contact(&e->turn, adc[3]);
    bool press_arm = e->arm.known && e->arm.value && !old_arm;
    bool press_turn = e->turn.known && e->turn.value && !old_turn;
    if (e->state == WAIT) {
        if (press_arm && e->turn.known && !e->turn.value) {
            zero_reset(e);
            e->state = ZERO;
        }
        return;
    }
    if (e->state == ZERO) {
        /* Raw release must interrupt zeroing immediately, not after debounce. */
        if (adc[2] > 200u || adc[3] < 3800u) {
            e->state = WAIT;
            zero_reset(e);
            return;
        }
        zero_sample(e);
        return;
    }
    if (!position(e))
        return;
    if (e->state == READY) {
        if (e->arm.known && !e->arm.value)
            e->released = true;
        if (press_arm && e->released) {
            if (magnitude(e->x_um) > 500 || magnitude(e->force_mn) > 2000 ||
                adc[3] < 3800u) {
                engine_fault(e, F_SEQUENCE);
                return;
            }
            e->state = OUT;
            e->last_x = e->x_um;
            e->last_force = e->force_mn;
            e->peak_out_mn = e->force_mn > 0 ? e->force_mn : 0;
            e->extreme = e->x_um;
        }
        return;
    }
    if (!integrate(e, dt))
        return;
    if (e->state == OUT && press_turn) {
        if (e->x_um < 5000 || adc[2] < 3800u) {
            engine_fault(e, F_SEQUENCE);
            return;
        }
        e->state = BACK;
        e->extreme = e->x_um;
        e->peak_back_mn = e->force_mn < 0 ? e->force_mn : 0;
    } else if (e->state == BACK && press_arm) {
        if (magnitude(e->x_um) > 500 || magnitude(e->force_mn) > 2000 || adc[3] < 3800u) {
            engine_fault(e, F_SEQUENCE);
            return;
        }
        e->state = DONE;
    } else if ((e->state == OUT && press_arm) || (e->state == BACK && press_turn)) {
        engine_fault(e, F_SEQUENCE);
    }
}
