/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

static uint32_t bounded_add(uint32_t value, uint32_t delta, uint32_t limit)
{
    return delta > limit - value ? limit : value + delta;
}

static bool increment(uint32_t *value)
{
    if (*value >= COUNT_LIMIT)
        return false;
    ++*value;
    return true;
}

static bool multiple(uint32_t value)
{
    return value && (value & (value - 1u));
}

static unsigned zone(uint32_t mask)
{
    for (unsigned i = 0; i < 4; ++i) {
        if (mask & (1u << i))
            return i;
    }
    return NO_ZONE;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->last = NO_ZONE;
    e->pending = NO_ZONE;
    e->axis = NO_AXIS;
}

void engine_fault(Engine *e, uint32_t fault)
{
    /* Freeze the terminal snapshot, including provisional evidence. */
    if (e->state == HALT)
        return;
    e->fault |= fault;
    e->state = HALT;
}

void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx_errors))
        engine_fault(e, F_COUNTER);
}

static void qualify(Engine *e, uint32_t now, uint8_t raw)
{
    /* Independent qualifiers prevent unrelated chatter hiding occupancy. */
    for (unsigned i = 0; i < 4; ++i) {
        Contact *c = &e->contact[i];
        uint8_t bit = (raw >> i) & 1u;
        if (!e->initialized || c->candidate != bit) {
            c->candidate = bit;
            c->since = now;
        } else if (now - c->since >= QUALIFY_MS) {
            c->stable = bit;
            c->valid = 1;
        }
        if (c->valid)
            e->qualified |= 1u << i;
        if (c->stable)
            e->mask |= 1u << i;
        else
            e->mask &= ~(1u << i);
    }
}

static bool commit_visit(Engine *e)
{
    /* Caller stages this operation. Capacity failure rolls back every bin. */
    unsigned current = e->pending;
    if (!increment(&e->visits))
        return false;
    if (e->last < NO_ZONE) {
        unsigned old = e->last;
        if (!increment(&e->matrix[4u * old + current]))
            return false;
        if (old == current) {
            if (!increment(&e->returns))
                return false;
            e->run = 0;
            e->axis = NO_AXIS;
        } else if (old / 2u == current / 2u) {
            unsigned axis = current / 2u;
            uint32_t *count = axis ? &e->y : &e->x;
            uint32_t *peak = axis ? &e->peak_y : &e->peak_x;
            if (!increment(count))
                return false;
            if (e->axis != axis)
                e->run = 0;
            e->axis = axis;
            if (!increment(&e->run))
                return false;
            if (e->run > *peak)
                *peak = e->run;
        } else {
            if (!increment(&e->turns))
                return false;
            e->run = 0;
            e->axis = NO_AXIS;
        }
    }
    e->last = current;
    e->pending = NO_ZONE;
    e->state = ARMED;
    e->clear_ms = CLEAR_MS;
    e->elapsed = 0;
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t dt = now - e->now;
    uint32_t old_raw = e->raw;
    uint32_t old_mask = e->mask;
    uint32_t old_qualified = e->qualified;
    e->now = now;
    e->raw = raw & 15u;
    if (e->state == HALT)
        return;
    if (e->initialized && dt > MAX_GAP_MS) {
        if (!increment(&e->gaps))
            engine_fault(e, F_COUNTER);
        else
            engine_fault(e, F_GAP);
        return;
    }
    if (e->initialized && !dt)
        return;
    qualify(e, now, raw);
    e->initialized = 1;
    /* A raw overlap cannot be laundered by different qualification times. */
    if (multiple(e->raw) || multiple(e->mask)) {
        engine_fault(e, F_AMBIGUOUS);
        return;
    }
    if (e->state == CAPTURE) {
        e->elapsed = bounded_add(e->elapsed, dt, TIMEOUT_MS);
        if (e->elapsed >= TIMEOUT_MS) {
            engine_fault(e, F_TIMEOUT);
            return;
        }
        if ((e->raw | e->mask) & ~(1u << e->pending)) {
            engine_fault(e, F_AMBIGUOUS);
            return;
        }
    }
    if (e->qualified != 15u)
        return;
    bool clear = !e->raw && !e->mask;
    bool previously_clear = !old_raw && !old_mask && old_qualified == 15u;
    if (clear && previously_clear)
        e->clear_ms = bounded_add(e->clear_ms, dt, CLEAR_MS);
    else
        e->clear_ms = 0;
    if (e->state == WAIT) {
        if (e->clear_ms >= CLEAR_MS)
            e->state = ARMED;
        return;
    }
    if (e->state == ARMED) {
        if (e->mask && e->mask == e->raw) {
            e->pending = zone(e->mask);
            e->state = CAPTURE;
            e->elapsed = 0;
            e->clear_ms = 0;
        }
        return;
    }
    if (e->clear_ms >= CLEAR_MS) {
        Engine next = *e;
        if (!commit_visit(&next))
            engine_fault(e, F_COUNTER);
        else
            *e = next;
    }
}

bool engine_valid(const Engine *e)
{
    if (!e || e->state > HALT || e->fault > 63u)
        return false;
    if (e->raw > 15u || e->mask > 15u || e->qualified > 15u)
        return false;
    if ((e->state == HALT) != (e->fault != 0))
        return false;
    if (e->last > NO_ZONE || e->pending > NO_ZONE || e->axis > NO_AXIS)
        return false;
    if (e->clear_ms > CLEAR_MS || e->elapsed > TIMEOUT_MS)
        return false;
    if (e->visits > COUNT_LIMIT || e->gaps > COUNT_LIMIT || e->tx_errors > COUNT_LIMIT)
        return false;
    if ((e->last < NO_ZONE) != (e->visits != 0))
        return false;
    if (e->state == CAPTURE && (e->pending == NO_ZONE || e->qualified != 15u))
        return false;
    if ((e->state == WAIT || e->state == ARMED) && e->pending != NO_ZONE)
        return false;
    if (e->state == ARMED && e->qualified != 15u)
        return false;
    uint64_t x = 0, y = 0, returns = 0, turns = 0;
    for (unsigned a = 0; a < 4; ++a) {
        for (unsigned b = 0; b < 4; ++b) {
            uint32_t value = e->matrix[4u * a + b];
            if (value > COUNT_LIMIT)
                return false;
            if (a == b)
                returns += value;
            else if (a / 2u != b / 2u)
                turns += value;
            else if (a < 2)
                x += value;
            else
                y += value;
        }
    }
    if (x != e->x || y != e->y || returns != e->returns || turns != e->turns)
        return false;
    if (x + y + returns + turns + (e->visits != 0) != e->visits)
        return false;
    if (e->peak_x > e->x || e->peak_y > e->y)
        return false;
    if ((e->peak_x != 0) != (e->x != 0) || (e->peak_y != 0) != (e->y != 0))
        return false;
    if ((e->axis == NO_AXIS) != (e->run == 0))
        return false;
    if (e->axis == 0 && (e->run > e->peak_x || e->last > 1u))
        return false;
    if (e->axis == 1 && (e->run > e->peak_y || e->last < 2u || e->last > 3u))
        return false;
    return true;
}
