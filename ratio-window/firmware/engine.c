/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

/* 00 -> 01 -> 11 -> 10 -> 00 is the wiring-defined positive ring.
 * Two-bit transitions have no defensible direction and return sentinel 2. */
int quadrature(unsigned before, unsigned after)
{
    static const int8_t table[16] = {
         0, 1,-1, 2,
        -1, 0, 2, 1,
         1, 2, 0,-1,
         2,-1, 1, 0
    };
    if (before > 3u || after > 3u)
        return 2;
    return table[before * 4u + after];
}

void engine_init(Engine *e)
{
    *e = (Engine){0};
}

static void clear_window(Engine *e)
{
    e->steps = 0;
    e->output = 0;
    e->age = 0;
    e->input_dir = 0;
    e->output_dir = 0;
}

static void reject(Engine *e, unsigned reason)
{
    if (e->state == ACTIVE)
        ++e->bad;
    e->state = WAIT;
    e->reason = reason;
    e->quiet = 0;
    e->clean = false;
    clear_window(e);
}

void engine_fault(Engine *e, uint32_t fault)
{
    /* One final abandonment preserves starts = good + bad + active.
     * Fault callbacks are idempotent; timestamps alone may advance afterward. */
    if (e->state != HALT)
        reject(e, R_PLATFORM);
    e->state = HALT;
    e->fault |= fault;
}

void engine_tx_error(Engine *e)
{
    if (e->tx_errors != UINT32_MAX)
        ++e->tx_errors;
}

static void begin(Engine *e)
{
    ++e->starts;
    clear_window(e);
    e->state = ACTIVE;
    e->quiet = 0;
}

static void complete(Engine *e)
{
    ++e->good;
    if (e->good == 1u || e->output < e->minimum)
        e->minimum = e->output;
    if (e->output > e->maximum)
        e->maximum = e->output;
    e->total += e->output;
    e->last = e->output;
    e->last_ms = e->age;
    e->last_input = e->input_dir;
    e->last_output = e->output_dir;
    if (!e->output_dir)
        ++e->still;
    else if (e->input_dir == e->output_dir)
        ++e->same;
    else
        ++e->opposite;
    /* A completed cut is also the origin of the next aperture. Output
     * motion in pauses between input edges is not silently discarded. */
    begin(e);
}

static void qualify(Engine *e, unsigned raw, int delta[2])
{
    for (unsigned lane = 0; lane < 2; ++lane) {
        unsigned shift = 2u * lane;
        unsigned value = (raw >> shift) & 3u;
        unsigned bit = 1u << lane;
        if (!e->stable[lane] || e->candidate[lane] != value) {
            e->candidate[lane] = (uint8_t)value;
            e->stable[lane] = 1;
        } else if (e->stable[lane] < QUALIFY) {
            ++e->stable[lane];
        }
        if (e->stable[lane] == QUALIFY) {
            unsigned old = (e->qualified >> shift) & 3u;
            if (e->valid & bit)
                delta[lane] = quadrature(old, value);
            e->qualified = (e->qualified & ~(3u << shift)) | (value << shift);
            e->valid |= bit;
        }
    }
}

static bool advance(uint32_t *direction, int delta)
{
    if (!delta)
        return true;
    unsigned next = delta > 0 ? 1u : 2u;
    if (*direction && *direction != next)
        return false;
    *direction = next;
    return true;
}

