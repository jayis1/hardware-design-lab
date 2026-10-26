/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

void cam_init(Cam *e, uint32_t now)
{
    *e = (Cam){0};
    e->previous = now;
    e->candidate_since = now;
}

void cam_fault(Cam *e)
{
    e->state = CAM_FAULT;
    e->reason = CAM_EXHAUSTED;
}

static void reject(Cam *e, enum cam_reason reason)
{
    if (e->state == CAM_FAULT)
        return;
    if (e->state == CAM_ACTIVE) {
        if (e->rejected == UINT32_MAX) {
            cam_fault(e);
            return;
        }
        ++e->rejected;
    }
    e->state = CAM_WAIT;
    e->reason = reason;
    e->travel = 0;
}

static void begin(Cam *e, uint32_t now)
{
    e->state = CAM_ACTIVE;
    e->start = now;
    e->travel = 0;
    e->closed = 0;
    e->transitions = 0;
    e->have_edge = false;
    for (unsigned i = 0; i < CAM_BINS; ++i)
        e->bins[i] = 0;
    /* Last rejection reason deliberately stays visible until reset. */
}

static void complete(Cam *e, uint32_t now)
{
    if (e->completed == UINT32_MAX) {
        cam_fault(e);
        return;
    }
    ++e->completed;
    e->result.duration = now - e->start;
    e->result.closed = e->closed;
    e->result.transitions = e->transitions;
    for (unsigned i = 0; i < CAM_BINS; ++i)
        e->result.bins[i] = e->bins[i];
}

static void qualify_follower(Cam *e, uint32_t now, bool raw)
{
    if (raw != e->candidate) {
        e->candidate = raw;
        e->candidate_since = now;
    }
    if (now - e->candidate_since >= CAM_DEBOUNCE) {
        e->follower = raw;
        e->follower_ready = true;
    }
}

static int decode(uint8_t previous, uint8_t phase)
{
    /* 00 -> 01 -> 11 -> 10 -> 00 is positive after input inversion.
     * A two-bit jump is never guessed, even if direction was established. */
    static const int8_t table[16] = {
        0, 1, -1, 2,
        -1, 0, 2, 1,
        1, 2, 0, -1,
        2, -1, 1, 0
    };
    return table[previous * 4u + phase];
}

static bool count_edge(Cam *e)
{
    if (e->travel == CAM_COUNTS) {
        reject(e, CAM_LENGTH);
        return false;
    }
    unsigned bin = e->travel / 10u;
    ++e->travel;
    if (e->follower) {
        ++e->closed;
        ++e->bins[bin];
    }
    if (e->have_edge && e->edge_follower != e->follower)
        ++e->transitions;
    e->edge_follower = e->follower;
    e->have_edge = true;
    return true;
}

void cam_tick(Cam *e, uint32_t now, uint8_t raw)
{
    uint32_t elapsed = now - e->previous;
    if (!elapsed)
        return;
    e->previous = now;
    raw &= 15u;
    e->raw = raw;
    bool index = (raw & 4u) != 0;
    uint8_t phase = raw & 3u;
    bool rising = index && !e->index;
    int step = decode(e->phase, phase);
    e->phase = phase;
    e->index = index;
    if (e->state == CAM_FAULT)
        return;
    if (!e->initialized) {
        e->initialized = true;
        e->candidate = (raw & 8u) != 0;
        e->candidate_since = now;
        /* Boot with index held must not establish an origin. */
        return;
    }
    if (elapsed != 1u) {
        if (e->gaps == UINT32_MAX) {
            cam_fault(e);
            return;
        }
        ++e->gaps;
        reject(e, CAM_GAP);
        e->follower_ready = false;
        e->candidate = (raw & 8u) != 0;
        e->candidate_since = now;
        return;
    }
    qualify_follower(e, now, (raw & 8u) != 0);
    if (step == 2) {
        reject(e, CAM_INVALID);
        return;
    }
    if (step < 0) {
        reject(e, CAM_REVERSE);
        return;
    }
    if (e->state == CAM_ACTIVE) {
        /* The deadline sample is not a successful completion. */
        if (now - e->start >= CAM_TIMEOUT) {
            reject(e, CAM_TIMEOUT_REASON);
            return;
        }
        if (step > 0 && !count_edge(e))
            return;
        if (rising) {
            if (e->travel != CAM_COUNTS) {
                reject(e, CAM_LENGTH);
                return;
            }
            complete(e, now);
            if (e->state != CAM_FAULT)
                begin(e, now);
        }
    } else if (rising && e->follower_ready) {
        /* An index that accompanies a valid positive edge is the origin;
         * that edge belongs to the preceding, unobserved revolution. */
        begin(e, now);
    }
}

bool cam_valid(const Cam *e)
{
    if (!e || e->state > CAM_FAULT || e->reason > CAM_EXHAUSTED)
        return false;
    if (e->state == CAM_FAULT && e->reason != CAM_EXHAUSTED)
        return false;
    if (e->state != CAM_FAULT && e->reason == CAM_EXHAUSTED)
        return false;
    if (e->raw > 15u || e->travel > CAM_COUNTS)
        return false;
    if (e->state == CAM_WAIT && e->travel)
        return false;
    unsigned sum = 0;
    unsigned minimum_changes = 0;
    unsigned maximum_changes = 0;
    for (unsigned i = 0; i < CAM_BINS; ++i) {
        unsigned count = e->result.bins[i];
        if (count > 10u)
            return false;
        sum += count;
        unsigned minority = count < 5u ? count : 10u - count;
        if (minority)
            ++minimum_changes;
        maximum_changes += minority == 5u ? 9u : 2u * minority;
        if (i) {
            unsigned before = e->result.bins[i - 1u];
            bool both_uniform = (before == 0u || before == 10u) &&
                                (count == 0u || count == 10u);
            if (both_uniform && before != count)
                ++minimum_changes;
            if (!both_uniform || before != count)
                ++maximum_changes;
        }
    }
    if (sum != e->result.closed || e->result.transitions > 399u)
        return false;
    if (e->result.transitions < minimum_changes ||
        e->result.transitions > maximum_changes)
        return false;
    if (!e->completed) {
        if (sum || e->result.duration || e->result.transitions)
            return false;
    } else {
        if (!e->result.duration || e->result.duration >= CAM_TIMEOUT)
            return false;
        if (e->result.duration < CAM_COUNTS)
            return false;
        if ((sum == 0 || sum == CAM_COUNTS) && e->result.transitions)
            return false;
        if (sum && sum < CAM_COUNTS && !e->result.transitions)
            return false;
    }
    return true;
}
