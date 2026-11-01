/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>

/* Difference constraints: x[j] - x[i] <= bound, with an optional strict
 * inequality. Neutral contributes +/-1 inclusive; left-low means xL-xR>1.
 * This retains a deadband, rather than making neutral observations transitive
 * equalities. No absolute weight estimate is invented. */
typedef struct { int bound; bool strict; } Limit;
static bool tighter(Limit a, Limit b)
{
    return a.bound<b.bound || (a.bound==b.bound && a.strict && !b.strict);
}
static void constrain(Limit matrix[3][3], unsigned from, unsigned to,
                      int bound, bool strict)
{
    Limit proposed={bound,strict};
    if (tighter(proposed,matrix[from][to]))
        matrix[from][to]=proposed;
}
bool balance_conflict(uint32_t evidence)
{
    static const unsigned left[3]={0,0,1},right[3]={1,2,2};
    Limit d[3][3];
    for (unsigned i=0;i<3;++i)
        for (unsigned j=0;j<3;++j)
            d[i][j]=(Limit){i==j?0:100,false};
    for (unsigned pair=0;pair<3;++pair) {
        unsigned a=left[pair],b=right[pair];
        if (evidence & (1u<<(3*pair))) {
            constrain(d,a,b,1,false);
            constrain(d,b,a,1,false);
        }
        if (evidence & (2u<<(3*pair)))
            constrain(d,a,b,-1,true);
        if (evidence & (4u<<(3*pair)))
            constrain(d,b,a,-1,true);
    }
    for (unsigned k=0;k<3;++k)
        for (unsigned i=0;i<3;++i)
            for (unsigned j=0;j<3;++j) {
                if (d[i][k].bound==100 || d[k][j].bound==100)
                    continue;
                Limit via={d[i][k].bound+d[k][j].bound,
                           d[i][k].strict || d[k][j].strict};
                if (tighter(via,d[i][j]))
                    d[i][j]=via;
            }
    for (unsigned i=0;i<3;++i)
        if (d[i][i].bound<0 || (d[i][i].bound==0 && d[i][i].strict))
            return true;
    return false;
}
void engine_init(Engine *e)
{
    unsigned char *p=(unsigned char *)e;
    for (unsigned i=0;i<sizeof *e;++i)
        p[i]=0;
}
void engine_fault(Engine *e, uint32_t fault)
{
    /* One disposition slot was reserved at admission. */
    if (e->state==ACTIVE)
        ++e->aborted;
    e->faults |= fault;
    e->state=HALT;
}
void engine_tx_error(Engine *e)
{
    if (e->tx_errors==UINT32_MAX)
        engine_fault(e,F_OVERFLOW);
    else
        ++e->tx_errors;
}
static void qualify(Engine *e, uint8_t raw)
{
    /* Pair identity and beam position are independent bundles. */
    static const uint8_t masks[2]={3,12};
    for (unsigned i=0;i<2;++i) {
        uint8_t value=raw & masks[i];
        if (!e->stable[i] || value!=e->candidate[i]) {
            e->candidate[i]=value;
            e->stable[i]=1;
        } else if (e->stable[i]<QUALIFY) {
            ++e->stable[i];
        }
        if (e->stable[i]==QUALIFY) {
            e->valid |= masks[i];
            e->levels=(e->levels & ~masks[i]) | value;
        }
    }
}
static void reject(Engine *e)
{
    ++e->rejected;
    e->state=DONE;
}
static void begin(Engine *e)
{
    if (e->starts==UINT32_MAX) {
        engine_fault(e,F_OVERFLOW);
        return;
    }
    ++e->starts;
    e->selected=e->levels & 3u;
    e->outcome=e->levels >> 2;
    e->age=0;
    e->dwell=0;
    e->state=ACTIVE;
}
static void active(Engine *e, uint8_t raw)
{
    ++e->age;
    /* Timeout and identity loss take precedence over a settling endpoint. */
    if (e->age>=MAX_TRIAL_MS || (raw & 3u)!=e->selected ||
        (e->levels & 3u)!=e->selected) {
        reject(e);
        return;
    }
    unsigned beam=e->levels >> 2;
    if ((raw >> 2)==beam && (e->prior_raw >> 2)==beam && e->outcome==beam) {
        if (e->dwell<SETTLE_MS)
            ++e->dwell;
    } else {
        e->dwell=0;
    }
    e->outcome=beam;
    if (e->dwell<SETTLE_MS)
        return;
    if (beam==3u) {
        reject(e);
        return;
    }
    unsigned index=3*(e->selected-1)+beam;
    /* No member counter can exceed accepted, which cannot exceed starts. */
    ++e->counts[index];
    ++e->accepted;
    e->evidence |= 1u<<index;
    e->conflict=balance_conflict(e->evidence);
    e->state=DONE;
}
void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->initialized && now==e->stamp)
        return;
    bool gap=e->initialized && now-e->stamp!=1u;
    e->initialized=true;
    e->now=now;
    e->stamp=now;
    if (e->state==HALT)
        return;
    if (raw>15u) {
        engine_fault(e,F_CONFIG);
        return;
    }
    if (gap) {
        ++e->gaps;
        engine_fault(e,F_GAP);
        return;
    }
    qualify(e,raw);
    if (e->valid==15u) {
        if (e->state==ACTIVE) {
            active(e,raw);
        } else if (e->state==READY && (e->levels & 3u) &&
                   (raw & 3u)==(e->levels & 3u)) {
            begin(e);
        } else if ((e->state==WAIT || e->state==DONE) &&
                   !((raw | e->levels) & 3u)) {
            e->state=READY;
        }
    }
    e->prior_raw=raw;
}
bool engine_valid(const Engine *e)
{
    if (e->state>HALT || e->levels>15u || e->valid>15u || e->faults>15u)
        return false;
    if ((e->state==HALT)!=(e->faults!=0))
        return false;
    if (e->aborted>1u || (e->aborted && e->state!=HALT))
        return false;
    if ((uint64_t)e->accepted+e->rejected+e->aborted+(e->state==ACTIVE)!=e->starts)
        return false;
    uint64_t total=0;
    uint32_t evidence=0;
    for (unsigned i=0;i<9;++i) {
        total+=e->counts[i];
        if (e->counts[i])
            evidence |= 1u<<i;
    }
    if (total!=e->accepted || evidence!=e->evidence)
        return false;
    if (e->conflict!=(uint32_t)balance_conflict(e->evidence))
        return false;
    if (e->selected>3u || e->outcome>3u || e->dwell>SETTLE_MS || e->age>MAX_TRIAL_MS)
        return false;
    if (e->state==ACTIVE && (!e->selected || e->age>=MAX_TRIAL_MS ||
        e->dwell>=SETTLE_MS || e->valid!=15u || (e->levels & 3u)!=e->selected))
        return false;
    if (e->state==READY && ((e->levels & 3u) || e->valid!=15u))
        return false;
    if (e->gaps>1 || (e->gaps!=0)!=((e->faults & F_GAP)!=0))
        return false;
    return true;
}
