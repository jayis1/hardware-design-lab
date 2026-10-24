/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static void hold(Atlas *a, uint32_t *t, unsigned mask, unsigned ms)
{
    while (ms--) atlas_sample(a, ++*t, (uint8_t)mask);
}
int main(void)
{
    Atlas a;
    AtlasEvent e;
    uint32_t t = 0;
    atlas_init(&a, t);
    hold(&a, &t, 1, 201);
    assert(a.known && a.stable == 1 && a.nodes[1].visits == 1);
    assert(!atlas_pop(&a, &e));
    hold(&a, &t, 0, 10);
    hold(&a, &t, 2, 201);
    assert(atlas_pop(&a, &e));
    assert(e.from == 1 && e.to == 2 && e.zero_ms == 10);
    assert(e.multi_ms == 0 && e.changes == 2);
    assert(a.edges[1][2].visits == 1);
    hold(&a, &t, 3, 7);
    hold(&a, &t, 1, 201);
    assert(atlas_pop(&a, &e) && e.multi_ms == 7);
    assert(e.from == 2 && e.to == 1);
    /* A brief excursion back to the same detent is still evidence. */
    hold(&a, &t, 0, 2);
    hold(&a, &t, 1, 201);
    assert(atlas_pop(&a, &e) && e.from == e.to);
    assert(e.zero_ms == 2);
    uint32_t before = a.transitions;
    atlas_sample(&a, t += 10, 4);
    assert(!a.known && a.gaps == 1);
    hold(&a, &t, 4, 201);
    assert(a.known && a.transitions == before);
    assert(!atlas_pop(&a, &e));
    /* No synthetic edge spans a sample gap; ordinary wrap is supported. */
    t = UINT32_MAX - 100;
    atlas_init(&a, t);
    hold(&a, &t, 8, 201);
    assert(a.known && a.stable == 8 && a.gaps == 0);
    hold(&a, &t, 0, 201);
    assert(atlas_pop(&a, &e) && e.to == 0);
    assert(atlas_population(15) == 4 && atlas_population(8) == 1);
    assert(atlas_add(UINT32_MAX - 1, 4) == UINT32_MAX);
    atlas_init(&a, 0); t = 0;
    hold(&a, &t, 1, 201);
    for (unsigned i = 0; i < ATLAS_QUEUE + 3; ++i)
        hold(&a, &t, (i % 2) ? 1 : 2, 201);
    assert(a.dropped == 3 && a.count == ATLAS_QUEUE);
    unsigned count = 0;
    while (atlas_pop(&a, &e)) ++count;
    assert(count == ATLAS_QUEUE);
    /* Invalid high bits are not silently interpreted as an open contact. */
    atlas_sample(&a, ++t, 16);
    assert(!a.known && a.invalid == 1);
    char frame[256];
    assert(wire_crc("123456789", 9) == 0x29b1);
    size_t n = wire_status(frame, sizeof frame, &a, t, 42, 0);
    assert(n && frame[n-1] == '\n' && !memcmp(frame, "CA1,S,42,", 9));
    assert(wire_status(frame, 8, &a, t, 42, 0) == 0);
    assert(wire_status(NULL, 100, &a, t, 42, 0) == 0);
    assert(wire_event(frame, sizeof frame, &e, 2) > 0);
    puts("PASS: detents, transitions, bounce, gaps, wrap, saturation, queue, CRC/bounds");
}
