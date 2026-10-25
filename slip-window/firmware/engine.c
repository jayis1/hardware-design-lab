/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <string.h>

static bool increment(uint32_t *value)
{
    if (*value==UINT32_MAX)
        return false;
    ++*value;
    return true;
}

bool engine_active(const Engine *e)
{
    return e->state>=CAST && e->state<=REST;
}

static void reject(Engine *e, Reason why)
{
    if (e->state!=REJECT)
        (void)increment(&e->rejected);
    e->state=REJECT;
    e->reason=why;
    e->arm_ms=0;
}

void engine_init(Engine *e, uint32_t now)
{
    memset(e,0,sizeof *e);
    e->now=now;
    e->state=CLEAR;
}

void engine_transport_loss(Engine *e)
{
    (void)increment(&e->transport_losses);
}

static void start_cycle(Engine *e)
{
    e->cycle_ms=0;
    e->residence_ms=0;
    e->drain_ms=0;
    e->longest_drain_ms=0;
    e->segment_ms=0;
    e->rest_ms=0;
    e->turn_ms=0;
    e->interruptions=0;
    e->reason=NONE;
    e->state=CAST;
}

static void finish_segment(Engine *e)
{
    if (e->segment_ms>e->longest_drain_ms)
        e->longest_drain_ms=e->segment_ms;
    e->segment_ms=0;
}

/* Timing belongs to the previous qualified state. A transition sample is
 * charged to that state before processing the newly qualified transition. */
static void account(Engine *e)
{
    if (!engine_active(e))
        return;
    if (!increment(&e->cycle_ms)) {
        reject(e,COUNTER_LIMIT);
        return;
    }
    if (e->cycle_ms>=CYCLE_LIMIT_MS) {
        reject(e,CYCLE_TIMEOUT);
        return;
    }
    bool ok=true;
    switch (e->state) {
    case CAST:
        ok=increment(&e->residence_ms);
        break;
    case TURN:
    case RETURN:
        ok=increment(&e->turn_ms);
        if (e->turn_ms>=TURN_LIMIT_MS)
            reject(e,TURN_TIMEOUT);
        break;
    case DRAIN:
        ok=increment(&e->drain_ms);
        ok=increment(&e->segment_ms) && ok;
        if (e->segment_ms>e->longest_drain_ms)
            e->longest_drain_ms=e->segment_ms;
        break;
    case REST:
        ok=increment(&e->rest_ms);
        break;
    default:
        break;
    }
    if (!ok)
        reject(e,COUNTER_LIMIT);
}

static void recover(Engine *e)
{
    /* Both raw and qualified evidence must agree; a held mark is not a new
     * fill. Do not erase the preceding cycle's measurements on rearming. */
    if (e->qualified && e->raw==(UP|LATCH) && e->stable==(UP|LATCH)) {
        if (e->arm_ms<ARM_MS)
            ++e->arm_ms;
        if (e->arm_ms==ARM_MS)
            e->state=READY;
    } else {
        e->arm_ms=0;
    }
}

static void transition(Engine *e, uint8_t before)
{
    uint8_t mask=e->stable;
    bool mark=(mask&MARK) && !(before&MARK);
    if ((mask&(UP|DOWN))==(UP|DOWN)) {
        reject(e,CONTRADICTION);
        return;
    }
    if (e->state==READY) {
        if ((mask&(UP|LATCH))!=(UP|LATCH)) {
            e->state=CLEAR;
            e->arm_ms=0;
        } else if (mark) {
            start_cycle(e);
        }
        return;
    }
    if (!engine_active(e))
        return;
    if (mark) {
        reject(e,MARK_AGAIN);
        return;
    }
    if (!(mask&LATCH)) {
        if (e->state==REST && (mask&UP)) {
            if (!increment(&e->completed)) {
                reject(e,COUNTER_LIMIT);
                return;
            }
            e->state=DONE;
            e->arm_ms=0;
        } else {
            reject(e,OPEN_EARLY);
        }
        return;
    }
    switch (e->state) {
    case CAST:
        if (!(mask&UP)) {
            e->turn_ms=0;
            e->state=(mask&DOWN)?DRAIN:TURN;
        }
        break;
    case TURN:
        if (mask&DOWN) {
            e->state=DRAIN;
            e->segment_ms=0;
        } else if (mask&UP) {
            /* An abandoned tip does not silently merge residence windows. */
            reject(e,NO_DRAIN);
        }
        break;
    case DRAIN:
        if (!(mask&DOWN)) {
            finish_segment(e);
            e->turn_ms=0;
            e->state=(mask&UP)?REST:RETURN;
        }
        break;
    case RETURN:
        if (mask&DOWN) {
            if (!increment(&e->interruptions)) {
                reject(e,COUNTER_LIMIT);
                return;
            }
            e->state=DRAIN;
            e->segment_ms=0;
        } else if (mask&UP) {
            e->state=REST;
        }
        break;
    case REST:
        if (!(mask&UP)) {
            /* Drain again, but record that the continuous segment ended. */
            if (!increment(&e->interruptions)) {
                reject(e,COUNTER_LIMIT);
                return;
            }
            e->state=(mask&DOWN)?DRAIN:TURN;
            e->turn_ms=0;
            e->segment_ms=0;
        }
        break;
    default:
        break;
    }
}

void engine_sample(Engine *e, uint8_t raw, uint32_t now)
{
    uint32_t delta=now-e->now;
    e->now=now;
    if (delta==0)
        return;
    e->raw=raw;
    if (raw>15u) {
        reject(e,BAD_INPUT);
        e->qualified=false;
        return;
    }
    if (delta!=1u) {
        (void)increment(&e->gaps);
        if (engine_active(e))
            reject(e,SAMPLE_GAP);
        else {
            e->state=CLEAR;
            e->arm_ms=0;
        }
        e->candidate=raw;
        e->qualified_ms=0;
        e->qualified=false;
        return;
    }
    account(e);
    if (raw!=e->candidate) {
        e->candidate=raw;
        e->qualified_ms=1;
    } else if (e->qualified_ms<QUALIFY_MS) {
        ++e->qualified_ms;
    }
    if (e->qualified_ms==QUALIFY_MS && (!e->qualified || raw!=e->stable)) {
        uint8_t previous=e->stable;
        e->stable=raw;
        e->qualified=true;
        transition(e,previous);
    }
    if (e->state==CLEAR || e->state==REJECT || e->state==DONE)
        recover(e);
}

const char *engine_state(State state)
{
    switch (state) {
    case CLEAR: return "clear";
    case READY: return "ready";
    case CAST: return "cast";
    case TURN: return "turn";
    case DRAIN: return "drain";
    case RETURN: return "return";
    case REST: return "rest";
    case DONE: return "done";
    case REJECT: return "reject";
    default: return "unknown";
    }
}

const char *engine_reason(Reason reason)
{
    switch (reason) {
    case NONE: return "none";
    case OPEN_EARLY: return "open_early";
    case CONTRADICTION: return "contradiction";
    case SAMPLE_GAP: return "sample_gap";
    case MARK_AGAIN: return "mark_again";
    case TURN_TIMEOUT: return "turn_timeout";
    case CYCLE_TIMEOUT: return "cycle_timeout";
    case COUNTER_LIMIT: return "counter_limit";
    case BAD_INPUT: return "bad_input";
    case NO_DRAIN: return "no_drain";
    default: return "unknown";
    }
}
