/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic electrical fixture, not hardware measurements. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Engine e;
static unsigned tick;
static void emit(void)
{
    char line[768];
    size_t n=wire_encode(&e,line,sizeof line);
    assert(n); fwrite(line,1,n,stdout);
}
static void hold(unsigned raw, unsigned n)
{
    while(n--) {
        engine_sample(&e,++tick,raw);
        if (tick%100==0) emit();
    }
}
int main(void)
{
    static const unsigned ring[]={0,1,3,2};
    engine_init(&e); emit();
    hold(0,QUALIFY+REARM);
    for (unsigned i=1;i<=96;i++)
        hold(ring[i%4] | (ring[i%4]<<2),QUALIFY);
    for (unsigned i=1;i<=96;i++)
        hold(ring[i%4] | (ring[(4-(i/2)%4)%4]<<2),QUALIFY);
    emit();
    assert(e.good==2 && e.last==48 && e.opposite==1);
}
