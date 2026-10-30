/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>
/* Totals form two independent conservation ledgers: sessions and cups. */
bool engine_valid(const Engine *e)
{
    if (e->state > HALT || e->depth > CHANNELS || e->mask > 15u || e->known > 15u)
        return false;
    if ((e->state == ACTIVE) != (e->depth != 0u))
        return false;
    if ((e->state == HALT) != (e->fault != 0u) || e->fault > 15u)
        return false;
    if ((uint64_t)e->completed + e->aborted + (e->depth != 0u) != e->started)
        return false;
    if ((uint64_t)e->pops + e->abandoned + e->depth != e->pushes)
        return false;
    if (e->completed > e->pops || e->started > e->pushes)
        return false;
    if (e->order_errors > e->aborted)
        return false;
    uint32_t bits = 0;
    for (unsigned i = 0; i < CHANNELS; ++i) {
        if (i < e->depth) {
            if (e->stack[i] >= CHANNELS || (bits & (1u << e->stack[i])))
                return false;
            bits |= 1u << e->stack[i];
        } else if (e->stack[i] != 0u) {
            return false;
        }
    }
    if (e->state == ACTIVE && (e->known != 15u || bits != e->mask))
        return false;
    if (e->state == READY && (e->known != 15u || e->mask != 0u))
        return false;
    return true;
}
static bool increment(uint32_t *value)
{
    if (*value == UINT32_MAX)
        return false;
    ++*value;
    return true;
}
static void abandon(Engine *e)
{
    /* Active depth was already included in pushes, so these cannot overflow
     * for any reachable state satisfying the conservation identities. */
    if (e->depth) {
        ++e->aborted;
        e->abandoned += e->depth;
    }
    e->depth = 0;
    for (unsigned i = 0; i < CHANNELS; ++i)
        e->stack[i] = 0;
    e->state = WAIT;
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
static void forget(Engine *e)
{
    e->known = 0;
    for (unsigned i = 0; i < CHANNELS; ++i)
        e->contact[i] = (Contact){0};
}
static void qualify(Engine *e, uint8_t raw)
{
    e->known = 0;
    e->mask = 0;
    for (unsigned i = 0; i < CHANNELS; ++i) {
        Contact *c = &e->contact[i];
        uint8_t bit = (raw >> i) & 1u;
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
static unsigned channel(uint32_t bit)
{
    unsigned result = 0;
    while ((bit >>= 1u) != 0u)
        ++result;
    return result;
}
static bool update(Engine *e, uint32_t previous)
{
    if (e->state == WAIT) {
        if (e->known == 15u && e->mask == 0u)
            e->state = READY;
        return true;
    }
    uint32_t changed = previous ^ e->mask;
    if (!changed)
        return true;
    /* A single qualified sample cannot order two physical movements. */
    if (changed & (changed - 1u)) {
        if (!increment(&e->collisions))
            return false;
        abandon(e);
        return true;
    }
    unsigned cup = channel(changed);
    if (e->mask & changed) {
        if (!increment(&e->pushes))
            return false;
        if (!e->depth && !increment(&e->started))
            return false;
        if (e->depth == CHANNELS)
            return false;
        e->stack[e->depth++] = cup;
        e->state = ACTIVE;
        return true;
    }
    if (!e->depth || e->stack[e->depth - 1u] != cup) {
        if (!increment(&e->order_errors))
            return false;
        abandon(e);
        return true;
    }
    if (!increment(&e->pops))
        return false;
    e->stack[--e->depth] = 0;
    if (!e->depth) {
        if (!increment(&e->completed))
            return false;
        e->state = READY;
    }
    return true;
}
void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (raw > 15u) {
        engine_fault(e, F_INPUT);
        return;
    }
    if (e->initialized && now == e->now)
        return;
    uint32_t elapsed = now - e->now;
    bool gap = e->initialized && elapsed != 1u;
    e->initialized = true;
    e->now = now;
    uint32_t previous = e->mask;
    if (gap) {
        if (!increment(&e->gaps))
            engine_fault(e, F_OVERFLOW);
        if (e->state != HALT)
            abandon(e);
        forget(e);
    }
    qualify(e, raw);
    if (e->state == HALT || e->known != 15u)
        return;
    /* Speculative accounting is atomic even when a later counter exhausts.
     * On overflow abandon the PREVIOUS accepted stack, not half an update. */
    Engine next = *e;
    if (!update(&next, previous)) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    *e = next;
}
