/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
static Engine e;
static uint32_t tick;
static uint32_t seq;
static void hold(uint8_t mask, unsigned ms)
{
    char line[256];
    for (unsigned i=0;i<ms;++i) {
        engine_sample(&e,mask,++tick);
        if (tick%250==0) {
            if (!wire_status(&e,seq++,line,sizeof line))
                return;
            fputs(line,stdout);
        }
    }
}
int main(void)
{
    engine_init(&e,0);
    hold(UP|LATCH,1250);
    hold(UP|LATCH|MARK,250);
    hold(UP|LATCH,2000);
    hold(LATCH,250);
    hold(DOWN|LATCH,2000);
    hold(LATCH,250);
    hold(DOWN|LATCH,1250);
    hold(LATCH,250);
    hold(UP|LATCH,1250);
    hold(UP,500);
    return 0;
}
