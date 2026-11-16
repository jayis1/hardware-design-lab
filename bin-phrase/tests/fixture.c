/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Engine e;
static void hold(unsigned raw,unsigned n) {
    while(n--) {
        engine_sample(&e,e.now+1,raw);
        char frame[768]; size_t len=wire_encode(&e,frame,sizeof frame);
        assert(len); fputs(frame,stdout);
    }
}
int main(void) {
    engine_init(&e); hold(0,10); hold(8,5);
    hold(9,8); hold(8,8); hold(14,8); hold(8,8);
    hold(9,8); hold(8,8); hold(0,8);
    return 0;
}
