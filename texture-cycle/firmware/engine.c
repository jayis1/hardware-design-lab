/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>

void engine_init(Engine *e)
{
    unsigned char *p=(unsigned char *)e;
    for (unsigned i=0;i<sizeof *e;++i)
        p[i]=0;
    e->state=WAIT;
}

/* Captive disk detents A,B,C,D read 00,01,11,10. */
static uint32_t index_of(uint32_t code)
{
    return code ^ (code >> 1);
}

uint32_t engine_cycle(uint32_t graph)
{
    /* Four-node transitive closure is cheap, deterministic, and bounded.
     * Do not replace original edges with closure: inference is not a vote. */
    uint32_t reach[4];
    for (unsigned i=0;i<4;++i)
        reach[i]=(graph>>(4*i)) & 15u;
    for (unsigned via=0;via<4;++via) {
        for (unsigned from=0;from<4;++from) {
            if (reach[from] & (1u<<via))
                reach[from] |= reach[via];
        }
    }
    for (unsigned i=0;i<4;++i) {
        if (reach[i] & (1u<<i))
            return 1;
    }
    return 0;
}

static void abandon(Engine *e)
{
    /* A start reserves its eventual disposition. No half-counted update. */
    if (e->state==ACTIVE)
        ++e->aborted;
    e->anchored=0;
    e->state=WAIT;
}

void engine_fault(Engine *e, uint32_t fault)
{
    abandon(e);
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

static void gap(Engine *e)
{
    /* A saved reference may no longer describe the current physical setup. */
    abandon(e);
    e->valid=0;
    e->levels=0;
    for (unsigned i=0;i<3;++i)
        e->stable[i]=0;
    if (e->gaps==UINT32_MAX)
        engine_fault(e,F_OVERFLOW);
    else
        ++e->gaps;
}

static void qualify(Engine *e, uint8_t raw)
{
    /* Selector bits are a bundle; paddle qualification is independent. */
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
    e->selected=index_of(previous & 3u);
    e->outcome=e->levels & 12u;
    if ((e->levels & 3u)!=(previous & 3u) ||
        (raw & 3u)!=(previous & 3u) ||
        (e->outcome!=4u && e->outcome!=8u) ||
        (raw & 12u)!=e->outcome)
        e->tainted=1;
}

static void complete(Engine *e)
{
    if (e->tainted) {
        ++e->rejected;
        e->anchored=0;
    } else if (e->outcome==4u) {
        /* A new reference deliberately replaces an unused reference. */
        ++e->references;
        e->anchor=e->selected;
        e->anchored=1;
    } else if (!e->anchored || e->anchor==e->selected) {
        ++e->rejected;
        e->anchored=0;
    } else {
        ++e->preferences;
        e->graph |= 1u<<(4u*e->selected+e->anchor);
        e->cycle=engine_cycle(e->graph);
        /* Exactly one comparison per reference; avoid accidental reuse. */
        e->anchored=0;
    }
    e->state=WAIT;
}

static void active_sample(Engine *e, uint8_t raw)
{
    ++e->age;
    /* Release qualifying at the deadline loses, not a successful vote. */
    if (e->age>=MAX_PRESS_MS) {
        abandon(e);
        return;
    }
    uint32_t opposite=e->outcome==4u ? 8u : 4u;
    if (index_of(raw & 3u)!=e->selected ||
        index_of(e->levels & 3u)!=e->selected ||
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
        gap(e);
        return;
    }
    uint32_t previous=e->levels;
    qualify(e,raw);
    if (e->valid!=15u)
        return;
    if (e->state==ACTIVE) {
        active_sample(e,raw);
    } else if (e->state==READY) {
        /* Track raw uncertainty before a paddle reaches qualification. */
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

static unsigned population(uint32_t mask)
{
    unsigned count=0;
    while (mask) {
        count += mask & 1u;
        mask >>= 1;
    }
    return count;
}

bool engine_valid(const Engine *e)
{
    if (e->state>HALT || e->levels>15u || e->valid>15u || e->faults>7u)
        return false;
    if ((e->state==HALT)!=(e->faults!=0u))
        return false;
    if (e->graph>65535u || (e->graph & 0x8421u) || e->tainted>1u)
        return false;
    if (e->cycle!=engine_cycle(e->graph))
        return false;
    if ((uint64_t)e->references+e->preferences+e->rejected+e->aborted+
        (e->state==ACTIVE)!=e->starts)
        return false;
    if (population(e->graph)>e->preferences)
        return false;
    if ((e->preferences!=0u)!=(e->graph!=0u))
        return false;
    if (e->anchored>1u || e->anchor>3u || e->selected>3u)
        return false;
    if (e->preferences>e->references ||
        (e->anchored && e->preferences==e->references))
        return false;
    if (e->state==HALT && e->anchored)
        return false;
    if (e->age>MAX_PRESS_MS)
        return false;
    if (e->outcome!=0u && e->outcome!=4u && e->outcome!=8u && e->outcome!=12u)
        return false;
    if (e->state==ACTIVE && (e->valid!=15u || !e->outcome))
        return false;
    if (e->state==ACTIVE && (e->age>=MAX_PRESS_MS ||
        (e->outcome==12u && !e->tainted)))
        return false;
    if (e->state==READY && (e->valid!=15u || (e->levels & 12u)))
        return false;
    return true;
}
