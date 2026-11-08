/* Author: jayis1. SPDX-License-Identifier: MIT. Synthetic inputs only. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static void run(Engine *e, unsigned mask, unsigned count)
{
    while(count--) {
        engine_sample(e,e->now+1,mask);
        char frame[768]; size_t n=wire_encode(e,frame,sizeof frame);
        assert(n); fwrite(frame,1,n,stdout);
    }
}
int main(void)
{
    Engine e; engine_init(&e);
    run(&e,5,30); run(&e,0,10); run(&e,9,20); run(&e,10,60);
    run(&e,15,10); run(&e,5,60);
    run(&e,0,10); run(&e,5,60);
    return 0;
}
