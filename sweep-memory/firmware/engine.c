/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

static bool active(const Engine *e)
{
    return e->state == OUTWARD || e->state == RETURNING;
}

void engine_init(Engine *e)
{
    /* Explicit initialization also works in a freestanding build. */
    *e = (Engine){0};
}

int engine_direction(unsigned old_phase, unsigned new_phase)
{
    /* 2 marks an impossible two-bit jump; it is never silently ignored. */
    static const int8_t delta[16] = {
        0, 1,-1, 2,
       -1, 0, 2, 1,
        1, 2, 0,-1,
        2,-1, 1, 0
    };
    if (old_phase > 3 || new_phase > 3)
        return 2;
    return delta[old_phase * 4 + new_phase];
}

static void reject(Engine *e, unsigned reason)
{
    if (active(e))
        ++e->abandoned; /* one-shot: starts is at most one */
    e->state = REJECTED;
    e->reason = (uint8_t)reason;
}

void engine_fault(Engine *e, unsigned fault)
{
    if (e->fault)
        return;
    if (active(e))
        ++e->abandoned;
    e->fault = (uint8_t)fault;
    e->state = HALT;
    e->reason = R_FAULT;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors == UINT32_MAX)
        engine_fault(e, F_OVERFLOW);
    else
        ++e->tx_errors;
}

static void capture_outward(Engine *e, unsigned contacts)
{
    uint32_t bit = 1u << e->position;
    if (contacts & 1u)
        e->outward_x |= bit;
    if (contacts & 2u)
        e->outward_y |= bit;
}

static void capture_return(Engine *e, unsigned contacts)
{
    uint32_t bit = 1u << e->position;
    e->compared |= bit;
    if (contacts & 1u)
        e->return_x |= bit;
    if (contacts & 2u)
        e->return_y |= bit;
    e->mismatch_x = (e->outward_x ^ e->return_x) & e->compared;
    e->mismatch_y = (e->outward_y ^ e->return_y) & e->compared;
}

static void transition(Engine *e, unsigned previous, unsigned word)
{
    int delta = engine_direction(previous & 3u, word & 3u);
    if (!delta)
        return;
    if (delta == 2) {
        reject(e, R_JUMP);
        return;
    }
    unsigned sign = delta > 0 ? 1u : 2u;
    if (e->state == READY) {
        e->starts = 1;
        e->state = OUTWARD;
        e->direction = (uint8_t)sign;
        e->position = 0;
        e->outward_x = 0;
        e->outward_y = 0;
        capture_outward(e, previous >> 2);
        e->age = 0;
    }
    if (e->state == OUTWARD) {
        if (sign != e->direction) {
            reject(e, R_REVERSAL);
            return;
        }
        ++e->position;
        capture_outward(e, word >> 2);
        if (e->position == SWEEP_STEPS)
            e->state = RETURNING;
    } else if (e->state == RETURNING) {
        if (sign == e->direction) {
            reject(e, R_OVERSHOOT);
            return;
        }
        --e->position;
        capture_return(e, word >> 2);
        if (!e->position) {
            e->completed = 1;
            e->state = DONE;
        }
    }
}

void engine_sample(Engine *e, uint32_t now, unsigned raw)
{
    if (raw > 15u) {
        engine_fault(e, F_INPUT);
        return;
    }
    uint32_t elapsed = now - e->now;
    if (e->started && (!elapsed || elapsed >= 0x80000000u)) {
        engine_fault(e, F_TIME);
        return;
    }
    e->now = now;
    e->raw = (uint8_t)raw;
    if (e->fault)
        return; /* wall time and raw remain diagnostic, evidence frozen */
    if (e->samples == UINT32_MAX) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    ++e->samples;
    if (e->started && elapsed != 1u) {
        if (e->gaps == UINT32_MAX || UINT32_MAX - e->missed < elapsed - 1u) {
            engine_fault(e, F_OVERFLOW);
            return;
        }
        ++e->gaps;
        e->missed += elapsed - 1u;
        e->valid = 0;
        e->stable = 0;
        e->candidate = (uint8_t)raw;
        /* A gap before a trial also destroys the baseline. A fresh reset is
         * required; quiet input is not proof of preserved shaft position. */
        if (e->state != DONE && e->state != REJECTED)
            reject(e, R_GAP);
        return;
    }
    if (!e->started) {
        e->started = 1;
        e->candidate = (uint8_t)raw;
        return;
    }
    if (active(e)) {
        ++e->age;
        /* Rejecting deadline has precedence over a tied completion edge. */
        if (e->age >= SWEEP_TIMEOUT) {
            reject(e, R_TIMEOUT);
            return;
        }
    }
    if (raw != e->candidate) {
        e->candidate = (uint8_t)raw;
        e->stable = 0;
        return;
    }
    if (e->stable < 5u)
        ++e->stable;
    if (e->stable != 5u)
        return;
    unsigned previous = e->qualified;
    e->qualified = (uint8_t)raw;
    if (!e->valid) {
        e->valid = 1;
        if (e->state == WAIT_BASELINE)
            e->state = READY;
        return;
    }
    if (e->state == READY || active(e))
        transition(e, previous, raw);
}

bool engine_valid(const Engine *e)
{
    if (e->raw > 15 || e->qualified > 15 || e->valid > 1 ||
        e->state > HALT || e->fault > F_WATCHDOG || e->reason > R_FAULT ||
        e->position > SWEEP_STEPS || e->direction > 2)
        return false;
    if ((e->state == HALT) != (e->fault != F_NONE))
        return false;
    if (e->starts > 1 || e->completed > 1 || e->abandoned > 1)
        return false;
    if (e->starts != e->completed + e->abandoned + (active(e) ? 1u : 0u))
        return false;
    if ((e->state == DONE) != (e->completed == 1 && !e->fault))
        return false;
    if (e->outward_x > 0x1ffffu || e->outward_y > 0x1ffffu ||
        e->compared > 0xffffu || e->return_x > 0xffffu || e->return_y > 0xffffu)
        return false;
    if ((e->return_x | e->return_y) & ~e->compared)
        return false;
    if (e->mismatch_x != ((e->outward_x ^ e->return_x) & e->compared) ||
        e->mismatch_y != ((e->outward_y ^ e->return_y) & e->compared))
        return false;
    if (e->completed && (e->position || e->compared != 0xffffu))
        return false;
    if (e->age > SWEEP_TIMEOUT || (active(e) && e->age >= SWEEP_TIMEOUT))
        return false;
    if (e->state == OUTWARD && (!e->position || e->position >= SWEEP_STEPS))
        return false;
    if (e->state == RETURNING && !e->position)
        return false;
    if (e->starts && !e->direction)
        return false;
    return true;
}