void engine_sample(Engine *e, uint32_t now, uint32_t raw)
{
    uint32_t elapsed = now - e->now;
    e->now = now;
    if (e->state == HALT)
        return;
    if (raw > 15u) {
        engine_fault(e, F_INPUT);
        return;
    }
    if (e->seeded && elapsed != 1u) {
        engine_fault(e, F_TIME);
        return;
    }
    if (e->samples == LIMIT) {
        engine_fault(e, F_OVERFLOW);
        return;
    }
    e->seeded = true;
    ++e->samples;
    e->raw = raw;
    unsigned previous = e->qualified;
    int delta[2] = {0, 0};
    qualify(e, raw, delta);
    bool clean = e->valid == 3u && e->raw == e->qualified;
    if (e->state == WAIT) {
        /* Count only intervals with raw/qualified agreement at both ends.
         * A rejected short raw glitch restarts rearm, not just its edge count. */
        if (clean && e->clean && previous == e->qualified)
            ++e->quiet;
        else
            e->quiet = 0;
        e->clean = clean;
        if (e->quiet == REARM)
            begin(e);
        return;
    }
    ++e->age;
    /* Deadline precedence is explicit: a boundary at exactly 120000 is bad. */
    if (e->age >= DEADLINE) {
        reject(e, R_TIMEOUT);
        return;
    }
    if (delta[0] == 2 || delta[1] == 2) {
        reject(e, R_JUMP);
        return;
    }
    if (!advance(&e->input_dir, delta[0]) ||
        !advance(&e->output_dir, delta[1])) {
        reject(e, R_REVERSE);
        return;
    }
    if (delta[0])
        ++e->steps;
    if (delta[1])
        ++e->output;
    if (e->output > OUTPUT_LIMIT) {
        reject(e, R_CAPACITY);
        return;
    }
    /* Both lane deltas are processed before a cut. A simultaneous qualified
     * output edge belongs to the ending window, with no sub-sample ordering. */
    if (e->steps == TURN)
        complete(e);
}

bool engine_valid(const Engine *e)
{
    if (!e || e->samples > LIMIT || e->state > HALT || e->fault > 31u)
        return false;
    if ((e->state == HALT) != (e->fault != 0u))
        return false;
    if (e->raw > 15u || e->qualified > 15u || e->valid > 3u)
        return false;
    if ((uint64_t)e->good + e->bad + (e->state == ACTIVE) != e->starts)
        return false;
    if ((uint64_t)e->same + e->opposite + e->still != e->good)
        return false;
    if (e->starts > e->samples || (uint64_t)e->good * TURN > e->samples)
        return false;
    if (e->total > e->samples || e->output > OUTPUT_LIMIT || e->steps >= TURN)
        return false;
    if (e->input_dir > 2u || e->output_dir > 2u || e->reason > R_PLATFORM)
        return false;
    if (e->age >= DEADLINE || e->quiet >= REARM)
        return false;
    if ((e->steps != 0u) != (e->input_dir != 0u) ||
        (e->output != 0u) != (e->output_dir != 0u))
        return false;
    if (e->state == ACTIVE) {
        if (e->valid != 3u || e->quiet)
            return false;
    } else if (e->steps || e->output || e->age || e->input_dir || e->output_dir) {
        return false;
    }
    if (!e->good) {
        return !(e->minimum || e->maximum || e->total || e->last ||
                 e->last_ms || e->last_input || e->last_output);
    }
    if (e->minimum > e->last || e->last > e->maximum || e->maximum > OUTPUT_LIMIT)
        return false;
    if ((uint64_t)e->minimum * e->good > e->total ||
        (uint64_t)e->maximum * e->good < e->total)
        return false;
    if (e->total < (uint64_t)e->maximum + (uint64_t)e->minimum * (e->good-1u))
        return false;
    if (e->last_input < 1u || e->last_input > 2u || e->last_output > 2u)
        return false;
    if ((e->last != 0u) != (e->last_output != 0u))
        return false;
    /* The retained latest category must actually exist in the totals. */
    uint32_t category = !e->last_output ? e->still :
        (e->last_input == e->last_output ? e->same : e->opposite);
    if (!category || (e->still && e->minimum) ||
        (uint64_t)e->same + e->opposite > e->total)
        return false;
    if (e->total > (uint64_t)e->minimum +
        (uint64_t)e->maximum * (e->good - 1u))
        return false;
    if (e->last_ms < TURN * QUALIFY || e->last_ms >= DEADLINE)
        return false;
    return true;
}
