/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic acquisition only: actual engine and actual wire encoder. */
#include <stdio.h>
#include <assert.h>
#include "engine.h"
#include "wire.h"
static Engine e;
static void emit(void)
{
    char frame[768];
    size_t count = wire_encode(&e, frame, sizeof frame);
    assert(count);
    assert(fwrite(frame, 1, count, stdout) == count);
}
static void drive(unsigned ticks, unsigned raw)
{
    while (ticks--) {
        engine_sample(&e, e.now + 1, raw);
        if (!(e.now % 100)) emit();
    }
}
int main(void)
{
    engine_init(&e);
    drive(300, 0);
    drive(100, 8);
    drive(200, 9);
    drive(200, 11);
    drive(200, 15);
    drive(100, 12);
    drive(100, 0);
    emit();
    return 0;
}
