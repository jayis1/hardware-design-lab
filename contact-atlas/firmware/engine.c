/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <limits.h>
#include <stddef.h>
#include <string.h>

uint32_t atlas_add(uint32_t value, uint32_t increment)
{
    if (UINT32_MAX - value < increment)
        return UINT32_MAX;
    return value + increment;
}

unsigned atlas_population(uint8_t mask)
{
    unsigned result = 0;
    for (unsigned bit = 0; bit < 4; ++bit) {
        if (mask & (1u << bit))
            ++result;
    }
    return result;
}

void atlas_init(Atlas *a, uint32_t now)
{
    if (!a)
        return;
    memset(a, 0, sizeof *a);
    a->previous = now;
}

static void forget_interval(Atlas *a, uint8_t mask)
{
    /* Keep historical evidence but never bridge an unobserved interval. */
    a->known = false;
    a->travelling = false;
    a->candidate = mask;
    a->raw = mask;
    a->candidate_age = 0;
    a->journey_ms = 0;
    a->zero_ms = 0;
    a->multi_ms = 0;
    a->changes = 0;
    a->seeded = true;
}

static void enqueue(Atlas *a, const AtlasEvent *event)
{
    if (a->count == ATLAS_QUEUE) {
        /* Drop newest: the order of already queued evidence is preserved. */
        a->dropped = atlas_add(a->dropped, 1);
        return;
    }
    unsigned tail = (a->head + a->count) % ATLAS_QUEUE;
    a->queue[tail] = *event;
    ++a->count;
}

bool atlas_pop(Atlas *a, AtlasEvent *event)
{
    if (!a || !event || !a->count)
        return false;
    *event = a->queue[a->head];
    a->head = (uint8_t)((a->head + 1u) % ATLAS_QUEUE);
    --a->count;
    return true;
}

static void visit_node(Atlas *a, uint8_t mask)
{
    AtlasNode *node = &a->nodes[mask];
    node->visits = atlas_add(node->visits, 1);
    a->stable = mask;
    a->known = true;
}

static void finish_journey(Atlas *a, uint32_t now)
{
    AtlasEdge *edge = &a->edges[a->stable][a->candidate];
    AtlasEvent event;
    a->transitions = atlas_add(a->transitions, 1);
    event.number = a->transitions;
    event.time = now;
    event.duration = a->journey_ms;
    event.zero_ms = a->zero_ms;
    event.multi_ms = a->multi_ms;
    event.changes = a->changes;
    event.from = a->stable;
    event.to = a->candidate;
    if (!edge->visits || event.duration < edge->minimum_ms)
        edge->minimum_ms = event.duration;
    if (event.duration > edge->maximum_ms)
        edge->maximum_ms = event.duration;
    edge->visits = atlas_add(edge->visits, 1);
    edge->zero_ms = atlas_add(edge->zero_ms, event.zero_ms);
    edge->multi_ms = atlas_add(edge->multi_ms, event.multi_ms);
    enqueue(a, &event);
    visit_node(a, a->candidate);
    a->travelling = false;
    a->journey_ms = 0;
}

static void start_journey(Atlas *a)
{
    a->travelling = true;
    a->journey_ms = 0;
    a->zero_ms = 0;
    a->multi_ms = 0;
    a->changes = 1;
}

static void account_interval(Atlas *a, uint32_t elapsed)
{
    /* Left-hand sample holds for this interval. No sub-ms claims. */
    if (a->known && !a->travelling) {
        AtlasNode *node = &a->nodes[a->stable];
        node->dwell_ms = atlas_add(node->dwell_ms, elapsed);
    }
    if (!a->travelling)
        return;
    a->journey_ms = atlas_add(a->journey_ms, elapsed);
    if (!a->raw)
        a->zero_ms = atlas_add(a->zero_ms, elapsed);
    if (atlas_population(a->raw) > 1)
        a->multi_ms = atlas_add(a->multi_ms, elapsed);
}

void atlas_sample(Atlas *a, uint32_t now, uint8_t mask)
{
    if (!a)
        return;
    uint32_t elapsed = now - a->previous;
    /* Multiple calls within one tick contribute no new timing evidence. */
    if (!elapsed)
        return;
    a->previous = now;
    if (mask >= ATLAS_MASKS) {
        a->invalid = atlas_add(a->invalid, 1);
        forget_interval(a, 0);
        a->seeded = false;
        return;
    }
    if (elapsed > ATLAS_MAX_STEP_MS) {
        a->gaps = atlas_add(a->gaps, 1);
        forget_interval(a, mask);
        return;
    }
    if (!a->seeded) {
        forget_interval(a, mask);
        return;
    }
    account_interval(a, elapsed);
    if (mask != a->raw) {
        if (a->known && !a->travelling)
            start_journey(a);
        else if (a->travelling)
            a->changes = atlas_add(a->changes, 1);
        a->raw = mask;
        a->candidate = mask;
        a->candidate_age = 0;
    } else {
        a->candidate_age = atlas_add(a->candidate_age, elapsed);
    }
    if (a->candidate_age < ATLAS_SETTLE_MS)
        return;
    if (!a->known) {
        visit_node(a, a->candidate);
        return;
    }
    if (a->travelling)
        finish_journey(a, now);
}
