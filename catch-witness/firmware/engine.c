/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <stddef.h>

static bool active(const Engine *e)
{
    return e->state==SETTLING || e->state==LATCHED;
}
static bool increment(uint32_t *value)
{
    if (*value==UINT32_MAX)
        return false;
    ++*value;
    return true;
}
static void clear_credits(Engine *e)
{
    e->handle_credit=0;
    e->tug_credit=0;
    e->handle_age=0;
    e->tug_age=0;
}
void engine_init(Engine *e)
{
    *e=(Engine){0};
}
void engine_fault(Engine *e, uint32_t fault)
{
    if (active(e))
        ++e->aborted; /* Conservation guarantees room: active implies starts>aborted. */
    e->state=HALT;
    e->faults|=fault;
    clear_credits(e);
}
void engine_tx_error(Engine *e)
{
    if (!increment(&e->tx_errors))
        engine_fault(e,F_OVERFLOW);
}
static void qualify(Engine *e, uint8_t raw)
{
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
static void age_credit(uint32_t *credit, uint32_t *age)
{
    if (!*credit)
        return;
    if (*age==CREDIT_MS) {
        *credit=0;
        *age=0;
    } else {
        ++*age;
    }
}
static void finish(Engine *e)
{
    e->state=DONE;
    e->rearm=0;
    clear_credits(e);
}
/* All counters are modified on a copy. A failing increment rolls the complete
 * observation back before abandoning the previous active episode exactly once. */
static bool step(Engine *e, uint8_t raw, uint32_t previous)
{
    uint32_t rises=e->levels & ~previous;
    uint32_t falls=previous & ~e->levels;
    if (e->state==WAIT || e->state==DONE) {
        if (e->valid==15 && !raw && !e->levels && !e->prior_raw) {
            if (e->rearm<REARM_MS)
                ++e->rearm;
            if (e->rearm==REARM_MS)
                e->state=READY;
        } else {
            e->rearm=0;
        }
        return true;
    }
    if (e->state==READY) {
        if (rises & DOOR) {
            if (!increment(&e->starts))
                return false;
            e->state=SETTLING;
            e->age=0;
            e->dwell=0;
            clear_credits(e);
        }
        return true;
    }
    if (!active(e))
        return true;
    ++e->age;
    age_credit(&e->handle_credit,&e->handle_age);
    age_credit(&e->tug_credit,&e->tug_age);
    /* The rejecting deadline wins, even if opening qualifies on this tick. */
    if (e->age>=MAX_TRIAL_MS) {
        if (!increment(&e->timed_out))
            return false;
        finish(e);
        return true;
    }
    if (!(e->levels & DOOR)) {
        uint32_t *counter;
        if (e->state==SETTLING)
            counter=&e->unseated;
        else if (e->handle_credit)
            counter=&e->intentional;
        else if (e->tug_credit)
            counter=&e->challenged;
        else
            counter=&e->unexplained;
        if (!increment(counter))
            return false;
        finish(e);
        return true;
    }
    if (e->state==SETTLING) {
        /* Both raw endpoints and qualified endpoints must establish closure.
         * A raw glitch receives no retroactive settlement credit on recovery. */
        if ((e->levels&3)==3 && (previous&3)==3 &&
            (raw&3)==3 && (e->prior_raw&3)==3 &&
            !(raw&HANDLE) && !(e->prior_raw&HANDLE) &&
            !(e->levels&HANDLE) && !(previous&HANDLE)) {
            ++e->dwell;
            if (e->dwell==SETTLE_MS) {
                if (!increment(&e->settled))
                    return false;
                e->state=LATCHED;
            }
        } else {
            e->dwell=0;
        }
        return true;
    }
    if (falls&BOLT) {
        if (!increment(&e->bolt_losses))
            return false;
    }
    /* Associations start only on fresh edges strictly before opening. They
     * are evidence tags, not authorization to move any mechanism. */
    if (rises&HANDLE) {
        e->handle_credit=1;
        e->handle_age=0;
    }
    if (rises&TUG) {
        e->tug_credit=1;
        e->tug_age=0;
    }
    return true;
}
void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    raw &= 15u;
    if (e->initialized && now==e->stamp)
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
    if (!step(&next,raw,previous)) {
        engine_fault(e,F_OVERFLOW);
        return;
    }
    next.prior_raw=raw;
    *e=next;
}
bool engine_valid(const Engine *e)
{
    if (!e || e->state>HALT || e->levels>15 || e->valid>15 ||
        e->faults>15 || e->handle_credit>1 || e->tug_credit>1)
        return false;
    uint64_t ended=(uint64_t)e->intentional+e->challenged+e->unexplained+
        e->unseated+e->timed_out+e->aborted;
    if (ended+(active(e)?1u:0u)!=e->starts)
        return false;
    uint64_t retained=(uint64_t)e->intentional+e->challenged+e->unexplained;
    if (retained>e->settled || e->settled>e->starts ||
        (uint64_t)e->settled+e->unseated>e->starts)
        return false;
    if (e->age>MAX_TRIAL_MS || e->dwell>SETTLE_MS ||
        e->handle_age>CREDIT_MS || e->tug_age>CREDIT_MS)
        return false;
    if ((e->state==HALT)!=(e->faults!=0))
        return false;
    if (e->gaps && !(e->faults&F_GAP) && !(e->faults&F_OVERFLOW))
        return false;
    if (e->state!=LATCHED && (e->handle_credit || e->tug_credit))
        return false;
    if ((!e->handle_credit && e->handle_age) || (!e->tug_credit && e->tug_age))
        return false;
    if (e->state==LATCHED && (e->dwell!=SETTLE_MS || !e->settled))
        return false;
    if (active(e) && (!(e->levels&DOOR) || e->age>=MAX_TRIAL_MS))
        return false;
    return true;
}
