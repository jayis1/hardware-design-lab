/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static Engine e;
static uint32_t now;
static uint32_t sequence;
static void output(void)
{
    char frame[512];
    if (wire_status(&e, sequence++, frame, sizeof frame)) fputs(frame, stdout);
}
static void hold(unsigned mask, unsigned ms)
{
    while (ms--) engine_sample(&e, ++now, (uint8_t)mask);
    output();
}
int main(void)
{
    engine_init(&e, 0);
    output();
    for (unsigned trial=0; trial<4; ++trial) {
        hold(5, 140);
        hold(4, 100);
        hold(0, 200);
        hold(2, 100);
        hold(10, 20);
    }
}
