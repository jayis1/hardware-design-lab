/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

/* starts bounds completed+aborted; finishing an existing trial cannot overflow. */
static void abandon(Engine *e, uint32_t reason)
{
    if (e->phase == ACTIVE) {
        ++e->aborted;
        e->reason = reason;
    }
    e->phase = BASELINE;
}

void engine_fault(Engine *e, uint32_t fault)
{
    abandon(e, R_FAULT);
    e->fault |= fault;
    e->phase = HALT;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors == LIMIT) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    ++e->tx_errors;
}

/* Each channel must independently acquire five matching observations.
 * No input can hide another input by chattering. */
static void qualify(Engine *e)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        uint8_t level = (uint8_t)(e->raw & bit);
        if (!e->run[i] || e->candidate[i] != level) {
            e->candidate[i] = level;
            e->run[i] = 1;
        } else if (e->run[i] < QUALIFY) {
            ++e->run[i];
        }
        if (e->run[i] == QUALIFY) {
            e->valid |= bit;
            e->qualified = (uint8_t)((e->qualified & ~bit) | level);
        }
    }
}

static void begin(Engine *e)
{
    if (e->starts == LIMIT) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    ++e->starts;
    e->phase = ACTIVE;
    e->age = 0;
    e->count = 0;
    e->reason = R_NONE;
    for (unsigned i = 0; i < WORD_SIZE; ++i)
        e->word[i] = 0;
}

static void complete(Engine *e)
{
    ++e->completed;
    e->last_age = e->age;
    e->last_count = e->count;
    for (unsigned i = 0; i < WORD_SIZE; ++i)
        e->last_word[i] = e->word[i];
    e->phase = BASELINE;
}

static void observe(Engine *e, uint8_t prior, uint8_t prior_raw)
{
    if (e->phase == BASELINE) {
        /* Held tray at boot or after abort never invents a session start. */
        if (e->valid == 15u && !e->qualified && !e->raw)
            e->phase = READY;
        return;
    }
    if (e->phase == READY) {
        if (e->qualified == 8u && e->raw == 8u && !prior && !(prior_raw & 7u))
            begin(e);
        else if (e->qualified || e->raw)
            /* Allow the tray's own five-sample settling window. */
            if ((e->raw & 7u) || (e->qualified & 7u))
                e->phase = BASELINE;
        return;
    }
    if (e->phase != ACTIVE)
        return;
    ++e->age;
    /* Rejecting deadline always takes priority over simultaneous removal. */
    if (e->age >= DEADLINE) {
        abandon(e, R_TIMEOUT);
        return;
    }
    if (!(e->qualified & 8u)) {
        /* No completion on ambiguous open-lid or chattering removal. */
        if (!(e->qualified & 7u) && !e->raw && !(prior & 7u))
            complete(e);
        else
            abandon(e, R_REMOVAL);
        return;
    }
    uint8_t opening = (uint8_t)(e->qualified & ~prior & 7u);
    if (!opening)
        return;
    /* A raw tray departure cannot authorize a new opening while debounce lags. */
    if (!(e->raw & 8u) || !(prior_raw & 8u)) {
        abandon(e, R_REMOVAL);
        return;
    }
    if (e->count == WORD_SIZE) {
        abandon(e, R_CAPACITY);
        return;
    }
    /* A mask is one unordered group, never an invented A-before-B order. */
    e->word[e->count++] = opening;
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t elapsed = now - e->now;
    e->now = now;
    if (e->phase == HALT)
        return;
    if (raw > 15u) {
        engine_fault(e, F_CONFIG);
        return;
    }
    if (e->started && elapsed != 1u) {
        if (e->gaps < LIMIT)
            ++e->gaps;
        engine_fault(e, F_GAP);
        return;
    }
    e->started = true;
    if (e->sequence == LIMIT) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    ++e->sequence;
    uint8_t prior = e->qualified;
    uint8_t prior_raw = e->raw;
    e->raw = raw;
    qualify(e);
    observe(e, prior, prior_raw);
}

static bool word_valid(const uint8_t *word, unsigned count)
{
    if (count > WORD_SIZE)
        return false;
    for (unsigned i = 0; i < WORD_SIZE; ++i) {
        if (i < count) {
            if (!word[i] || word[i] > 7u)
                return false;
        } else if (word[i]) {
            return false;
        }
    }
    return true;
}

bool engine_valid(const Engine *e)
{
    if (!e || e->phase > HALT || e->fault > 15u)
        return false;
    if ((e->phase == HALT) != (e->fault != 0))
        return false;
    if (e->raw > 15u || e->qualified > 15u || e->valid > 15u)
        return false;
    if (e->sequence > LIMIT || e->starts > LIMIT)
        return false;
    if (e->completed > LIMIT || e->aborted > LIMIT)
        return false;
    if (e->starts != e->completed + e->aborted + (e->phase == ACTIVE))
        return false;
    if (e->age > DEADLINE || e->last_age >= DEADLINE)
        return false;
    if (e->phase == ACTIVE && (e->age >= DEADLINE || !(e->qualified & 8u)))
        return false;
    if (e->gaps > LIMIT || e->tx_errors > LIMIT || e->reason > R_FAULT)
        return false;
    if (!e->completed && (e->last_count || e->last_age))
        return false;
    return word_valid(e->word, e->count) && word_valid(e->last_word, e->last_count);
}
