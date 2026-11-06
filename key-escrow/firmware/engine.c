/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

/* Terminal accounting is idempotent. A started trial reserves one eventual
 * outcome, so abandoning it cannot overflow while starts remains bounded. */
static void stop(Engine *e, uint8_t result, uint8_t fault)
{
    if (e->state == HALT)
        return;
    if (e->state == ACTIVE) {
        ++e->aborted;
        e->last_ms = e->age;
    }
    e->state = HALT;
    e->result = result;
    e->fault |= fault;
    e->baseline = 0;
    e->previous_clean = false;
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
    e->state = WAIT;
}

void engine_fault(Engine *e, uint8_t fault)
{
    stop(e, HARDWARE, fault ? fault : F_CONFIG);
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors < LIMIT)
        ++e->tx_errors;
    else
        stop(e, EXHAUSTED, F_EXHAUST);
}

/* Independent channels ensure chatter on a bolt endpoint cannot hide a key
 * removal. Five observations, not five elapsed milliseconds, qualify a bit. */
static void qualify(Engine *e)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((e->raw ^ e->candidate) & bit) {
            e->candidate ^= bit;
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY_MS) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY_MS) {
            e->valid |= bit;
            e->qualified = (uint8_t)((e->qualified & ~bit) |
                                     (e->candidate & bit));
        }
    }
}

static bool baseline(const Engine *e)
{
    return e->valid == 15 && e->raw == (EXTENDED | CLOSED) &&
           e->qualified == (EXTENDED | CLOSED);
}

static bool release_position(uint8_t mask)
{
    return (mask & (RETRACTED | EXTENDED | CLOSED)) ==
           (EXTENDED | CLOSED);
}

static bool open_position(uint8_t mask)
{
    return (mask & (RETRACTED | EXTENDED | KEY)) == (RETRACTED | KEY);
}

/* Completion has no automatic safety meaning. CYCLE only means that the
 * full observed insertion/open/close/removal trace met this fixed policy. */
static void complete(Engine *e)
{
    if (e->opened) {
        ++e->completed;
        e->result = CYCLE;
    } else {
        ++e->canceled;
        e->result = RETURNED;
    }
    e->last_ms = e->age;
    e->state = WAIT;
    e->baseline = 0;
    e->previous_clean = false;
}

static void observe_active(Engine *e, uint8_t previous)
{
    const uint8_t q = e->qualified;
    if ((q & (RETRACTED | EXTENDED)) == (RETRACTED | EXTENDED)) {
        stop(e, CONTRADICTORY, F_INPUT);
        return;
    }
    if (!(q & KEY)) {
        /* An extended bolt arriving on the release observation is too late.
         * Raw endpoints also exclude an already-departing mechanical state. */
        if ((previous & KEY) && release_position(previous) &&
            release_position(q) && e->raw == (EXTENDED | CLOSED))
            complete(e);
        else
            stop(e, EARLY_KEY, F_INPUT);
        return;
    }
    if ((previous & CLOSED) && !(q & CLOSED)) {
        if (!open_position(previous) || !open_position(q) ||
            !open_position(e->raw)) {
            stop(e, BAD_OPEN, F_INPUT);
            return;
        }
        e->opened = 1;
    }
    /* Extension needs a previously closed flap, not a tied closing edge. */
    if ((q & EXTENDED) && (!(q & CLOSED) || !(previous & CLOSED))) {
        stop(e, OPEN_EXTEND, F_INPUT);
        return;
    }
    if (!(q & CLOSED) && !open_position(q)) {
        stop(e, BAD_OPEN, F_INPUT);
        return;
    }
}

static void observe(Engine *e, uint8_t previous)
{
    if (e->state == ACTIVE) {
        observe_active(e, previous);
        return;
    }
    if (e->state == READY) {
        const uint8_t q = e->qualified;
        if ((q & 3u) == 3u) {
            stop(e, CONTRADICTORY, F_INPUT);
            return;
        }
        if (q & KEY) {
            if (!release_position(previous) || !release_position(q) ||
                !release_position(e->raw)) {
                stop(e, UNKEYED, F_INPUT);
                return;
            }
            if (e->starts == LIMIT) {
                stop(e, EXHAUSTED, F_EXHAUST);
                return;
            }
            ++e->starts;
            e->state = ACTIVE;
            e->age = 0;
            e->opened = 0;
            e->baseline = 0;
            e->previous_clean = false;
            return;
        }
        if (q != (EXTENDED | CLOSED))
            stop(e, UNKEYED, F_INPUT);
        return;
    }
    /* WAIT deliberately ignores startup position. It does not invent the
     * missing key history when attached to an already-open mock mechanism. */
    bool clean = baseline(e);
    if (clean && e->previous_clean) {
        if (e->baseline < BASELINE_MS)
            ++e->baseline;
    } else {
        e->baseline = 0;
    }
    e->previous_clean = clean;
    if (e->baseline == BASELINE_MS)
        e->state = READY;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->state == HALT) {
        /* Wall-clock transmission can continue, but evidence is frozen. */
        e->now = now;
        return;
    }
    if (e->sequence == UINT32_MAX) {
        stop(e, EXHAUSTED, F_EXHAUST);
        return;
    }
    if (e->have_sample && now - e->now != 1u) {
        ++e->gaps; /* first gap is terminal, hence no gap-counter overflow */
        e->now = now;
        stop(e, LOST, F_GAP);
        return;
    }
    if (raw > 15) {
        stop(e, HARDWARE, F_INPUT);
        return;
    }
    e->have_sample = true;
    e->now = now;
    ++e->sequence;
    e->raw = raw;
    uint8_t previous = e->qualified;
    qualify(e);
    if (e->state == ACTIVE) {
        ++e->age;
        /* Rejecting deadline precedes a same-observation completion. */
        if (e->age >= DEADLINE_MS) {
            stop(e, TIMEOUT, F_INPUT);
            return;
        }
    }
    if (e->valid == 15)
        observe(e, previous);
}

bool engine_valid(const Engine *e)
{
    if (e->state > HALT || e->result > HARDWARE || e->fault > 31 ||
        e->raw > 15 || e->qualified > 15 || e->valid > 15 || e->opened > 1)
        return false;
    if (e->starts > LIMIT || e->completed > e->starts ||
        e->canceled > e->starts || e->aborted > e->starts)
        return false;
    if (e->starts != e->completed + e->canceled + e->aborted +
                     (e->state == ACTIVE))
        return false;
    if (e->baseline > BASELINE_MS || e->age > DEADLINE_MS ||
        e->last_ms > DEADLINE_MS || e->tx_errors > LIMIT || e->gaps > 1)
        return false;
    if ((e->state == HALT) != (e->fault != 0))
        return false;
    if (e->state == HALT && e->result < EARLY_KEY)
        return false;
    if (e->state != HALT && e->result > RETURNED)
        return false;
    if (e->gaps && (!(e->fault & F_GAP) || e->result != LOST))
        return false;
    if (e->state == READY && (e->qualified != 10 || e->valid != 15 ||
                             e->baseline != BASELINE_MS))
        return false;
    if (e->state == ACTIVE) {
        if (e->valid != 15 || !(e->qualified & KEY) ||
            (e->qualified & 3u) == 3u || e->age >= DEADLINE_MS)
            return false;
        if (!(e->qualified & CLOSED) &&
            (!open_position(e->qualified) || !e->opened))
            return false;
    }
    if (e->result == CYCLE && !e->completed)
        return false;
    if (e->result == RETURNED && !e->canceled)
        return false;
    return true;
}
