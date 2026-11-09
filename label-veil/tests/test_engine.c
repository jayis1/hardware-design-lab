/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick;
static void samples(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e, ++tick, raw);
        assert(engine_valid(e));
    }
}
static void reset(Engine *e)
{
    tick = 0;
    engine_init(e);
    samples(e, 0, 8);
    assert(e->state == READY);
}
static void trial(Engine *e, unsigned truth, unsigned answer)
{
    unsigned dock = 1u | (truth << 1);
    samples(e, dock, 8);
    assert(e->state == ACTIVE);
    samples(e, dock | (4u << answer), 8);
    samples(e, dock, 8);
    samples(e, 0, 8);
}
int main(void)
{
    Engine e;
    char frame[768];
    reset(&e);
    assert(wire_crc("123456789", 9) == 0x29b1);
    /* Independent category-pair oracle, with rolling repeated trials. */
    unsigned oracle[4] = {0};
    for (unsigned word = 0; word < 256; ++word) {
        reset(&e);
        memset(oracle, 0, sizeof oracle);
        for (unsigned j = 0; j < 4; ++j) {
            unsigned pair = (word >> (2*j)) & 3;
            trial(&e, pair >> 1, pair & 1);
            ++oracle[pair];
            for (unsigned k = 0; k < 4; ++k) assert(e.cells[k] == oracle[k]);
            assert(e.completed == j+1 && e.rejected == 0);
            assert(wire_encode(&e, frame, sizeof frame));
        }
    }
    reset(&e);
    samples(&e, 1, 8);
    samples(&e, 5, 8);
    samples(&e, 1, 8);
    samples(&e, 9, 1); /* Opposite raw glitch taints even without qualification. */
    samples(&e, 1, 8);
    samples(&e, 0, 8);
    assert(e.completed == 0 && e.rejected == 1);
    reset(&e);
    samples(&e, 1, 8);
    samples(&e, 3, 1); /* Hidden identity changed under dock. */
    samples(&e, 5, 8);
    samples(&e, 1, 8);
    samples(&e, 0, 8);
    assert(e.rejected == 1);
    reset(&e);
    samples(&e, 1, 8);
    samples(&e, 0, 1); /* Removal begins before answer qualification. */
    samples(&e, 5, 8);
    samples(&e, 0, 8);
    assert(e.rejected == 1);
    reset(&e);
    samples(&e, 5, 8); /* Insertion and response tied. */
    samples(&e, 0, 8);
    assert(e.rejected == 1 && e.completed == 0);
    reset(&e);
    samples(&e, 1, 8);
    samples(&e, 5, 8);
    samples(&e, 1, 8);
    samples(&e, 5, 8); /* Repeated declaration. */
    samples(&e, 0, 8);
    assert(e.rejected == 1);
    reset(&e);
    samples(&e, 1, 8);
    engine_sample(&e, tick+2, 1);
    assert(e.state == HALT && e.rejected == 1 && e.gaps == 1);
    assert(engine_valid(&e));
    samples(&e, 0, 8);
    assert(e.rejected == 1);
    reset(&e);
    samples(&e, 1, 8);
    samples(&e, 5, 8);
    samples(&e, 1, 8);
    /* A completion at TIMEOUT is rejected; one tick before is accepted. */
    e.age = TIMEOUT-QUALIFY;
    samples(&e, 0, QUALIFY);
    assert(e.rejected == 1 && e.completed == 0);
    reset(&e);
    samples(&e, 1, 8);
    samples(&e, 5, 8);
    samples(&e, 1, 8);
    e.age = TIMEOUT-QUALIFY-1;
    samples(&e, 0, QUALIFY);
    assert(e.completed == 1 && e.last_ms == TIMEOUT-1);
    reset(&e);
    e.sequence = LIMIT;
    samples(&e, 0, 1);
    assert(e.state == HALT && (e.fault & F_COUNTER));
    reset(&e);
    e.now = UINT32_MAX; tick = UINT32_MAX;
    samples(&e, 0, 1);
    assert(e.state == READY);
    engine_init(&e); tick=0;
    samples(&e, 1, 20); /* Occupied boot is never an insertion. */
    assert(e.starts == 0 && e.state == WAIT);
    samples(&e, 0, 8);
    trial(&e, 1, 1);
    assert(e.cells[3] == 1);
    size_t size = wire_encode(&e, frame, sizeof frame);
    assert(size && !memcmp(frame, "LV1,", 4));
    for (size_t cap=0; cap<size; ++cap) assert(wire_encode(&e, frame, cap)==0);
    puts("PASS category oracle, raw guards, deadlines, gap, wrap, boot, encoder bounds");
}
