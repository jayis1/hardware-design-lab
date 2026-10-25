/* Author: jayis1. Synthetic contact stimulus, not physical wash data. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
static Engine engine;
static uint32_t now, sequence;
static void hold(uint8_t mask, unsigned duration)
{
    while (duration--) {
        engine_sample(&engine, ++now, mask);
    }
    char frame[512];
    size_t n = wire_status(&engine, sequence++, frame, sizeof frame);
    fwrite(frame, 1, n, stdout);
}
int main(void)
{
    engine_init(&engine, 0);
    hold(0, 20);
    hold(8, 20);
    hold(0, 20);
    hold(1, 200);
    hold(3, 100);
    hold(0, 20);
    hold(8, 20);
    hold(0, 20);
    hold(5, 200);
    hold(0, 20);
    return 0;
}
