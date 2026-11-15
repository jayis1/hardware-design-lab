/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic acquisition, not data from a physical float fixture. */
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static uint32_t now;
static void run(Engine *e, unsigned mask, unsigned count)
{
    for (unsigned i=0;i<count;i++) engine_sample(e,++now,mask);
    char line[768]; size_t n=wire_encode(e,line,sizeof line);
    if (n) fwrite(line,1,n,stdout);
}
int main(void)
{
    Engine e; engine_init(&e);
    run(&e,0,100); run(&e,1,100); run(&e,9,100);
    run(&e,11,100); run(&e,3,100); /* open in mid-high band */
    run(&e,15,100); /* close simultaneous with HIGH: unknown */
    run(&e,9,100); run(&e,1,100);
    return 0;
}
