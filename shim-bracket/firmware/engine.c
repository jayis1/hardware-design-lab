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

/* Gray detents in increasing thickness order: 00, 01, 11, 10. */
static uint32_t index_of(uint32_t code)
{
    return code ^ (code >> 1);
}

static uint32_t contradiction(uint32_t pass, uint32_t refuse)
{
    for (unsigned thick=0;thick<4;++thick) {
        for (unsigned thin=0;thin<=thick;++thin) {
            if ((pass & (1u<<thick)) && (refuse & (1u<<thin)))
                return 1;
        }
    }
    return 0;
}

static void abandon(Engine *e)
{
    /* Every start reserves a disposition slot; totals cannot exceed starts. */
    if (e->state==ACTIVE)
        ++e->aborted;
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
    /* Bundle the selector bits: independent bit debounce can invent a detent.
     * The two result paddles remain independent of selector chatter. */
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
    /* tainted already tracks setup glitches before the qualified press. */
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
    } else {
        ++e->accepted;
        if (e->outcome==4u)
            e->pass |= 1u<<e->selected;
        else
            e->refuse |= 1u<<e->selected;
        e->conflict=contradiction(e->pass,e->refuse);
    }
    e->state=WAIT;
}

static void active_sample(Engine *e, uint8_t raw)
{
    ++e->age;
    /* Exact deadline is an abort, even if release qualifies on this sample. */
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
        /* Preserve setup uncertainty across the raw envelope before debounce.
         * A rejected short press can clear only after an observed low baseline. */
        if ((raw | e->levels) & 12u) {
            if ((raw & 3u)!=(previous & 3u) ||
                (e->levels & 3u)!=(previous & 3u) || (raw & 12u)==12u)
                e->tainted=1;
            if (e->levels & 12u)
                begin(e,previous,raw);
        } else {
            e->tainted=0;
        }
    }
    /* No automatic survey reset: accepted masks survive every rearm/gap. */
    if (e->state==WAIT && !((raw | e->levels) & 12u)) {
        e->state=READY;
        e->tainted=0;
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
    if (e->pass>15u || e->refuse>15u || e->conflict>1u || e->tainted>1u)
        return false;
    if (e->conflict!=contradiction(e->pass,e->refuse))
        return false;
    if ((uint64_t)e->accepted+e->rejected+e->aborted+(e->state==ACTIVE)!=e->starts)
        return false;
    if (population(e->pass)+population(e->refuse)>e->accepted)
        return false;
    if (!e->accepted && (e->pass || e->refuse))
        return false;
    if (e->accepted && !(e->pass || e->refuse))
        return false;
    if (e->selected>3u || e->age>MAX_PRESS_MS)
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
