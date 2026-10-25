/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

static uint32_t add(uint32_t value, uint32_t increment)
{
    if (UINT32_MAX - value < increment) {
        return UINT32_MAX;
    }
    return value + increment;
}

static void increment(uint32_t *value)
{
    *value = add(*value, 1);
}

static void duration(Basket *b, uint32_t *value, uint32_t elapsed)
{
    if (UINT32_MAX - *value <= elapsed) {
        b->flags |= SATURATED;
    }
    *value = add(*value, elapsed);
}

static void clear_basket(Basket *b)
{
    b->total = 0;
    b->epoch_ms = 0;
    b->streak = 0;
    b->longest = 0;
    b->entries = 0;
    b->baths = 0;
    b->last_epoch = 0;
    b->flags = 0;
}

static void reset_contact(Contact *c, bool raw)
{
    /* Invalidate continuity, but never treat this one sample as an edge. */
    c->candidate = raw;
    c->age = 0;
    c->valid = false;
}

void engine_init(Engine *e, uint32_t now)
{
    if (!e) {
        return;
    }
    e->now = now;
    e->epoch = 0;
    e->rejected = 0;
    e->gaps = 0;
    e->losses = 0;
    e->ready = false;
    e->levels = 0;
    for (unsigned i = 0; i < 4; ++i) {
        e->contact[i].level = false;
        reset_contact(&e->contact[i], false);
    }
    for (unsigned i = 0; i < BASKETS; ++i) {
        clear_basket(&e->basket[i]);
    }
}

static void qualify(Contact *c, bool raw, uint32_t elapsed)
{
    if (raw != c->candidate) {
        c->candidate = raw;
        c->age = 0;
    } else {
        c->age = add(c->age, elapsed);
    }
    if (c->age >= DEBOUNCE_MS) {
        c->level = c->candidate;
        c->valid = true;
    }
}

static bool all_valid(const Engine *e)
{
    for (unsigned i = 0; i < 4; ++i) {
        if (!e->contact[i].valid) {
            return false;
        }
    }
    return true;
}

static uint8_t qualified_levels(const Engine *e)
{
    uint8_t result = 0;
    for (unsigned i = 0; i < 4; ++i) {
        if (e->contact[i].level) {
            result |= (uint8_t)(1u << i);
        }
    }
    return result;
}

static void break_continuity(Engine *e, uint8_t raw)
{
    increment(&e->gaps);
    e->ready = false;
    for (unsigned i = 0; i < 4; ++i) {
        reset_contact(&e->contact[i], (raw & (1u << i)) != 0);
    }
    for (unsigned i = 0; i < BASKETS; ++i) {
        e->basket[i].flags |= UNCERTAIN;
        e->basket[i].streak = 0;
    }
}

static void accrue(Engine *e, uint32_t elapsed)
{
    if (!e->ready || !e->epoch) {
        return;
    }
    for (unsigned i = 0; i < BASKETS; ++i) {
        Basket *b = &e->basket[i];
        if (!(e->levels & (1u << i))) {
            continue;
        }
        duration(b, &b->total, elapsed);
        duration(b, &b->epoch_ms, elapsed);
        duration(b, &b->streak, elapsed);
        if (b->streak > b->longest) {
            b->longest = b->streak;
        }
    }
}

static void new_bath(Engine *e)
{
    /* Exhausted identities cannot silently alias an earlier bath. */
    if (e->epoch == UINT32_MAX) {
        increment(&e->rejected);
        e->ready = false;
        for (unsigned i = 0; i < BASKETS; ++i) {
            e->basket[i].flags |= SATURATED;
        }
        return;
    }
    increment(&e->epoch);
    for (unsigned i = 0; i < BASKETS; ++i) {
        e->basket[i].epoch_ms = 0;
    }
}

static void transitions(Engine *e, uint8_t previous)
{
    for (unsigned i = 0; i < BASKETS; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        Basket *b = &e->basket[i];
        if (!(e->levels & bit)) {
            b->streak = 0;
            continue;
        }
        if (previous & bit) {
            continue;
        }
        if (b->entries == UINT32_MAX) {
            b->flags |= SATURATED;
        }
        increment(&b->entries);
        if (b->last_epoch != e->epoch) {
            if (b->baths == UINT32_MAX) {
                b->flags |= SATURATED;
            }
            increment(&b->baths);
            b->last_epoch = e->epoch;
        }
    }
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (!e) {
        return;
    }
    uint32_t elapsed = now - e->now;
    if (!elapsed) {
        return;
    }
    e->now = now;
    raw &= 15u;
    if (elapsed > MAX_GAP_MS) {
        break_continuity(e, raw);
        return;
    }
    /* Attribute the elapsed interval to the prior qualified state. */
    accrue(e, elapsed);
    uint8_t previous = e->levels;
    for (unsigned i = 0; i < 4; ++i) {
        qualify(&e->contact[i], (raw & (1u << i)) != 0, elapsed);
    }
    e->levels = qualified_levels(e);
    if (!all_valid(e)) {
        return;
    }
    if (!e->ready) {
        if (!e->levels && e->epoch != UINT32_MAX) {
            e->ready = true;
        }
        return;
    }
    bool marker = (e->levels & 8u) && !(previous & 8u);
    if (marker) {
        /* Simultaneous removal/marking does not prove water was replaced. */
        if ((previous | e->levels) & 7u) {
            increment(&e->rejected);
        } else {
            new_bath(e);
        }
    }
    if (!e->epoch) {
        /* A basket inserted before the first accepted marker is untracked. */
        for (unsigned i = 0; i < BASKETS; ++i) {
            if (e->levels & (1u << i)) {
                e->basket[i].flags |= UNCERTAIN;
            }
        }
        return;
    }
    transitions(e, previous);
}

void engine_transport_loss(Engine *e)
{
    if (e) {
        increment(&e->losses);
    }
}
