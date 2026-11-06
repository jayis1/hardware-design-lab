/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

void engine_fault(Engine *e, uint32_t fault)
{
    /* A halt freezes evidence. Each accepted start has exactly one fate. */
    if (e->state == HALT)
        return;
    if (e->state == ACTIVE)
        ++e->aborted;
    e->fault = fault;
    e->state = HALT;
    e->baseline = 0;
}

void engine_tx_error(Engine *e)
{
    /* Transport diagnostics do not change retained acquisition evidence. */
    if (e->tx_errors < LIMIT)
        ++e->tx_errors;
}

static void qualify(Engine *e, uint8_t raw)
{
    /* Independently qualify contacts: chatter on a pallet must not mask a
     * sustained index or RUN edge. A gap halts before this routine runs. */
    e->raw = raw;
    for (unsigned i = 0; i < 4; ++i) {
        unsigned bit = 1u << i;
        uint8_t value = (raw & bit) != 0;
        if (!e->stable[i] || value != e->candidate[i]) {
            e->candidate[i] = value;
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY) {
            e->valid |= bit;
            e->qualified = (e->qualified & ~bit) | (value ? bit : 0);
        }
    }
}

static void start(Engine *e)
{
    ++e->starts;
    e->state = ACTIVE;
    e->age = 0;
    e->baseline = 0;
    e->pulses = 0;
    e->a = 0;
    e->b = 0;
    e->neither = 0;
    e->both = 0;
    e->uncertain = 0;
    e->aa = 0;
    e->ab = 0;
    e->ba = 0;
    e->bb = 0;
    e->anchor = 0;
}

static void arrival(Engine *e, unsigned previous)
{
    ++e->pulses;
    unsigned pallet = e->qualified & 3u;
    /* A contact changing at the event endpoint cannot establish ordering.
     * Even a subqualification raw glitch defeats attribution for that event. */
    if ((previous & 3u) != pallet || (e->raw & 3u) != pallet) {
        ++e->uncertain;
        e->anchor = 0;
        return;
    }
    if (!pallet) {
        ++e->neither;
        e->anchor = 0;
        return;
    }
    if (pallet == 3) {
        ++e->both;
        e->anchor = 0;
        return;
    }
    if (pallet == 1)
        ++e->a;
    else
        ++e->b;
    if (e->anchor == 1) {
        if (pallet == 1)
            ++e->aa;
        else
            ++e->ab;
    } else if (e->anchor == 2) {
        if (pallet == 1)
            ++e->ba;
        else
            ++e->bb;
    }
    e->anchor = pallet;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->state == HALT) {
        e->now = now;
        return;
    }
    if (raw > 15) {
        engine_fault(e, F_CONFIG);
        return;
    }
    uint32_t delta = now - e->now;
    e->now = now;
    if (e->seen && delta != 1) {
        e->gaps = 1;
        engine_fault(e, F_GAP);
        return;
    }
    e->seen = true;
    if (e->sequence == UINT32_MAX) {
        engine_fault(e, F_COUNTER);
        return;
    }
    ++e->sequence;
    unsigned previous = e->qualified;
    unsigned previous_raw = e->raw;
    unsigned previous_valid = e->valid;
    qualify(e, raw);
    if (e->valid != 15)
        return;
    unsigned q = e->qualified;
    bool index_rise = (q & 4) && !(previous & 4);
    if (e->state == ACTIVE) {
        ++e->age;
        /* Rejecting deadline has precedence over simultaneous RUN closure,
         * release, index and any apparent successful completion. */
        if (e->age >= DEADLINE) {
            engine_fault(e, F_DEADLINE);
            return;
        }
        if (!(q & 8)) {
            if ((q | raw) & 4) {
                engine_fault(e, F_BOUNDARY);
                return;
            }
            ++e->completed;
            e->state = WAIT;
            return;
        }
        if (index_rise) {
            if (!(raw & 8)) {
                engine_fault(e, F_BOUNDARY);
                return;
            }
            /* At most 6000 events fit within the bounded acquisition run.
             * Still guard explicitly so corrupted state cannot wrap. */
            if (e->pulses >= LIMIT) {
                engine_fault(e, F_COUNTER);
                return;
            }
            arrival(e, previous);
        }
        return;
    }
    if (e->state == READY) {
        if ((q & 8) && !(previous & 8)) {
            if (e->starts == LIMIT) {
                engine_fault(e, F_COUNTER);
                return;
            }
            start(e);
            if ((q | raw) & 4)
                engine_fault(e, F_BOUNDARY);
            return;
        }
        /* Any index activity invalidates the idle arming baseline. */
        if ((q | raw) & 4) {
            e->state = WAIT;
            e->baseline = 0;
        }
        return;
    }
    /* No held-RUN startup. Both index and RUN must be raw/qualified low
     * across fifty intervals after qualification, not just fifty samples. */
    if (previous_valid == 15 && ((q | raw | previous | previous_raw) & 12u) == 0) {
        if (e->baseline < BASELINE)
            ++e->baseline;
        if (e->baseline == BASELINE)
            e->state = READY;
    } else {
        e->baseline = 0;
    }
}

bool engine_valid(const Engine *e)
{
    if (!e || e->state > HALT || e->fault > F_COUNTER)
        return false;
    if ((e->state == HALT) != (e->fault != 0))
        return false;
    if (e->fault && (e->fault & (e->fault - 1)))
        return false;
    if (e->raw > 15 || e->qualified > 15 || e->valid > 15)
        return false;
    if (e->age > DEADLINE || e->baseline > BASELINE || e->anchor > 2)
        return false;
    if (e->starts > LIMIT || e->completed > LIMIT || e->aborted > LIMIT)
        return false;
    if (e->starts != e->completed + e->aborted + (e->state == ACTIVE))
        return false;
    if (e->gaps > 1 || e->tx_errors > LIMIT)
        return false;
    if (e->gaps && e->fault != F_GAP)
        return false;
    uint64_t counts = (uint64_t)e->a + e->b + e->neither + e->both + e->uncertain;
    if (e->pulses != counts || e->pulses > LIMIT)
        return false;
    uint64_t pairs = (uint64_t)e->aa + e->ab + e->ba + e->bb;
    if (pairs > (uint64_t)e->a + e->b)
        return false;
    if (e->pulses && pairs >= e->pulses)
        return false;
    if ((uint64_t)e->aa + e->ba > e->a || (uint64_t)e->ab + e->bb > e->b)
        return false;
    if ((uint64_t)e->aa + e->ab + (e->anchor == 1) > e->a ||
        (uint64_t)e->ba + e->bb + (e->anchor == 2) > e->b)
        return false;
    if (e->anchor == 1 && !e->a)
        return false;
    if (e->anchor == 2 && !e->b)
        return false;
    if (!e->starts && (e->age || e->pulses || e->anchor))
        return false;
    if ((e->state == WAIT || e->state == READY) && e->starts && e->age < QUALIFY)
        return false;
    /* A rising edge needs five high samples and ten samples between rises. */
    if (e->pulses > (e->age + 5u) / 10u)
        return false;
    if (e->state == ACTIVE && (e->valid != 15 || !(e->qualified & 8) || e->age >= DEADLINE))
        return false;
    if (e->state == READY && (e->valid != 15 || (e->qualified & 12) || e->baseline != BASELINE))
        return false;
    if (e->fault == F_DEADLINE && e->age != DEADLINE)
        return false;
    return true;
}
