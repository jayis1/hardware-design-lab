/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

static bool increment(uint32_t *value)
{
    if (*value==UINT32_MAX)
        return false;
    ++*value;
    return true;
}
void engine_init(Engine *e)
{
    *e=(Engine){0};
}
void engine_fault(Engine *e,uint32_t fault)
{
    /* Retain the incomplete aperture, never relabel it as a zero-hit result. */
    e->state=HALT;
    e->faults|=fault;
}
void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx_errors))
        engine_fault(e,F_OVERFLOW);
}
static void qualify(Engine *e,uint8_t raw)
{
    /* Independent qualification prevents the other oscillator's chatter from
     * postponing this gate's timestamp. Initial high inputs are not anchors. */
    for (unsigned i=0;i<4;++i) {
        unsigned bit=1u<<i;
        uint8_t value=(uint8_t)((raw>>i)&1u);
        if (!e->stable[i] || e->candidate[i]!=value) {
            e->candidate[i]=value;
            e->stable[i]=1;
        } else if (e->stable[i]<QUALIFY) {
            ++e->stable[i];
        }
        if (e->stable[i]==QUALIFY) {
            e->valid|=bit;
            if (value)
                e->levels|=bit;
            else
                e->levels&=~bit;
        }
    }
}
static bool ages(Engine *e)
{
    for (unsigned i=0;i<2;++i) {
        if (e->side[i].phase) {
            ++e->side[i].age;
            if (e->side[i].age>=MAX_CYCLE_MS) {
                /* Deadline wins over a return qualifying on the same sample. */
                engine_fault(e,F_TIMEOUT);
                return false;
            }
        }
    }
    return true;
}
static bool endpoint(Oscillator *s,unsigned pair,bool *returned)
{
    *returned=false;
    if (pair&1u) {
        if (s->phase==2) {
            if (!increment(&s->cycles))
                return false;
            *returned=true;
        }
        if (!s->phase || *returned)
            s->age=0;
        s->phase=1;
    }
    if ((pair&2u) && s->phase)
        s->phase=2;
    return true;
}
static bool record_b(Engine *e,uint32_t offset)
{
    if (!increment(&e->assigned) || !increment(&e->pending))
        return false;
    if (e->pending==1)
        e->first=offset;
    e->last=offset;
    return true;
}
static bool close_window(Engine *e,uint32_t period)
{
    uint32_t *bin=e->pending==0?&e->zero:(e->pending==1?&e->one:&e->many);
    if (!increment(&e->windows) || !increment(bin))
        return false;
    if (UINT32_MAX-e->completed_hits<e->pending)
        return false;
    e->completed_hits+=e->pending;
    e->period=period;
    e->last_hits=e->pending;
    e->last_first=e->first;
    e->last_last=e->last;
    e->pending=0;
    e->first=0;
    e->last=0;
    return true;
}
static bool step(Engine *e,uint8_t raw,uint32_t previous)
{
    if (e->state==WAIT) {
        if (e->valid==15 && !e->levels && !raw)
            e->state=RUN;
        return true;
    }
    if (!ages(e))
        return true;
    /* Disjoint zones are mandatory within a pendulum, but both pendulums may
     * visit their corresponding endpoints simultaneously. No imposed order. */
    if ((e->levels&3u)==3u || (e->levels&12u)==12u) {
        engine_fault(e,F_OVERLAP);
        return true;
    }
    uint32_t rises=e->levels&~previous;
    bool a_return=false,b_return=false;
    bool aperture=e->side[0].phase!=0;
    uint32_t offset=e->side[0].age;
    if (!endpoint(&e->side[0],rises&3u,&a_return) ||
        !endpoint(&e->side[1],(rises>>2)&3u,&b_return))
        return false;
    /* (A_previous, A_current] is intentional. Coincident B completions close
     * the old aperture; a B completion on the first A anchor is unassigned. */
    if (b_return && aperture && !record_b(e,offset))
        return false;
    if (a_return && !close_window(e,offset))
        return false;
    return true;
}
void engine_sample(Engine *e,uint32_t now,uint8_t raw)
{
    raw&=15u;
    if (e->initialized && e->stamp==now)
        return;
    uint32_t elapsed=now-e->stamp;
    e->now=now;
    e->stamp=now;
    if (e->state==HALT)
        return;
    if (e->initialized && elapsed!=1) {
        if (!increment(&e->gaps))
            engine_fault(e,F_OVERFLOW);
        else
            engine_fault(e,F_GAP);
        return;
    }
    e->initialized=true;
    Engine next=*e;
    uint32_t previous=next.levels;
    qualify(&next,raw);
    /* Stage the entire sample: overflow cannot publish half a tied return. */
    if (!step(&next,raw,previous)) {
        engine_fault(e,F_OVERFLOW);
        return;
    }
    *e=next;
}
static bool offsets(uint32_t count,uint32_t first,uint32_t last,uint32_t bound)
{
    if (!count)
        return !first && !last;
    if (!first || first>last || last>bound)
        return false;
    if (count==1)
        return first==last;
    /* Opposite endpoints can qualify in consecutive five-sample runs. */
    return (uint64_t)(count-1)*MIN_CYCLE_MS<=last-first;
}
bool engine_valid(const Engine *e)
{
    if (!e || e->state>HALT || e->levels>15 || e->valid>15 || e->faults>63)
        return false;
    if ((e->state==HALT)!=(e->faults!=0))
        return false;
    if (e->gaps && !(e->faults&(F_GAP|F_OVERFLOW)))
        return false;
    for (unsigned i=0;i<2;++i) {
        if (e->side[i].phase>2 || e->side[i].age>MAX_CYCLE_MS)
            return false;
        if (!e->side[i].phase && (e->side[i].age || e->side[i].cycles))
            return false;
        if (e->state!=HALT && e->side[i].age>=MAX_CYCLE_MS)
            return false;
    }
    if ((uint64_t)e->zero+e->one+e->many!=e->windows)
        return false;
    if (e->windows!=e->side[0].cycles)
        return false;
    if ((uint64_t)e->completed_hits+e->pending!=e->assigned ||
        e->assigned>e->side[1].cycles)
        return false;
    if ((uint64_t)e->one+2ull*e->many>e->completed_hits)
        return false;
    if (!e->many && e->completed_hits!=e->one)
        return false;
    if (!offsets(e->pending,e->first,e->last,e->side[0].age))
        return false;
    if (!offsets(e->last_hits,e->last_first,e->last_last,e->period))
        return false;
    if (e->windows) {
        if (e->period<MIN_CYCLE_MS || e->period>=MAX_CYCLE_MS)
            return false;
        if ((!e->last_hits && !e->zero) || (e->last_hits==1 && !e->one) ||
            (e->last_hits>1 && !e->many) || e->last_hits>e->completed_hits)
            return false;
    } else if (e->period || e->last_hits || e->completed_hits) {
        return false;
    }
    if (e->state==WAIT && (e->windows || e->side[0].phase || e->side[1].phase))
        return false;
    return true;
}
