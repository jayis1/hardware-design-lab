/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

void engine_init(Engine *e)
{
    /* Explicit byte clearing avoids an implicit freestanding libc dependency. */
    unsigned char *bytes = (unsigned char *)e;
    for (unsigned i = 0; i < sizeof *e; ++i)
        bytes[i] = 0;
    e->state = WAIT;
}

static void reject(Engine *e, uint32_t reason)
{
    /* A reserved start guarantees room for its single terminal disposition. */
    if (e->state == ACTIVE)
        ++e->rejected;
    e->reason = reason;
    e->state = WAIT;
    e->age = 0;
    e->answer = 0;
    e->tainted = 0;
    e->envelope = 0;
    e->removing = false;
}

void engine_fault(Engine *e, uint32_t fault)
{
    if (e->state != HALT) {
        reject(e, fault == F_GAP ? R_GAP : R_FAULT);
        e->state = HALT;
    }
    e->fault |= fault;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors < LIMIT)
        ++e->tx_errors;
    else
        engine_fault(e, F_COUNTER);
}

static void qualify(Engine *e, uint8_t raw)
{
    /* Dock and encoded class are a bundled identity, paddles independent. */
    static const uint8_t masks[3] = {3, 4, 8};
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t mask = masks[i];
        uint8_t value = raw & mask;
        if (!e->stable[i] || e->candidate[i] != value) {
            e->candidate[i] = value;
            e->stable[i] = 1;
        } else if (e->stable[i] < QUALIFY) {
            ++e->stable[i];
        }
        if (e->stable[i] == QUALIFY) {
            e->qualified = (e->qualified & (uint8_t)~mask) | value;
            e->valid |= mask;
        }
    }
}

static void begin(Engine *e)
{
    if (e->starts == LIMIT) {
        engine_fault(e, F_COUNTER);
        return;
    }
    ++e->starts;
    e->state = ACTIVE;
    e->age = 0;
    e->answer = 0;
    e->truth = (e->qualified >> 1) & 1u;
    /* Insertion with any pressed/qualifying paddle is a rejected trial. */
    e->tainted = ((e->raw | e->qualified) & 12u) != 0;
    e->envelope = e->raw & 12u;
    e->removing = false;
}

static void finish(Engine *e)
{
    if (e->tainted || ((e->raw | e->qualified) & 12u)) {
        reject(e, R_AMBIGUOUS);
        return;
    }
    if (!e->answer) {
        reject(e, R_UNANSWERED);
        return;
    }
    /* Each accepted trial contributes exactly one confusion-matrix cell. */
    unsigned cell = 2u * e->truth + e->answer - 1u;
    ++e->cells[cell];
    ++e->completed;
    e->last_truth = e->truth + 1u;
    e->last_answer = e->answer;
    e->last_ms = e->age;
    e->state = WAIT;
    e->age = 0;
    e->answer = 0;
    e->tainted = 0;
    e->envelope = 0;
    e->removing = false;
    e->reason = R_NONE;
}

static void observe_active(Engine *e, uint8_t previous)
{
    ++e->age;
    /* Deadline has precedence over even an otherwise clean removal. */
    if (e->age >= TIMEOUT) {
        reject(e, R_TIMEOUT);
        return;
    }
    if (!(e->raw & 1u))
        e->removing = true;
    else {
        if (e->removing)
            e->tainted = 1; /* A raw removal/reentry breaks one presentation. */
        if (((e->raw >> 1) & 1u) != e->truth)
            e->tainted = 1;
    }
    /* Remember opposite raw activity across the whole presentation. */
    e->envelope |= e->raw & 12u;
    if (e->envelope == 12u)
        e->tainted = 1;
    uint8_t rising = e->qualified & (uint8_t)~previous & 12u;
    if (rising) {
        if (e->removing || e->answer || rising == 12u)
            e->tainted = 1;
        else
            e->answer = rising == 4u ? 1u : 2u;
    }
    if (!(e->qualified & 1u))
        finish(e);
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - e->now;
    e->now = now;
    if (e->state == HALT)
        return; /* Timestamp may advance; acquired sequence cannot. */
    if (e->seen && delta != 1u) {
        if (e->gaps < LIMIT)
            ++e->gaps;
        engine_fault(e, F_GAP);
        return;
    }
    e->seen = true;
    if (e->sequence == LIMIT) {
        engine_fault(e, F_COUNTER);
        return;
    }
    ++e->sequence;
    e->raw = raw & 15u;
    uint8_t previous = e->qualified;
    qualify(e, e->raw);
    if (e->valid != 15u)
        return;
    switch (e->state) {
    case WAIT:
        /* Class bit is irrelevant while dock is empty. */
        if (!((e->raw | e->qualified) & 13u))
            e->state = READY;
        break;
    case READY:
        if ((e->qualified & 1u) && !(previous & 1u))
            begin(e);
        break;
    case ACTIVE:
        observe_active(e, previous);
        break;
    default:
        engine_fault(e, F_CONFIG);
        break;
    }
}

bool engine_valid(const Engine *e)
{
    if (e->state > HALT || e->fault > 15u || e->valid > 15u)
        return false;
    if ((e->state == HALT) != (e->fault != 0))
        return false;
    if (e->sequence > LIMIT || e->starts > e->sequence)
        return false;
    if (e->completed > e->starts || e->rejected > e->starts)
        return false;
    if ((uint64_t)e->completed + e->rejected + (e->state == ACTIVE) != e->starts)
        return false;
    uint64_t sum = 0;
    for (unsigned i = 0; i < 4; ++i) {
        if (e->cells[i] > e->completed)
            return false;
        sum += e->cells[i];
    }
    if (sum != e->completed)
        return false;
    if (e->age >= TIMEOUT || e->answer > 2 || e->tainted > 1)
        return false;
    if (e->state != ACTIVE && (e->age || e->answer || e->tainted))
        return false;
    if (e->state == ACTIVE && e->valid != 15u)
        return false;
    if (e->reason > R_FAULT || e->gaps > LIMIT || e->tx_errors > LIMIT)
        return false;
    if (e->last_truth > 2 || e->last_answer > 2 || e->last_ms >= TIMEOUT)
        return false;
    if (!e->completed)
        return !(e->last_truth || e->last_answer || e->last_ms);
    if (!e->last_truth || !e->last_answer || !e->last_ms)
        return false;
    return e->cells[2u * (e->last_truth - 1u) + e->last_answer - 1u] != 0;
}
