/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static uint32_t now;
static unsigned phase;
static const uint8_t gray[] = {0,1,3,2};
static void sample(Cam *e, uint8_t flags, unsigned ms)
{
    while (ms--) cam_tick(e, ++now, gray[phase] | flags);
}
static void setup(Cam *e)
{
    now = 0; phase = 0;
    cam_init(e, now);
    sample(e, 0, 10);
    sample(e, 4, 1);
    sample(e, 0, 10);
    assert(e->state == CAM_ACTIVE);
}
static void edge(Cam *e, uint8_t flags)
{
    phase = (phase + 1) % 4;
    sample(e, flags, 1);
    sample(e, flags & 8, 19);
}
static void good_turn(Cam *e)
{
    for (unsigned i = 0; i < 400; ++i) {
        uint8_t follower = i < 200 ? 8 : 0;
        sample(e, follower, 6);
        edge(e, follower | (i == 399 ? 4 : 0));
    }
}
int main(int argc, char **argv)
{
    Cam e;
    setup(&e); good_turn(&e);
    assert(e.completed == 1 && e.rejected == 0);
    assert(e.result.closed == 200 && e.result.transitions == 1);
    for (unsigned b = 0; b < 40; ++b) assert(e.result.bins[b] == (b < 20 ? 10 : 0));
    char frame[256];
    size_t n = cam_encode(&e, now, 0, frame, sizeof frame);
    assert(n && n < sizeof frame);
    assert(!cam_encode(&e, now, 0, frame, 10));
    n = cam_encode(&e, now, 0, frame, sizeof frame);
    if (argc > 1) {
        FILE *f = fopen(argv[1], "w"); assert(f);
        fputs("# Author: jayis1; synthetic C encoder fixture, not hardware\n", f);
        fwrite(frame, 1, n, f); fclose(f);
    }
    setup(&e); phase = 3; sample(&e, 0, 1);
    assert(e.rejected == 1 && e.reason == CAM_REVERSE);
    setup(&e); phase = 2; sample(&e, 0, 1);
    assert(e.rejected == 1 && e.reason == CAM_INVALID);
    setup(&e); now += 4; sample(&e, 0, 1);
    assert(e.rejected == 1 && e.gaps == 1 && e.reason == CAM_GAP);
    setup(&e); for (unsigned i = 0; i < 401; ++i) edge(&e, 0);
    assert(e.rejected == 1 && e.reason == CAM_LENGTH);
    setup(&e); for (unsigned i = 0; i < 399; ++i) edge(&e, i == 398 ? 4 : 0);
    assert(e.completed == 0 && e.rejected == 1 && e.reason == CAM_LENGTH);
    setup(&e); sample(&e, 0, CAM_TIMEOUT);
    assert(e.rejected == 1 && e.reason == CAM_TIMEOUT_REASON);
    setup(&e); e.rejected = UINT32_MAX; phase = 3; sample(&e, 0, 1);
    assert(e.state == CAM_FAULT);
    setup(&e); e.completed = UINT32_MAX; good_turn(&e);
    assert(e.state == CAM_FAULT);
    now = UINT32_MAX - 5; phase = 0; cam_init(&e, now);
    sample(&e, 0, 10); assert(e.gaps == 0);
    setup(&e); sample(&e, 8, 3); sample(&e, 0, 8);
    assert(!e.follower); /* Rejected short pulse. */
    setup(&e); good_turn(&e); e.result.bins[0] = 11;
    assert(!cam_encode(&e, now, 0, frame, sizeof frame));
    setup(&e); good_turn(&e); e.result.transitions = 399;
    assert(!cam_encode(&e, now, 0, frame, sizeof frame));
    puts("PASS angular bins, index closure, reverse, invalid, gap, timeout, overflow, wrap, debounce, framing");
    return 0;
}
