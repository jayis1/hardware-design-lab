/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>
#include <string.h>

bool engine_reset_allows_home(uint32_t reset_cause)
{
    /* RM0368 RCC_CSR: accept only a pin reset, never an automatic recovery.
     * A power-on must be followed by the operator's explicit home reset. */
    const uint32_t automatic=(1u<<25)|(1u<<27)|(1u<<28)|(1u<<29)|(1u<<30)|(1u<<31);
    return (reset_cause & (1u<<26)) && !(reset_cause & automatic);
}

static void increment(uint32_t *value)
{
    if (*value != UINT32_MAX) {
        ++*value;
    }
}

static uint32_t add_saturated(uint32_t value, uint32_t delta)
{
    if (delta > UINT32_MAX-value) {
        return UINT32_MAX;
    }
    return value+delta;
}

void engine_init(Engine *e, uint32_t now)
{
    if (!e) {
        return;
    }
    memset(e,0,sizeof *e);
    e->now=now;
    e->last_sample=now;
}

void engine_transport_loss(Engine *e)
{
    if (e) {
        increment(&e->transport_losses);
    }
}

uint32_t engine_flags(const Engine *e)
{
    if (!e) {
        return 0;
    }
    uint32_t result=0;
    if (e->ready) result|=FLAG_READY;
    if (e->known) result|=FLAG_KNOWN;
    if (e->active) result|=FLAG_ACTIVE;
    if (e->episode_bad) result|=FLAG_BAD_EPISODE;
    if (e->overdue) result|=FLAG_OVERDUE;
    return result;
}

static bool registered(uint8_t levels)
{
    return (levels & INPUT_REGISTERED)!=0;
}

static bool parked(uint8_t levels)
{
    return (levels & INPUT_PARKED)!=0;
}

static bool at_station(const Engine *e)
{
    return e->known && e->position%4u==0;
}

/* The lookup uses closure bits A=bit0 and B=bit1.
 * Positive rotation: 00 -> 01 -> 11 -> 10 -> 00.
 * Opposite corners cannot establish direction and invalidate absolute position.
 * Contact bounce may reverse one step; it never produces a fictitious full turn.
 */
static int decode(uint8_t before, uint8_t after)
{
    static const int8_t transitions[16]={
         0, 1,-1, 2,
        -1, 0, 2, 1,
         1, 2, 0,-1,
         2,-1, 1, 0
    };
    return transitions[((before & 3u)<<2)|(after & 3u)];
}

static void lose_position(Engine *e)
{
    /* Only a deliberate reset at the marked station can establish home again.
     * A later registration closure alone is not an absolute reference. */
    e->known=false;
    e->home_attempted=true;
    e->last_station_valid=false;
    if (e->active) {
        e->episode_bad=true;
    }
}

static void move(Engine *e, uint8_t before, uint8_t after)
{
    int step=decode(before,after);
    if (!step) {
        return;
    }
    if (step==2) {
        increment(&e->quad_faults);
        lose_position(e);
        return;
    }
    if (registered(before) || registered(after) || e->active) {
        increment(&e->motion_faults);
        if (e->active) {
            e->episode_bad=true;
        }
    }
    if (!e->known) {
        return;
    }
    if (step>0) {
        e->position=(uint8_t)((e->position+1u)%ORBIT_STEPS);
    } else {
        e->position=(uint8_t)((e->position+ORBIT_STEPS-1u)%ORBIT_STEPS);
    }
}

static void begin_episode(Engine *e, uint8_t before, uint8_t after)
{
    e->active=true;
    e->episode_ms=0;
    e->overdue=false;
    /* Require registration before AND after pickup. Simultaneous changes
     * cannot establish the ordering needed for an accepted use episode. */
    e->episode_bad=!registered(before) || !registered(after) || !at_station(e);
    e->episode_station=(uint8_t)(e->position/4u);
    if ((before & 3u)!=(after & 3u)) {
        e->episode_bad=true;
    }
}

static void finish_episode(Engine *e)
{
    e->last_duration=e->episode_ms;
    if (e->episode_bad || !at_station(e) || e->episode_station!=e->position/4u) {
        increment(&e->rejected);
    } else {
        increment(&e->accepted);
        increment(&e->station_counts[e->episode_station]);
        if (e->last_station_valid) {
            if (e->episode_station==e->last_station) {
                increment(&e->repeats);
            } else if (e->episode_station!=(e->last_station+1u)%ORBIT_STATIONS) {
                increment(&e->skips);
            }
        }
        e->last_station=e->episode_station;
        e->last_station_valid=true;
    }
    e->active=false;
    /* Keep episode_bad and overdue latched until the next pickup. */
}

static void accept_vector(Engine *e, uint8_t levels)
{
    if (!e->ready) {
        e->ready=true;
        e->levels=levels;
        if (!e->home_attempted) {
            e->home_attempted=true;
            /* Operator physically aligns marked station zero before reset. */
            e->known=levels==12u;
            e->position=0;
        }
        /* Booting with a tool already lifted cannot establish a start time. */
        return;
    }
    uint8_t before=e->levels;
    move(e,before,levels);
    if (registered(levels) && !registered(before) && !at_station(e)) {
        increment(&e->registration_faults);
    }
    if (e->active && (!registered(levels) || !registered(before))) {
        e->episode_bad=true;
    }
    if (parked(before) && !parked(levels)) {
        begin_episode(e,before,levels);
    } else if (!parked(before) && parked(levels) && e->active) {
        finish_episode(e);
    }
    e->levels=levels;
}

static void gap(Engine *e, uint8_t raw)
{
    increment(&e->gaps);
    lose_position(e);
    /* Discard qualification age, but keep the last accepted vector. A raw
     * transient after a gap must not fabricate a parked transition. */
    e->candidate=raw;
    e->candidate_age=0;
}

static void age_episode(Engine *e, uint32_t elapsed)
{
    if (!e->active) {
        return;
    }
    e->episode_ms=add_saturated(e->episode_ms,elapsed);
    if (e->episode_ms>=ORBIT_MAX_EPISODE) {
        e->overdue=true;
        e->episode_bad=true;
    }
}

void engine_sample(Engine *e, uint8_t closures, uint32_t now)
{
    if (!e) {
        return;
    }
    closures &= 15u;
    e->now=now;
    uint32_t elapsed=now-e->last_sample;
    if (!elapsed) {
        return;
    }
    e->last_sample=now;
    age_episode(e,elapsed);
    if (!e->sampled) {
        e->sampled=true;
        e->candidate=closures;
        e->candidate_age=0;
        return;
    }
    if (elapsed!=1u) {
        gap(e,closures);
        return;
    }
    if (closures!=e->candidate) {
        e->candidate=closures;
        e->candidate_age=0;
        return;
    }
    e->candidate_age=add_saturated(e->candidate_age,1u);
    if (e->candidate_age<ORBIT_DEBOUNCE) {
        return;
    }
    if (!e->ready || e->levels!=e->candidate) {
        accept_vector(e,e->candidate);
    }
}
