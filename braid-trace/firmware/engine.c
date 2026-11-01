/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>

void engine_init(Engine *e)
{
    unsigned char *p=(unsigned char *)e;
    for (unsigned i=0;i<sizeof *e;++i)
        p[i]=0;
    e->state=WAIT;
    /* Base-three digits encode strand identities in left-to-right order. */
    e->permutation=21;
}

void engine_fault(Engine *e, uint32_t fault)
{
    /* Each admitted start reserves space for one disposition. */
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
    /* Two detents are one selector bundle; paddles qualify independently. */
    static const uint8_t masks[3]={3,4,8};
    for (unsigned i=0;i<3;++i) {
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

static void begin(Engine *e, uint32_t previous, uint8_t raw)
{
    if (e->starts==UINT32_MAX) {
        engine_fault(e,F_OVERFLOW);
        return;
    }
    ++e->starts;
    e->state=ACTIVE;
    e->age=0;
    e->selected=previous & 3u;
    e->outcome=e->levels & 12u;
    if ((e->selected!=1u && e->selected!=2u) ||
        (e->levels & 3u)!=e->selected || (raw & 3u)!=e->selected ||
        (e->outcome!=4u && e->outcome!=8u) ||
        (raw & 12u)!=e->outcome)
        e->tainted=1;
}

static void append(Engine *e)
{
    /* The full word is immutable. Reduction never erases source evidence.
     * Codes 1,2 are positive; codes 3,4 their respective inverses. */
    unsigned code=e->selected+(e->outcome==8u?2u:0u);
    unsigned index=e->accepted;
    e->packed[index/8] |= code << (4*(index%8));
    ++e->accepted;
    if (code<=2)
        ++e->positive;
    else
        ++e->negative;
    if (e->depth && ((e->reduced[e->depth-1]+1u)%4u+1u)==code) {
        --e->depth;
        e->reduced[e->depth]=0;
        ++e->cancelled;
    } else {
        e->reduced[e->depth++]=(uint8_t)code;
    }
    /* Both crossing signs exchange adjacent endpoint positions. */
    unsigned p[3]={e->permutation%3,(e->permutation/3)%3,e->permutation/9};
    unsigned at=e->selected-1;
    unsigned swap=p[at];
    p[at]=p[at+1];
    p[at+1]=swap;
    e->permutation=p[0]+3*p[1]+9*p[2];
}

static void complete(Engine *e)
{
    if (e->tainted) {
        ++e->rejected;
    } else if (e->accepted==WORD_LIMIT) {
        /* No truncation, ring overwrite or partial crossing at capacity. */
        engine_fault(e,F_CAPACITY);
        return;
    } else {
        append(e);
    }
    e->state=WAIT;
}

static void active_sample(Engine *e, uint8_t raw)
{
    ++e->age;
    /* Deadline takes precedence over release qualification. */
    if (e->age>=MAX_PRESS_MS) {
        engine_fault(e,F_TIMEOUT);
        return;
    }
    uint32_t opposite=e->outcome==4u?8u:4u;
    if ((raw & 3u)!=e->selected || (e->levels & 3u)!=e->selected ||
        ((raw | e->levels) & opposite))
        e->tainted=1;
    if (!((raw | e->levels) & 12u))
        complete(e);
}

void engine_sample(Engine *e, uint32_t now, uint8_t raw)
{
    if (e->initialized && now==e->stamp)
        return;
    uint32_t elapsed=now-e->stamp;
    bool lost=e->initialized && elapsed!=1u;
    e->initialized=true;
    e->stamp=now;
    e->now=now;
    if (e->state==HALT)
        return;
    if (raw>15u) {
        engine_fault(e,F_CONFIG);
        return;
    }
    if (lost) {
        /* Silence cannot establish that no crossing occurred. Never resume. */
        if (e->gaps==UINT32_MAX)
            engine_fault(e,F_OVERFLOW);
        else {
            ++e->gaps;
            engine_fault(e,F_GAP);
        }
        return;
    }
    uint32_t previous=e->levels;
    qualify(e,raw);
    if (e->valid!=15u)
        return;
    if (e->state==ACTIVE) {
        active_sample(e,raw);
    } else if (e->state==READY) {
        if ((raw | e->levels) & 12u) {
            e->paddle_seen |= raw & 12u;
            if ((raw & 3u)!=(previous & 3u) ||
                (e->levels & 3u)!=(previous & 3u) || e->paddle_seen==12u)
                e->tainted=1;
            if (e->levels & 12u)
                begin(e,previous,raw);
        } else {
            e->tainted=0;
            e->paddle_seen=0;
        }
    }
    if (e->state==WAIT && !((raw | e->levels) & 12u)) {
        e->state=READY;
        e->tainted=0;
        e->paddle_seen=0;
    }
}

bool engine_valid(const Engine *e)
{
    if (e->state>HALT || e->levels>15u || e->valid>15u || e->faults>63u)
        return false;
    if ((e->state==HALT)!=(e->faults!=0u))
        return false;
    if ((uint64_t)e->accepted+e->rejected+e->aborted+(e->state==ACTIVE)!=e->starts)
        return false;
    if (e->accepted>WORD_LIMIT || e->depth>WORD_LIMIT || e->tainted>1u)
        return false;
    if (e->positive+e->negative!=e->accepted || e->positive>WORD_LIMIT || e->negative>WORD_LIMIT)
        return false;
    if (e->depth+2u*e->cancelled!=e->accepted || e->cancelled>WORD_LIMIT/2)
        return false;
    if (e->selected>3u || e->age>MAX_PRESS_MS)
        return false;
    if (e->outcome!=0u && e->outcome!=4u && e->outcome!=8u && e->outcome!=12u)
        return false;
    if (e->state==ACTIVE && (e->valid!=15u || !e->outcome || e->age>=MAX_PRESS_MS))
        return false;
    if (e->state==ACTIVE && !e->tainted &&
        ((e->selected!=1 && e->selected!=2) || e->outcome==12u))
        return false;
    if (e->state==READY && (e->valid!=15u || (e->levels & 12u)))
        return false;
    if ((e->gaps!=0)!=((e->faults & F_GAP)!=0))
        return false;
    /* Recompute from the authoritative word before allowing transmission. */
    unsigned stack[WORD_LIMIT],depth=0,positive=0,negative=0,p[3]={0,1,2};
    for (unsigned i=0;i<WORD_LIMIT;++i) {
        unsigned code=(e->packed[i/8]>>(4*(i%8))) & 15u;
        if (i>=e->accepted) {
            if (code)
                return false;
            continue;
        }
        if (code<1 || code>4)
            return false;
        if (code<=2)
            ++positive;
        else
            ++negative;
        unsigned at=(code-1)%2,swap=p[at];
        p[at]=p[at+1];
        p[at+1]=swap;
        if (depth && ((stack[depth-1]+1)%4+1)==code)
            --depth;
        else
            stack[depth++]=code;
    }
    if (depth!=e->depth || positive!=e->positive || negative!=e->negative ||
        e->permutation!=p[0]+3*p[1]+9*p[2])
        return false;
    for (unsigned i=0;i<WORD_LIMIT;++i) {
        if (e->reduced[i]!=(i<depth?stack[i]:0u))
            return false;
    }
    return true;
}
